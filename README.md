# tab5_utilities_firmware

Arduino firmware for the [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5) that
turns the device into a small tablet of offline- and network-backed utility apps. A
single sketch (`tab5_utilities_firmware.ino`) boots into a home-screen menu
(`MenuApp`) and hands off to whichever app the user taps; `AppManager` drives
whichever app is active and draws a persistent status bar (Home button, clock,
battery) on top of it every frame.

## Requirements

- M5Stack Tab5 hardware (ESP32-P4 + ESP32-C6 Wi-Fi co-processor).
- Arduino IDE 2.x with the `m5stack:esp32` board core and `M5Unified`/`M5GFX`
  installed.
- A microSD card in the Tab5's SD_MMC slot for Notepad, Voice Recorder, and
  the alarm store's persistence needs (most other apps use on-chip
  `Preferences` instead).
- Wi-Fi for the apps that call out to the internet (Weather, News,
  Dictionary); everything else works fully offline.

Build with `arduino-cli compile --fqbn m5stack:esp32:m5stack_tab5 ...` (see
`CLAUDE.md` for build/toolchain notes). This has only been verified via
compilation, not yet run on physical hardware from within an agent session.

## Apps

Tapping a tile on the home menu opens one of the following:

- **Settings** — a picker that hands off to two sub-screens:
  - *Wi-Fi* — scans for nearby networks, connects with a password entered on
    the on-screen keyboard, and remembers saved networks (via `WifiStore`)
    so the device auto-reconnects on future boots. Saved networks can be
    forgotten from the list.
  - *Time Zone* — a grid of named fixed UTC-offset zones plus a manual DST
    toggle; selecting one immediately re-applies the offset to the RTC.
- **Calculator** — offline four-function calculator with immediate (chained)
  evaluation, like a basic pocket calculator.
- **Weather** — current conditions (temperature, wind, sky) for a saved city
  via [Open-Meteo](https://open-meteo.com/) (free, no API key). Asks for a
  city the first time it's opened, geocodes and caches the location, and
  offers a shortcut into Wi-Fi settings if there's no connection.
- **Notepad** — lists text notes stored on the SD card and opens a full-screen
  keyboard (multiline mode) to create or edit one.
- **Voice Recorder** — records mono 16kHz PCM to a WAV file on the SD card and
  plays recordings back through the speaker. Mic and speaker share one I2S
  bus, so recording and playback can't happen at the same time.
- **Stopwatch** — combined stopwatch and countdown timer with quick-start
  presets, switched via a tab toggle. Keeps running correctly even if you
  leave the screen and come back.
- **To-Do List** — up to 8 short to-do items; tap to toggle done/not-done,
  delete, or add new items via the on-screen keyboard. Persisted on-device.
- **Unit Converter** — converts between units in four categories (Length,
  Weight, Temperature, Volume) with a numeric keypad, converting live as
  digits are entered. Fully offline.
- **Dictionary** — looks up a word's definitions (via
  [dictionaryapi.dev](https://dictionaryapi.dev/)) or synonyms (via
  [Datamuse](https://www.datamuse.com/api/)), both free and keyless.
- **News** — pick a preset RSS feed (or enter a custom URL) and browse
  headlines, tapping one for its full description. Feeds are converted from
  RSS to JSON via [rss2json.com](https://rss2json.com/). The selected feed is
  remembered for next time.
- **Alarm Clock** — set one-time or repeating alarms with hour/minute
  steppers and AM/PM. An alarm interrupts whatever app is on screen with a
  full-screen ringing overlay, not just the Alarm Clock app itself.
- **Formulas** — a reference calculator with over 30 formulas across six
  categories (Everyday, Finance, Geometry, Physics, Health, Electronic).
  Pick a category, pick a formula, fill in its labeled fields with the
  numeric keypad, and tap Calculate. Fully offline.

## Data sources & attribution

The network-backed apps call free, keyless public APIs directly from the
device — no credentials are stored or required anywhere in this repo:

- **Weather** — [Open-Meteo](https://open-meteo.com/). Free tier is scoped to
  non-commercial use and licensed CC BY 4.0, which requires attribution; this
  project is a personal/hobby build, and the Weather screen credits
  Open-Meteo directly (see `WeatherApp::drawShow()`). See
  [Open-Meteo's terms](https://open-meteo.com/en/terms).
- **Dictionary** — [dictionaryapi.dev](https://dictionaryapi.dev/) for
  definitions, [Datamuse](https://www.datamuse.com/api/) for synonyms. Both
  free with no attribution requirement.
- **News** — [rss2json.com](https://rss2json.com/) converts RSS to JSON on
  the free, keyless tier.

## Architecture notes

- Every app implements a common `App` interface (`App.h`) — `onEnter()`,
  `onExit()`, `update()`, `draw()` — so `AppManager` can drive all of them
  uniformly and allocate big buffers/peripherals only while an app is active.
- Shared widgets (`Keyboard`, `TextBox`) and a shared color palette
  (`Theme.h`) keep every screen visually consistent.
- Features that need to interrupt the active screen regardless of which app
  is open (currently just the alarm overlay) live in `AppManager` itself
  rather than in an individual app — see `CLAUDE.md` for why.

See `CLAUDE.md` for detailed conventions, hardware quirks, and gotchas
discovered while building this (status bar layout, color type pitfalls,
Wi-Fi/SD bring-up, camera-support investigation, etc.).
