// Battery readout & soft power-off.
// - BOARD_HAS_PMIC: AXP2101 via XPowersLib (1.75 AMOLED / SmartRing-Plus V2).
// - BOARD_HAS_V1_BAT: SmartRing-Plus V1 (GPIO1 ADC1_CH0 voltage divider ÷2 + OCV table + GPIO47 soft power-off).
#include "battery.h"
#include "config.h"

#include <Arduino.h>

#if BOARD_HAS_PMIC
#include <Wire.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

static XPowersPMU PMU;
static bool s_ok = false;

bool battery_begin() {
#if defined(PIN_PW_OFF) && (PIN_PW_OFF >= 0)
    pinMode(PIN_PW_OFF, OUTPUT);
    digitalWrite(PIN_PW_OFF, LOW);
#endif
    s_ok = PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL);
    if (s_ok) {
        PMU.enableBattDetection();
        PMU.enableBattVoltageMeasure();
        Serial.println("[batt] AXP2101 ready");
    } else {
        Serial.println("[batt] AXP2101 not found");
    }
    return s_ok;
}

bool battery_present() { return s_ok && PMU.isBatteryConnect(); }
int  battery_percent() { return s_ok ? PMU.getBatteryPercent() : -1; }
bool battery_charging() { return s_ok && PMU.isCharging(); }
bool battery_vbus_in() { return s_ok && PMU.isVbusIn(); }

void battery_enable_codec_rail() {
    if (!s_ok) return;
#if defined(BOARD_SMARTRING_PLUS)
    PMU.setALDO3Voltage(3300);
    PMU.enableALDO3();
    Serial.println("[batt] ALDO3 enabled @3.3V");
#else
    PMU.setALDO1Voltage(3300);   // ES8311 analog supply (the reference board enables this)
    PMU.enableALDO1();
    Serial.println("[batt] ALDO1 (codec AVDD) enabled @3.3V");
#endif
}

void battery_power_off() {
#if defined(PIN_PW_OFF) && (PIN_PW_OFF >= 0)
    Serial.println("[batt] soft power off via GPIO47");
    Serial.flush();
    pinMode(PIN_PW_OFF, OUTPUT);
    digitalWrite(PIN_PW_OFF, HIGH);
    delay(PW_OFF_HOLD_MS);
    digitalWrite(PIN_PW_OFF, LOW);
#else
    if (!s_ok) { Serial.println("[batt] power off: no PMIC"); return; }
    Serial.println("[batt] power off");
    Serial.flush();
    PMU.shutdown();
#endif
}

#elif BOARD_HAS_V1_BAT

// SmartRing-Plus V1 battery monitor ported from VIEWESMART/smartring_plus/bsp_battery.c:
// Single-cell LiPo 3.7V / 600mAh (3.30V empty .. 4.20V full), voltage divider 1:2 on GPIO1.
struct BatCurvePoint {
    float voltage_v;
    int   percent;
};

static const BatCurvePoint s_curve[] = {
    { 3.30f,   0 },
    { 3.40f,   5 },
    { 3.50f,  10 },
    { 3.55f,  15 },
    { 3.60f,  20 },
    { 3.65f,  28 },
    { 3.70f,  40 },
    { 3.75f,  48 },
    { 3.80f,  55 },
    { 3.85f,  62 },
    { 3.90f,  70 },
    { 3.95f,  76 },
    { 4.00f,  82 },
    { 4.05f,  88 },
    { 4.10f,  93 },
    { 4.15f,  97 },
    { 4.20f, 100 },
};
static constexpr int CURVE_POINTS = sizeof(s_curve) / sizeof(s_curve[0]);

static constexpr int   SAMPLES_PER_READING = 16;
static constexpr int   AVG_WINDOW          = 8;
static constexpr float CHG_ENTER_V         = 4.25f;
static constexpr float CHG_EXIT_V          = 4.15f;
static constexpr int   CHG_CONFIRM_COUNT   = 2;
static constexpr float V_VALID_MIN         = 2.80f;

static float    s_avgBuf[AVG_WINDOW] = {};
static int      s_avgCount           = 0;
static int      s_avgIdx             = 0;
static bool     s_charging           = false;
static int      s_chgStreak          = 0;
static int      s_socUi              = -1;
static bool     s_primed             = false;
static uint32_t s_lastSampleMs       = 0;

