# M5Stack Tab5 Multi-App Launcher

Single sketch (`Test.ino`) that boots into a menu (`MenuApp`) of small
utility apps, driven by `AppManager`. Architecture, app list, and API
citations are in the original build plan; this file is conventions and
gotchas discovered while building it, so they don't get re-learned the
hard way.

## Camera capture is not currently possible from this toolchain

**Confirmed empirically, don't re-attempt without new information.** Tab5
has a real onboard camera (2MP, MIPI-CSI, sensor documented inconsistently
as SC2356/SC202CS), but there is no Arduino-level API for it anywhere:
neither M5Unified nor M5GFX has any camera code, and the classic bundled
`esp32-camera` library only supports the older parallel/DVP interface, not
MIPI-CSI. The real driver stack is ESP-IDF's newer V4L2-style video
subsystem (`espressif/esp_video` + `espressif/esp_cam_sensor`), added to a
project via an `idf_component.yml` manifest and Espressif's IDF Component
Manager.

**That manifest mechanism does not work for a plain `.ino` sketch compiled
via arduino-cli/Arduino IDE.** Verified directly: an `idf_component.yml`
placed at the sketch root never even got copied into arduino-cli's sketch
build staging directory (only recognized source file types are), and no
`managed_components` fetch ever ran -- the component manager step simply
isn't part of the `.ino` sketch compile pipeline this project uses. The
"Arduino as an ESP-IDF component + IDF Component Manager" docs that turn
up when researching this describe the *opposite* direction (using Arduino
*inside* a native ESP-IDF/`idf.py` project), not pulling ESP-IDF
components into an Arduino sketch. The one concrete example found of Tab5
camera capture working at all (a bidirectional video-streaming project) is
built with PlatformIO on pure ESP-IDF, explicitly not Arduino IDE.

A from-scratch `CameraDriver` was written and compile-tested against real
`esp_video_init.h`/`esp_video_device.h` struct/macro definitions (fetched
directly from the `espressif/esp-video-components` GitHub repo, so the
V4L2 open/ioctl/mmap code itself was reasonably well-grounded) to confirm
this concretely rather than guess: it failed immediately with `sys/mman.h:
No such file or directory` -- a standard POSIX header the real ESP-IDF
example uses successfully, absent entirely from this Arduino core's bundled
toolchain (`m5stack:esp32 3.3.8`) header search paths. That dead-end
attempt (`idf_component.yml`, `CameraDriver.h/.cpp`) was removed after
confirming it broke the whole sketch's build (Arduino compiles every .cpp
in the folder regardless of whether anything calls it yet).

If this gets revisited: check whether a newer `m5stack:esp32` core version
adds camera support, or whether M5Unified/M5GFX ship a high-level wrapper
-- don't restart from the raw `esp_video` V4L2 API without re-confirming
the toolchain gap is actually closed. The alternative (switching this
project to PlatformIO/native ESP-IDF) would mean rebuilding the entire
launcher outside Arduino IDE, not just adding a camera app -- a device
only runs one firmware at a time, so a from-scratch PlatformIO project
sitting next to this one wouldn't let both coexist without picking one
build system for the whole project.

## Conventions every screen must follow

- **Every app's content starts at `y = AppManager::kStatusBarHeight`, not
  `y = 0`.** `AppManager::draw()` paints the status bar (Home/clock/battery)
  *every frame*, before calling the active app's `draw()`. Any widget that
  draws into `y < kStatusBarHeight` will fight the status bar for those
  pixels every single frame -- this is exactly what caused two separate
  "flashing header" bugs (one from a missing `setTextSize()` call leaking
  stale state into the bar, one from `Keyboard` drawing its title/preview
  area from `y=0`). If you add a new full-screen app or modal widget, offset
  its content by `AppManager::kStatusBarHeight` like every existing app
  does, and check it against `Keyboard.cpp`'s `drawPreview()`/`layout()` as
  the reference for how a modal widget should do it.

- **Always call `setTextSize()` before `drawString()`/`print()`.** Text
  size (and datum, and color) are mutable state shared on the `M5.Display`
  object, not scoped per draw call. If you skip `setTextSize()`, your text
  renders at whatever size the last thing drawn happened to leave behind --
  which varies frame to frame depending on what the *previous* app screen
  did. `AppManager::drawStatusBar()` is a good example of getting this
  right (see the comment there).

- **`LGFX_Button` labels silently truncate at 11 chars, without a null
  terminator.** `LGFX_Button::initButtonUL()`'s `label` param is copied
  into an internal `char[12]` via `strncpy` -- if your label is 11+ chars,
  you get garbled/overflowing text, not a clean truncation. For anything
  that might be long (app names, SSIDs, filenames), pass a short placeholder
  (or `""`) to `initButtonUL()`, and pass the real string as `drawButton()`'s
  `long_name` argument at draw time instead (see `MenuApp::draw()` or
  `WifiSettingsApp::drawMain()`).

