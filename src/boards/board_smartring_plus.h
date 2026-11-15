#pragma once
// Board Support Header for VIEWE SmartRing-Plus (ESP32-S3-N16R8)
// Hardware revision: V1 (GPIO1 ADC battery + GPIO47 soft power-off)
// Display panel: ST77916 360x360 QSPI (Version B: UE018HV-RB39-A004A)
// Reference BSP: https://github.com/VIEWESMART/smartring_plus

#define BOARD_NAME          "VIEWE SmartRing-Plus"
#define BOARD_PIO_ENV       "esp32-s3-smartring-plus"

// ---------- Screen geometry (360x360 round) ----------
#define SCREEN_W            360
#define SCREEN_H            360
#define SCREEN_CX           180
#define SCREEN_CY           180
#define RADAR_R_OUTER_PX    168

// ---------- Panel (ST77916 QSPI) ----------
#define PIN_LCD_CS          11
#define PIN_LCD_RST         39
#define PIN_LCD_SCLK        10
#define PIN_LCD_D0          12
#define PIN_LCD_D1          13
#define PIN_LCD_D2          15
#define PIN_LCD_D3          14
#define PIN_LCD_TE          38
#define LCD_COL_OFFSET      0
#define LCD_ROW_OFFSET      0
#define LCD_QSPI_HZ         40000000

// ---------- Backlight (LEDC PWM, active-high via Q1 AO3400A) ----------
#define PIN_LCD_BL          46
#define LCD_BL_PWM_FREQ     4000
#define LCD_BL_PWM_BITS     8

// ---------- Shared I2C (touch + IMU + audio codec) ----------
#define PIN_I2C_SDA         8
#define PIN_I2C_SCL         9

// ---------- Touch (CST816S, single-point, CST8xx protocol) ----------
#define TOUCH_DRIVER_CST820 1
#define I2C_ADDR_TOUCH      0x15
#define PIN_TP_INT          41
#define PIN_TP_RST          40
#define TP_MIRROR_X         false
#define TP_MIRROR_Y         false

// ---------- Battery & Power (V1: GPIO1 ADC1_CH0 + GPIO47 soft power-off) ----------
#define BOARD_HAS_PMIC      0
#define BOARD_HAS_V1_BAT    1
#define PIN_BAT_ADC         1
#define BAT_ADC_DIVIDER     2.0f
#define BAT_CHG_IR_DROP_V   0.30f
#define PIN_PW_OFF          47
#define PW_OFF_HOLD_MS      3500

// ---------- Audio (ES8311 + I2S + NS4150B PA) ----------
#define BOARD_HAS_AUDIO     1
#define PIN_I2S_MCLK        48
#define PIN_I2S_BCLK        21
#define PIN_I2S_LRCLK       17
#define PIN_I2S_DOUT        16
#define PIN_I2S_DIN         18
#define PIN_AUDIO_PA        45

// ---------- Sensors & Buttons ----------
#define I2C_ADDR_IMU        0x6B
#define PIN_IMU_INT1        42
#define I2C_ADDR_RTC        0x51
#define PIN_BOOT_BUTTON     0