static int percent_from_voltage(float v) {
    if (v <= s_curve[0].voltage_v) return s_curve[0].percent;
    if (v >= s_curve[CURVE_POINTS - 1].voltage_v) return s_curve[CURVE_POINTS - 1].percent;
    for (int i = 1; i < CURVE_POINTS; ++i) {
        if (v <= s_curve[i].voltage_v) {
            const float t = (v - s_curve[i - 1].voltage_v)
                          / (s_curve[i].voltage_v - s_curve[i - 1].voltage_v);
            return s_curve[i - 1].percent
                 + (int)(t * (s_curve[i].percent - s_curve[i - 1].percent) + 0.5f);
        }
    }
    return 0;
}

static float read_voltage_once() {
    uint32_t mv_sum = 0;
    for (int i = 0; i < SAMPLES_PER_READING; ++i) {
        mv_sum += analogReadMilliVolts(PIN_BAT_ADC);
    }
    const float mv = (float)mv_sum / (float)SAMPLES_PER_READING;
    return (mv / 1000.0f) * BAT_ADC_DIVIDER;
}

static void v1_sample_if_due() {
    const uint32_t now = millis();
    if (s_primed && (uint32_t)(now - s_lastSampleMs) < 1000U) return;
    s_lastSampleMs = now;

    const float v = read_voltage_once();
    s_avgBuf[s_avgIdx] = v;
    s_avgIdx = (s_avgIdx + 1) % AVG_WINDOW;
    if (s_avgCount < AVG_WINDOW) s_avgCount++;

    float sum = 0.0f;
    for (int i = 0; i < s_avgCount; ++i) sum += s_avgBuf[i];
    const float v_avg = sum / (float)s_avgCount;

    if (v_avg < V_VALID_MIN) return;

    bool want = s_charging;
    if (!s_charging && v_avg > CHG_ENTER_V)      want = true;
    else if (s_charging && v_avg < CHG_EXIT_V)   want = false;

    if (want != s_charging) {
        if (++s_chgStreak >= CHG_CONFIRM_COUNT) {
            s_charging = want;
            s_chgStreak = 0;
        }
    } else {
        s_chgStreak = 0;
    }

    const bool use_chg_cal = s_charging || (v_avg > CHG_ENTER_V);
    float v_cell = use_chg_cal ? (v_avg - BAT_CHG_IR_DROP_V) : v_avg;
    if (v_cell < s_curve[0].voltage_v) v_cell = s_curve[0].voltage_v;
    if (v_cell > s_curve[CURVE_POINTS - 1].voltage_v) v_cell = s_curve[CURVE_POINTS - 1].voltage_v;

    const int target = percent_from_voltage(v_cell);
    if (!s_primed || s_socUi < 0) {
        s_socUi = target;
        s_primed = true;
    } else if (target > s_socUi) {
        s_socUi++;
    } else if (target < s_socUi) {
        s_socUi--;
    }
    if (s_socUi < 0)   s_socUi = 0;
    if (s_socUi > 100) s_socUi = 100;
}

bool battery_begin() {
#if defined(PIN_PW_OFF) && (PIN_PW_OFF >= 0)
    pinMode(PIN_PW_OFF, OUTPUT);
    digitalWrite(PIN_PW_OFF, LOW);
#endif
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
    v1_sample_if_due();
    Serial.printf("[batt] SmartRing-Plus V1 ADC ready (GPIO%d, soc=%d%%, chg=%d)\n",
                  PIN_BAT_ADC, s_socUi, (int)s_charging);
    return true;
}

bool battery_present() {
    v1_sample_if_due();
    return s_primed && (s_socUi >= 0);
}

int battery_percent() {
    v1_sample_if_due();
    return s_primed ? s_socUi : -1;
}

bool battery_charging() {
    v1_sample_if_due();
    return s_charging;
}

bool battery_vbus_in() {
    v1_sample_if_due();
    return s_charging;
}

void battery_enable_codec_rail() {
    // V1 powers ES8311 from fixed 3V3 rail; no PMIC LDO configuration needed.
}

void battery_power_off() {
#if defined(PIN_PW_OFF) && (PIN_PW_OFF >= 0)
    Serial.println("[batt] V1 soft power off: holding GPIO47 HIGH for 3.5 s...");
    Serial.flush();
    pinMode(PIN_PW_OFF, OUTPUT);
    digitalWrite(PIN_PW_OFF, HIGH);
    delay(PW_OFF_HOLD_MS);
    digitalWrite(PIN_PW_OFF, LOW);
#endif
}

#else   // !BOARD_HAS_PMIC && !BOARD_HAS_V1_BAT

bool battery_begin()   { Serial.println("[batt] no PMIC on this board"); return false; }
bool battery_present() { return false; }
int  battery_percent() { return -1; }
bool battery_charging(){ return false; }
bool battery_vbus_in() { return false; }
void battery_enable_codec_rail() {}
void battery_power_off() {}

#endif
