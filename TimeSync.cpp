#include "TimeSync.h"

#include <M5Unified.h>
#include <Preferences.h>
#include <WiFi.h>

#include <ctime>

namespace {
const char* kPrefsNamespace = "settings";
const char* kOffsetKey = "tzoffset";

// Any time() value below this (roughly Nov 2023) means NTP hasn't landed
// yet -- the clock still reads its un-synced default.
const time_t kPlausibleEpoch = 1700000000;

// Days since 1970-01-01 for a (proleptic Gregorian) UTC calendar date.
// Standard public-domain algorithm (Howard Hinnant's civil_from_days,
// inverted) -- used instead of timegm() (a POSIX/GNU extension, not
// guaranteed present in ESP32's newlib) so this doesn't depend on any
// libc timezone behavior at all, just plain arithmetic.
long daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  int yoe = (int)(y - era * 400);
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097L + doe - 719468;
}

// Converts an RTC datetime (interpreted as a plain UTC calendar date/time,
// no timezone applied) to a Unix epoch second count.
time_t rtcDateTimeToEpoch(const decltype(M5.Rtc.getDateTime())& dt) {
  long days = daysFromCivil(dt.date.year, dt.date.month, dt.date.date);
  return days * 86400L + dt.time.hours * 3600L + dt.time.minutes * 60L +
         dt.time.seconds;
}
}  // namespace

long currentTimezoneOffsetSec() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/true);
  long offset = prefs.getLong(kOffsetKey, 0);
  prefs.end();
  return offset;
}

void applyTimezoneOffset(long newOffsetSec) {
  time_t sysNow = time(nullptr);
  if (sysNow >= kPlausibleEpoch) {
    // Preferred path: the system clock is already true UTC (NTP-synced at
    // boot or on last Wi-Fi connect) and ticks forward on its own, so this
    // doesn't depend on the RTC or the previously persisted offset having
    // stayed perfectly in sync with each other.
    time_t newLocal = sysNow + newOffsetSec;
    Serial.printf(
        "[TimeSync] applyTimezoneOffset(%ld) via system clock: utc=%ld -> "
        "local=%ld\n",
        newOffsetSec, (long)sysNow, (long)newLocal);
    if (M5.Rtc.isEnabled()) M5.Rtc.setDateTime(gmtime(&newLocal));
  } else if (M5.Rtc.isEnabled()) {
    // Fallback: no NTP-synced system clock this boot (never connected to
    // Wi-Fi yet). Reconcile against the RTC's own reading instead --
    // requires the RTC and the *previously persisted* offset to not have
    // drifted apart, which is a weaker guarantee than the path above.
    long oldOffsetSec = currentTimezoneOffsetSec();
    auto dt = M5.Rtc.getDateTime();
    time_t oldLocal = rtcDateTimeToEpoch(dt);
    time_t utc = oldLocal - oldOffsetSec;
    time_t newLocal = utc + newOffsetSec;
    Serial.printf(
        "[TimeSync] applyTimezoneOffset(%ld) via RTC fallback: "
        "oldOffset=%ld, rtcLocal=%ld -> utc=%ld -> local=%ld\n",
        newOffsetSec, oldOffsetSec, (long)oldLocal, (long)utc, (long)newLocal);
    M5.Rtc.setDateTime(gmtime(&newLocal));
  }

  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);
  prefs.putLong(kOffsetKey, newOffsetSec);
  prefs.end();
}

bool syncRtcFromNtp(unsigned long timeoutMs) {
  if (WiFi.status() != WL_CONNECTED) return false;

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  time_t now = time(nullptr);
  unsigned long start = millis();
  while (now < kPlausibleEpoch && millis() - start < timeoutMs) {
    delay(100);
    now = time(nullptr);
  }
  if (now < kPlausibleEpoch) return false;

  long offset = currentTimezoneOffsetSec();
  time_t local = now + offset;
  Serial.printf(
      "[TimeSync] syncRtcFromNtp: utc=%ld, offset=%ld -> local=%ld\n",
      (long)now, offset, (long)local);
  M5.Rtc.setDateTime(gmtime(&local));
  return true;
}
