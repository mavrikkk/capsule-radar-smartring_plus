#pragma once
// Capsule Radar — central configuration (ESP32-S3-Touch-AMOLED-1.75)

#define FW_VERSION "1.9.0"

// ---------- Default home location & range (used until WiFiManager sets them) ----------
#define HOME_LAT_DEFAULT      38.8409   // Dénia, Spain — override via captive portal
#define HOME_LON_DEFAULT       0.1059
#define RANGE_KM_DEFAULT      30.0f
// Query radius is wider than the displayed range so aircraft just outside the ring are
// already tracked when they cross inward, and zooming in doesn't need a new fetch.
// Keep this modest: in busy airspace (London, Frankfurt, NYC) radius² scales the JSON
// from ~15 KB to 150+ KB and blows the parser buffer / starves LVGL of heap.
#define ADSB_QUERY_MULT       1.4f
#define ADSB_QUERY_MIN_KM     12.0f
#define ADSB_QUERY_MAX_KM     150.0f
// Tap-to-cycle range steps on the radar screen:
static constexpr float RANGE_STEPS_KM[] = { 10.0f, 20.0f, 30.0f, 50.0f, 100.0f };
static constexpr int   RANGE_STEPS_N    = sizeof(RANGE_STEPS_KM) / sizeof(RANGE_STEPS_KM[0]);
// Steps exposed by the web setup page (/save) — superset of RANGE_STEPS_KM:
static constexpr float WEB_RANGE_KM[]   = { 10.0f, 15.0f, 25.0f, 30.0f, 50.0f, 100.0f, 150.0f };
static constexpr int   WEB_RANGE_N      = sizeof(WEB_RANGE_KM) / sizeof(WEB_RANGE_KM[0]);
#define RANGE_KM_MIN          10.0f
#define RANGE_KM_MAX          150.0f

// ---------- Polling & motion ----------
#define POLL_INTERVAL_MS          2000   // USB / externally powered
#define POLL_INTERVAL_BATTERY_MS  5000   // on LiPo (saves WiFi radio power)
#define MOTION_INTERP         1       // 1 = dead-reckon between polls using gs/track
#define AC_STALE_MS           15000   // drop tracks not seen for this long
// How long a seen hex stays in the "already alerted" table before its slot can be reused
// (or re-alerted if it leaves and comes back). Must be much longer than AC_STALE_MS so a
// brief ADS-B dropout doesn't re-trigger the new-aircraft / watch beep when the plane
// reappears a few polls later.
#define ALERT_FORGET_MS       (15UL * 60UL * 1000UL)   // 15 min
#define ALERT_MEMORY_SLOTS    256                      // >= 2x ADSB_MAX_AIRCRAFT
// Emergency squawk (7500/7600/7700) re-alert interval while the condition persists:
#define ALERT_EMERG_REPING_MS (10UL * 60UL * 1000UL)   // 10 min
// Local weather (Open-Meteo, free, no API key) refresh period for the header & page 3:
#define WEATHER_REFRESH_MS    1800000UL                // 30 min
// Precipitation radar overlay (RainViewer, free, no API key) tile refresh period:
#define WX_RADAR_REFRESH_MS   300000UL                 // 5 min (RainViewer updates every 10)
// Animated cloud background image refresh period:
#define CLOUD_IMAGE_REFRESH_MS 600000UL                // 10 min