- **A class's private `static constexpr` members aren't visible to
  anonymous-namespace code in the same .cpp file**, even though it feels
  like "the same file." Static data tables (zone lists, unit tables, key
  layouts) that need to be sized against a count like `kZoneCount` either
  need to be actual class members (declared in the header, defined
  out-of-class in the .cpp with the qualified name -- see
  `TimeZoneApp::kZones`) or the count itself needs to be `public` (see
  `UnitConverterApp::kCategoryCount`/`kKeyCount`, `NewsApp::kPresetCount`).
  Hit this exact compile error twice building the second batch of apps.

- **Use `Theme.h`'s palette (`Theme::kPrimary`, `kDanger`, `kSuccess`,
  `kBackground`, `kSurface`, `kOutline`, `kTextPrimary`, `kTextSecondary`),
  not raw `TFT_*` named colors.** The whole UI was moved onto this palette
  so it reads as one cohesive dark theme instead of a grab-bag of the
  default library colors (harsh saturated navy/maroon/darkgreen on pure
  black). If you add a new screen, reuse these constants for the same
  semantics everywhere else uses them (outline = `kOutline`, idle/neutral
  fill = `kOutline` or `kSurface`, primary action = `kPrimary`, confirm =
  `kSuccess`, destructive = `kDanger`).

- **Color arguments must all share one type within a single call, and that
  type controls how the color is *interpreted*, not just its C++ type.**
  `LGFX_Button::initButtonUL()`/`drawButton()` and `LGFXBase::setTextColor()`
  etc. are templated and pick their conversion behavior from the argument
  type: `int`/`int16_t`/`int32_t` are interpreted as **RGB565**, `uint32_t`
  is interpreted as **raw RGB888** (`c & 0xFFFFFF`, no conversion). `TFT_*`
  macros are `static constexpr int` (RGB565), and `Theme::` constants are
  `uint32_t` (RGB888) -- mixing them in one call, or declaring a local
  color variable as `int fill = Theme::kSomething;`, either fails to
  compile ("deduced conflicting types for parameter T") or silently
  renders the wrong color if it's a lone default parameter value that
  happens to still typecheck (this genuinely happened: `TextBox.h`'s old
  `uint32_t color = TFT_WHITE` default rendered as near-black/dark-blue,
  not white, since `0xFFFF` got reinterpreted as RGB888 instead of RGB565).
  Local variables holding a `Theme::` color must be declared `uint32_t`,
  not `int`. Hit the "conflicting types" version of this 4 separate times
  across this codebase.

- **Lists don't scroll -- cap them and let old items fall off,
  don't try to fit more.** Every list screen (Notepad, Voice Recorder,
  Wi-Fi networks/scan results, To-Do, News headlines) uses a fixed
  `kMaxRows`/`kMaxItems`-style cap (usually 8) sized to fit one screen
  without scrolling, since there's no scroll gesture handling anywhere in
  this codebase. If a new list-based app needs more than ~8-12 items
  visible, that's a real scrolling feature to design, not something to
  paper over by shrinking rows.

## Networking gotchas

- **Parse JSON from `http.getString()`, not `http.getStream()`.** Open-Meteo
  (and probably most APIs) serves chunked responses. `deserializeJson()`
  reading directly off a live `HTTPClient` stream can bail out early if the
  socket delivers bytes in bursts with small gaps, producing a parse
  failure on JSON that's actually complete and valid. Always buffer the
  full body first, then parse the buffer (see `WeatherClient.cpp`'s
  `httpGetJson()`).
