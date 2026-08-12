#pragma once

// Syncs M5's RTC from NTP over an already-connected Wi-Fi link, applying
// the currently selected timezone offset (see currentTimezoneOffsetSec()).
// Returns true if the sync succeeded within timeoutMs (false if not
// connected, or NTP didn't respond in time).
bool syncRtcFromNtp(unsigned long timeoutMs = 5000);

// Reads the persisted timezone offset from Preferences (seconds east of
// UTC). Defaults to 0 (UTC) if never set.
long currentTimezoneOffsetSec();

// Switches to a new timezone offset immediately. Prefers the ESP32 system
// clock (time(nullptr), kept in true UTC by NTP and ticking forward on its
// own -- independent of the RTC chip) as the source of "current UTC" when
// it's available; only falls back to reconciling against the RTC's own
// (old-offset) reading if the system clock was never NTP-synced this boot
// (e.g. no Wi-Fi yet). The fallback path requires the RTC and the
// previously *persisted* offset to have never drifted out of sync with
// each other -- if they ever do, it silently compounds the error, which is
// exactly why the system-clock path is preferred whenever possible.
// Persists newOffsetSec for future syncs either way.
void applyTimezoneOffset(long newOffsetSec);