// ---------- Screen geometry (board header may override before the fallback below) ----------
// Note: board header is included at the bottom of this file; only boards with a
// non-466x466 panel define SCREEN_W/H themselves.
#if !defined(BOARD_LCD21) && !defined(BOARD_SMARTRING_PLUS)
#define SCREEN_W              466
#define SCREEN_H              466
#define SCREEN_CX             233
#define SCREEN_CY             233
#define RADAR_R_OUTER_PX      218     // outer range ring radius (leaves margin for bezel)
#endif
#define SWEEP_PERIOD_MS       8000    // LVGL sweep: 1 full rotation every 8 s
#define SWEEP_COMP_PERIOD_MS  40000   // compositor sweep: 1 full rotation every 40 s
#define SWEEP_FRAME_MS        30      // sweep tick
#define SWEEP_TRAIL_DEG       38.0f   // fading trail behind the lead line
#define SWEEP_TRAIL_STEPS     20      // LVGL sweep: lines in the trail
#define SWEEP_TRAIL_OPA       72      // trail opacity at the lead edge (fades to 0)
#define SWEEP_LEAD_OPA        217     // lead line opacity
#define SWEEP_LEAD_W          2.0f    // compositor: lead line width in pixels (anti-aliased, round ends)
#define PULSE_PERIOD_MS       2600    // centre ripple: one expansion
#define PULSE_D_MIN           10      // ring diameter at the start / end of an expansion (px)
#define PULSE_D_MAX           54
#define PULSE_W               2       // ring width (px)
#define PULSE_OPA             220     // ring opacity at the start (fades to 0)
#define LV_COLOR_DEPTH_BITS   16
// LCD_COL_OFFSET / LCD_ROW_OFFSET / LCD_QSPI_HZ are panel-specific -> board header.
#define BRIGHTNESS_DEFAULT    200     // 0..255, panel brightness via cmd 0x51 or LEDC PWM
// Face-down sleep (QMI8658, logic in facedown_sleep.h): the IMU is read every
// FACEDOWN_CHECK_MS, and FACEDOWN_COUNT face-down readings in a row with no touch in between
// turn the screen off (~1.6 s). Accelerometer Z at ±2 g full scale: 16384 LSB/g.
#define FACEDOWN_CHECK_MS     400
#define FACEDOWN_COUNT        4
#define FACEDOWN_THRESHOLD    9000    // ~0.55 g past zero, on the side opposite the resting pose
#define TZ_STR                "CET-1CEST,M3.5.0,M10.5.0/3" // POSIX TZ (Spain) for local time/date
#define BRIGHTNESS_IDLE       25      // dimmed after no touch for IDLE_DIM_MS
#define IDLE_DIM_MS           20000   // dim the screen after this long without a touch
// Idle clock: instead of dimming, the radar animates into an analog clock (clock_face.h has
// the animation, radar_view.cpp the dial and the dimmed scene, display.cpp's compositor the
// moving hands). Angles 0 = 12 o'clock; lengths are fractions of RADAR_R_OUTER_PX.
#define CLOCK_ENTER_MS        2000    // radar -> clock animation
#define CLOCK_EXIT_MS         1200    // clock -> radar animation
#define CLOCK_LOCK_FRAC       0.75f   // the spinning sweep line is on the second from here on
#define CLOCK_MIN_SPIN_DEG    120.0f  // ...after spinning forward at least this far
#define CLOCK_RADAR_OPA       150     // planes, trails, rings, map behind the clock (255 = as usual)
#define CLOCK_RADAR_OPA_STEP1 200     // step 1 of the 2-step fade on Enter (avoids intermediate renders)
#define CLOCK_TRAIL_OPA       150     // sweep trail at the start of Enter (fades to 0 at LOCK_FRAC)
#define CLOCK_BRIGHTNESS_PCT  70      // backlight in clock mode (% of the user's brightness setting)
#define CLOCK_HOUR_LEN        0.50f
#define CLOCK_MIN_LEN         0.76f
#define CLOCK_SEC_LEN         0.94f
#define CLOCK_SEC_TAIL        0.14f   // counterweight past the centre
#define CLOCK_HOUR_W          8       // px
#define CLOCK_MIN_W           5       // px
#define CLOCK_OUTLINE_W       4       // dark halo total width around the hour/minute hands (px)
#define CLOCK_OUTLINE_OPA     200     // halo opacity — keeps hands legible over map & rings
#define CLOCK_CAP_R           7       // centre cap outer radius (px, theme lead colour)
#define CLOCK_CAP_DOT_R       3       // centre cap inner dot radius (px, dark)
#define CLOCK_NUM_R           0.84f   // 12 / 3 / 6 / 9 centre radius
#define CLOCK_HTICK_R0        0.90f   // hour tick inner end
#define CLOCK_MTICK_R0        0.965f  // minute tick inner end
#define CLOCK_TICK_R1         0.995f  // tick outer end
#define CLOCK_NUM_OPA         190
#define CLOCK_HTICK_OPA       170
#define CLOCK_MTICK_OPA       80

// ---------- Touch / UI interaction ----------
#define UI_THEME_HOLD_MS         1500 // long-press on empty radar scope -> cycle colour theme
// Touch sampling + release-glitch filter (logic in touch_queue.h):
// CST9217 only asserts data-ready at ~50-100 Hz; reads between frames return NODATA even
// while a finger is held down, which LVGL would otherwise see as RELEASED -> PRESSED and
// fire a phantom click on every drag frame.
#define TOUCH_SAMPLE_MS          20   // poll interval (100 Hz is redundant; 50 Hz matches chip)
#define TOUCH_NODATA_RELEASE_MS  100  // hold last DOWN across NODATA gaps shorter than this
#define TOUCH_QUEUE_LEN          32   // ~640 ms of samples buffered across slow LVGL frames
// Detail card close-button ("X") hit zone in screen pixels (top-right of the card):
#define UI_CARD_CLOSE_HIT        21
#define UI_CARD_CLOSE_MARGIN     12