- **Tab5 has no native Wi-Fi.** The ESP32-P4 talks to the onboard ESP32-C6
  over SDIO; `WiFi.setPins(...)` (see `Test.ino`'s `setup()`) must run
  before any `WiFi.*` call or nothing works.
- **A "successful" HTTP request can still fail to parse.** Don't let a JSON
  parse failure silently report as HTTP 200 -- propagate it as a distinct
  error so the failure mode is visible on-screen/in Serial instead of
  looking like "no results."
- **rss2json.com (used by `NewsClient` to convert RSS to JSON) doesn't parse
  every valid RSS feed.** CNN's and Al Jazeera's feeds are reachable and
  valid XML but come back as `{"status":"error",...}` from rss2json; BBC,
  NPR, and The Guardian all work fine. Verify any new preset feed against
  `https://api.rss2json.com/v1/api.json?rss_url=<feed>` directly before
  adding it, don't assume a feed works just because the raw XML loads.

## Hardware quirks

- **Tab5's default mic gain is quiet.** The board config sets
  `mic_config_t.magnification = 2` (library default elsewhere is 16). Bump
  it via `M5.Mic.config()` before `M5.Mic.begin()` if recordings sound too
  quiet (see `VoiceRecorderApp::startRecording()`).
- **Speaker default master volume is 64/255**, not the max. Call
  `M5.Speaker.setVolume(255)` explicitly for playback.
- **Mic and speaker share one I2S bus and cannot run concurrently.** Always
  `end()` one before `begin()`-ing the other.
- **SD card is SD_MMC (4-bit SDIO), not SPI.** Use `M5.getPin(m5::pin_name_t::sd_mmc_*)`
  rather than hardcoding GPIO numbers.
- **RTC is never set on its own.** Nothing syncs it automatically; call
  `syncRtcFromNtp()` (`TimeSync.cpp`) after Wi-Fi connects, or the clock
  reads whatever un-synced default the RTC chip powers on with.
  `syncRtcFromNtp()` applies whatever timezone offset is currently
  persisted (`currentTimezoneOffsetSec()`, set via `TimeZoneApp`, UTC by
  default until the user picks a zone). DST here is a manual toggle
  against a static offset list, not automatic.
- **Don't derive "current UTC" by reading the RTC and subtracting the
  previously-persisted offset -- prefer `time(nullptr)`.** The ESP32
  system clock is kept in true UTC by NTP and ticks forward on its own,
  completely independent of the RTC chip. `applyTimezoneOffset()`
  (`TimeSync.cpp`) originally *only* did the RTC-minus-old-offset
  reconciliation, which requires the RTC and the persisted offset to never
  drift out of sync with each other -- if they ever do (even once), the
  next timezone change compounds the error: a stale/wrong RTC reading gets
  treated as if it were UTC and offset *again*, silently doubling it. This
  caused a real bug: MST+DST (should be UTC-6) displayed 6 hours further
  off than expected -- effectively UTC-12, exactly double. Fixed by
  preferring `time(nullptr)` (guarded by the same `kPlausibleEpoch`
  sanity check `syncRtcFromNtp()` uses) whenever it's available, only
  falling back to the fragile RTC-reconciliation math when there's no
  NTP-synced system clock at all (never connected to Wi-Fi this boot).
  Both paths now log to Serial (`[TimeSync] ...`) so which one ran, and
  the exact values involved, are visible if this class of bug resurfaces.
- **No debug output exists unless you add it.** `Serial.begin(115200)` runs
  in `setup()`; add `Serial.printf` logging for anything you're debugging
  blind, especially networking -- see `WeatherClient.cpp`'s `httpGetJson()`
  for the pattern (log the request, the result code, and any parse error).

## Testing

Nothing in this project has been run on physical hardware from within an
agent session -- only compiled (`arduino-cli compile --fqbn
m5stack:esp32:m5stack_tab5 ...`). If the Arduino IDE is open with this
sketch loaded, its language server compiles in the background too; point
manual `arduino-cli compile` runs at an isolated `--build-path` (e.g. a
temp dir) to avoid both processes racing the same build-cache directory,
which produces confusing unrelated-looking toolchain errors (corrupted ELF,
missing dependency files, etc.).

- **`arduino-cli` is not on PATH in this environment** -- it's not a
  separate install. The Arduino IDE (2.x) bundles its own copy at
  `%LOCALAPPDATA%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`.
  Invoke it by that full path (a shell session losing this across restarts
  produced a plain "command not found" that looked like a missing install).

## Cross-cutting overlays live in AppManager, not in an app

`AppManager` only calls `update()`/`draw()` on the single active app each
frame -- a feature that needs to interrupt the screen *regardless* of
which app is currently open (the Alarm Clock's ringing alert is the first
example) can't be implemented inside that feature's own app class, since
that class's `update()`/`draw()` simply won't run while some other app is
active. The pattern used for this: `AppManager` gains a small bit of
state (`_alarmRinging` etc.) and a public trigger method (`ringAlarm()`);
`Test.ino`'s `loop()` checks the underlying condition every frame
independent of the active app (`AlarmStore::checkDue()`) and calls the
trigger method when it fires; `AppManager::update()`/`draw()` check that
state *before* delegating to the active app and short-circuit into a
full-screen overlay when set, resuming normal delegation once dismissed.
The status bar itself is the original example of this same idea (drawn
every frame on top of whatever app is active) -- the alarm overlay just
takes it a step further by fully pre-empting the active app's own
draw/update instead of layering on top of it. If a future feature needs
the same kind of "must interrupt any screen" behavior, extend
`AppManager` the same way rather than trying to make an individual app
reach outside its own screen.