// ---------- ADS-B feeds ----------
// Three free, unauthenticated ADSBexchange-v2-compatible REST endpoints, rotated on failure:
//   #0 airplanes.live — primary (1 req/s rate limit; returns HTTP 429 if exceeded)
//   #1 adsb.fi        — first fallback (sometimes returns HTTP 403 depending on cloud egress IP)
//   #2 adsb.lol       — second fallback
// All three expose: GET https://<host>/v2/point/<lat>/<lon>/<radius_nm>
#define ADSB_PRIMARY_HOST     "api.airplanes.live"
#define ADSB_OPENDATA_HOST    "opendata.adsb.fi"
#define ADSB_FALLBACK_HOST    "api.adsb.lol"
#define ADSB_PROVIDER_COUNT   3
#define ADSB_USER_AGENT       "CapsuleRadar/" FW_VERSION " (ESP32-S3 hobby; +https://github.com/socquique/capsule-radar)"
#define ADSB_HTTPS_INSECURE   1       // skip CA bundle for simplicity on hobby device
// Bound the TLS handshake so a wedged server / half-open NAT session cannot stall the
// network task for lwIP's default ~120 s (during which no provider failover happens):
#define TLS_HANDSHAKE_S       10
#define ADSB_MAX_AIRCRAFT     60
// Provider pacing & cooldown policy (pure logic lives in adsb_pacing.h):
#define ADSB_COOLDOWN_403_MS         900000UL   // 15 min initial cooldown when a host 403s
#define ADSB_COOLDOWN_403_MAX_MS   21600000UL   // 6 h cap after repeated 403s (15m -> 30m -> 1h -> 2h -> 4h -> 6h)
#define ADSB_SPACING_STEP_MS           4000UL   // +4 s per 429 on top of the active poll floor
#define ADSB_SPACING_MAX_MS           60000UL   // cap per-host spacing at 60 s
#define ADSB_SPACING_EASE_OKS             3     // decay spacing by one step after N consecutive 200s...
#define ADSB_SPACING_EASE_QUIET_MS   600000UL   // ...or after 10 min without a 429 from that host
// When the primary (airplanes.live) has been throttled to a slow spacing and we are leaning
// on a fallback to keep the UI fresh, probe the primary at least this often so its spacing
// can decay via the quiet-window rule once the upstream rate-limit window clears:
#define ADSB_SPACING_PROBE_MS       3600000UL   // 1 h
// Only call it a "feed outage" (log warning, force WiFi reconnect, trip the feed watchdog)
// once EVERY provider has been silent for at least this long — an instant rotation onto a
// healthy fallback is not an outage:
#define ADSB_SILENCE_OUTAGE_MAX_MS    10000UL
// Main-loop watchdog over the ADS-B task (lives in feed_watchdog.h). Because TLS handshakes
// can still block inside mbedTLS/lwIP past socket timeouts when the radio or heap is unwell,
// the UI loop monitors the task's progress and takes escalating recovery steps:
#define ADSB_FEED_STALE_MS            60000UL   // >60 s with no aircraft update -> UI shows feed-stale warning
#define ADSB_STUCK_MS                180000UL   // >3 min with no poll completion -> bounce WiFi; if internal heap is also fragmented below ADSB_STUCK_MIN_LARGEST_BLOCK, reboot
#define ADSB_STUCK_MIN_LARGEST_BLOCK  28000     // bytes; mbedTLS handshake needs ~16-24 KB contiguous internal RAM
#define ADSB_STUCK_HARD_MS          1800000UL   // >30 min with no poll completion regardless of heap -> reboot

// ---------- Diagnostics ----------
// Set to 1 to print a `[mem]` line every 5 s with internal/PSRAM free + largest contiguous
// internal block (useful when hunting TLS alloc failures). Off by default to keep Serial clean.
#define DEBUG_MEM             0

// ---------- Board ----------
// The pin map, panel gaps, touch driver and which peripherals exist all live in a
// per-board header. Select one with a build flag in platformio.ini; the 1.75 is the
// default so an unflagged build behaves exactly as before.
//   -DBOARD_LCD21          -> Waveshare ESP32-S3-Touch-LCD-2.1
//   -DBOARD_AMOLED_143     -> Waveshare ESP32-S3-Touch-AMOLED-1.43
//   -DBOARD_SMARTRING_PLUS -> VIEWE SmartRing-Plus (ESP32-S3-N16R8, ST77916 360x360)
//   (none)                 -> Waveshare ESP32-S3-Touch-AMOLED-1.75  (reference board)
#if defined(BOARD_LCD21)
#  include "boards/board_lcd_21.h"
#elif defined(BOARD_AMOLED_143)
#  include "boards/board_amoled_143.h"
#elif defined(BOARD_SMARTRING_PLUS)
#  include "boards/board_smartring_plus.h"
#else
#  include "boards/board_amoled_175.h"
#endif

// Safety net: catches a board header that still has placeholder pins in it.
#if (PIN_LCD_SCLK < 0) || (PIN_I2C_SDA < 0)
#  error "board header: QSPI/I2C pins are placeholders (-1). Fill in the real values."
#endif

#ifndef ADSB_TASK_STACK_BYTES
#define ADSB_TASK_STACK_BYTES 16384
#endif

// An on/off buzzer instead of a codec (the LCD 2.1): see audio.cpp.
#ifndef BOARD_HAS_BUZZER
#define BOARD_HAS_BUZZER 0
#endif

#ifndef BOARD_HAS_V1_BAT
#define BOARD_HAS_V1_BAT 0
#endif
