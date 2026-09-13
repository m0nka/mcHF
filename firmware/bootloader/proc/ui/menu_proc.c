/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:       menu_proc.c                                                   **
**  Description:     console-like menu system for bootloader diagnostics           **
**  Last Modified:                                                                 **
**  Licence:         https://github.com/m0nka/mcHF/blob/main/LICENSE              **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"
#include "version.h"

#include "lcd_low.h"
#include "lcd_high.h"

#include "selftest_proc.h"
#include "bms_proc.h"
#include "bms_gold.h"
#include "menu_proc.h"

// -----------------------------------------------------------------------
// Externs
// -----------------------------------------------------------------------
extern ulong   sys_timer;
extern uchar   gen_boot_reason_err;
extern ushort  batt_status;
extern uchar   flash_source;           // 0 = SD, 1 = USB (in mchf_pro_board.c)
extern uchar   charge_mode;
extern uchar   soc;
extern short   pack_curr;
extern ushort  pack_volt;

// -----------------------------------------------------------------------
// Layout constants
//
// Landscape view on a 480x800 ILI9806E panel:
//   X axis (0..479) = vertical,  X=0 = TOP,  X=479 = BOTTOM
//   Y axis (0..799) = horizontal, Y=0 = LEFT, Y=799 = RIGHT
//
// FillRect(x, y, w, h): w along X (vertical), h along Y (horizontal)
// DisplayStringAt(Xpos, Ypos, ...): Xpos=vertical, Ypos=horizontal
//
// Items cascade DOWNWARD with increasing X values.
// -----------------------------------------------------------------------

// Header: topmost strip
#define HDR_X           0       // header start (top of screen)
#define HDR_H           20      // header height

// Menu area: title + 5 items, going downward
#define MENU_X          28      // menu title text
#define ITEM_X          54      // first menu item
#define ITEM_STEP       22      // add per item (items go downward)

// Log area: label + 9 lines, below the menu
#define LOG_X           182     // "Output:" label
#define LOG_LINE_X      204     // first log line
#define LOG_LINE_STEP   18      // add per line (lines go downward)

// Status bar: near the bottom
#define STAT_X          400     // status text line

// Footer: bottommost strip, near physical buttons
#define FTR_X           430     // footer text line
#define FTR_H           24      // footer height

// Horizontal positions
#define COL_LEFT        10      // default left column
#define SCREEN_W        800     // visible horizontal extent (Y axis)

// -----------------------------------------------------------------------
// Local state
// -----------------------------------------------------------------------
static uchar    menu_id         = MENU_MAIN;
static uchar    menu_pending    = 0xFF;     // pending key event
static uchar    menu_dirty      = 1;        // screen needs full repaint
static uchar    action_running  = 0;        // action in progress
static ulong    menu_start_time = 0;        // startup lockout timer

// Log buffer
static char     log_buf[MENU_LOG_MAX_LINES][48];
static uchar    log_count = 0;

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
static void menu_draw(void);
static void menu_draw_header(void);
static void menu_draw_items(void);
static void menu_draw_log(void);
static void menu_draw_status(void);
static void menu_draw_footer(void);
static void menu_log_clear(void);
static void menu_log_add(const char *msg);

static void action_hw_sdram(void);
static void action_hw_sdcard(void);
static void action_hw_bms(void);
static void action_hw_all(void);
static void action_fw_app(void);
static void action_fw_baseband(void);
static void action_bms_status(void);
static void action_bms_gold_backup(void);
static void action_bms_gold_flash(void);
static void action_system_info(void);
static void action_boot_radio(void);

// Extended HW test actions
static void action_5v_toggle(void);
static void action_fan_toggle(void);
static void action_leds_toggle(void);
static void action_backlight_cycle(void);
static void action_bq25730_ch224a(void);
static void action_codec_i2c(void);
static void action_si5351_i2c(void);
static void action_gt911_i2c(void);
static void action_gps_check(void);
static void action_lora_check(void);
static void action_encoders_read(void);

// -----------------------------------------------------------------------
// Menu definitions
// -----------------------------------------------------------------------
typedef struct {
    const char  *label;
    uchar       key;
} menu_item_t;

static const menu_item_t main_menu[] = {
    { "[F1] Hardware Tests",    MENU_BTN_1 },
    { "[F2] Firmware Update",   MENU_BTN_2 },
    { "[F3] BMS Tools",         MENU_BTN_3 },
    { "[F4] System Info",       MENU_BTN_4 },
    { "[F5] Boot Radio",        MENU_BTN_5 },
};

static const menu_item_t hw_menu[] = {
    { "[F1] Core (RAM/SD/BMS)", MENU_BTN_1 },
    { "[F2] Power/GPIO",        MENU_BTN_2 },
    { "[F3] I2C Bus",           MENU_BTN_3 },
    { "[F4] Peripherals",       MENU_BTN_4 },
    { "[F5] Back",              MENU_BTN_5 },
};

static const menu_item_t hw_core_menu[] = {
    { "[F1] SDRAM Test",        MENU_BTN_1 },
    { "[F2] SD Card Test",      MENU_BTN_2 },
    { "[F3] BMS Comms Test",    MENU_BTN_3 },
    { "[F4] Run All Core",      MENU_BTN_4 },
    { "[F5] Back",              MENU_BTN_5 },
};

static const menu_item_t hw_power_menu[] = {
    { "[F1] 5V/8V Toggle",      MENU_BTN_1 },
    { "[F2] Fan Toggle",        MENU_BTN_2 },
    { "[F3] LEDs Toggle",       MENU_BTN_3 },
    { "[F4] Backlight Cycle",   MENU_BTN_4 },
    { "[F5] Back",              MENU_BTN_5 },
};

static const menu_item_t hw_i2c_menu[] = {
    { "[F1] BQ25730 + CH224A",  MENU_BTN_1 },
    { "[F2] Codec (CS4245)",    MENU_BTN_2 },
    { "[F3] SI5351 Clock Gen",  MENU_BTN_3 },
    { "[F4] GT911 Touch",       MENU_BTN_4 },
    { "[F5] Back",              MENU_BTN_5 },
};

static const menu_item_t hw_periph_menu[] = {
    { "[F1] GPS Check",         MENU_BTN_1 },
    { "[F2] LoRa Check",        MENU_BTN_2 },
    { "[F3] Encoders Read",     MENU_BTN_3 },
    { "[F4] ---",               MENU_BTN_4 },
    { "[F5] Back",              MENU_BTN_5 },
};

static const menu_item_t fw_menu_sd[] = {
    { "[F1] Flash App Proc",    MENU_BTN_1 },
    { "[F2] Flash Baseband",    MENU_BTN_2 },
    { "[F3] Source: SD Card",   MENU_BTN_3 },
    { "[F4] ---",               MENU_BTN_4 },
    { "[F5] Back",              MENU_BTN_5 },
};

static const menu_item_t fw_menu_usb[] = {
    { "[F1] Flash App Proc",    MENU_BTN_1 },
    { "[F2] Flash Baseband",    MENU_BTN_2 },
    { "[F3] Source: USB Stick",  MENU_BTN_3 },
    { "[F4] ---",               MENU_BTN_4 },
    { "[F5] Back",              MENU_BTN_5 },
};

static const menu_item_t bms_menu[] = {
    { "[F1] BMS Status",        MENU_BTN_1 },
    { "[F2] Gold Backup",       MENU_BTN_2 },
    { "[F3] Gold Flash",        MENU_BTN_3 },
    { "[F4] ---",               MENU_BTN_4 },
    { "[F5] Back",              MENU_BTN_5 },
};

static const menu_item_t *menu_get_items(uchar *count)
{
    switch(menu_id)
    {
        case MENU_HW_TESTS:    *count = 5; return hw_menu;
        case MENU_HW_CORE:     *count = 5; return hw_core_menu;
        case MENU_HW_POWER:    *count = 5; return hw_power_menu;
        case MENU_HW_I2C:      *count = 5; return hw_i2c_menu;
        case MENU_HW_PERIPH:   *count = 5; return hw_periph_menu;
        case MENU_FW_UPDATE:   *count = 5; return flash_source ? fw_menu_usb : fw_menu_sd;
        case MENU_BMS_TOOLS:   *count = 5; return bms_menu;
        default:               *count = 5; return main_menu;
    }
}

static const char *menu_get_title(void)
{
    switch(menu_id)
    {
        case MENU_HW_TESTS:    return "Hardware Tests";
        case MENU_HW_CORE:     return "HW > Core Tests";
        case MENU_HW_POWER:    return "HW > Power/GPIO";
        case MENU_HW_I2C:      return "HW > I2C Bus";
        case MENU_HW_PERIPH:   return "HW > Peripherals";
        case MENU_FW_UPDATE:   return flash_source ? "FW Update [USB]" : "FW Update [SD]";
        case MENU_BMS_TOOLS:   return "BMS Tools";
        default:               return "Main Menu";
    }
}

// -----------------------------------------------------------------------
// Log helpers
// -----------------------------------------------------------------------
static void menu_log_clear(void)
{
    log_count = 0;
}

static void menu_log_add(const char *msg)
{
    if(log_count >= MENU_LOG_MAX_LINES)
    {
        // Scroll up
        for(uchar i = 0; i < MENU_LOG_MAX_LINES - 1; i++)
            strcpy(log_buf[i], log_buf[i + 1]);

        log_count = MENU_LOG_MAX_LINES - 1;
    }

    strncpy(log_buf[log_count], msg, 47);
    log_buf[log_count][47] = 0;
    log_count++;
}

// -----------------------------------------------------------------------
// Drawing — console style, black bg, bright text, line separators
// -----------------------------------------------------------------------
static void menu_draw_header(void)
{
    char buff[64];

    lcd_high_SetBackColor(LCD_COLOR_BLACK);
    lcd_high_SetFont(&Font16);

    // Title at the top of the screen
    lcd_high_SetTextColor(LCD_COLOR_CYAN);
    lcd_high_DisplayStringAt(HDR_X + 2, COL_LEFT, (uchar *)DEVICE_STRING, LEFT_MODE);

    // Version on the right
    lcd_high_SetTextColor(LCD_COLOR_WHITE);
    sprintf(buff, "v %d.%d.%d.%d", 	MCHF_L_VER_MAJOR, MCHF_L_VER_MINOR,
            						MCHF_L_VER_RELEASE, MCHF_L_VER_BUILD);

    lcd_high_DisplayStringAt(HDR_X + 2, 200, (uchar *)buff, LEFT_MODE);

    // Separator below header
    lcd_high_FillRect(HDR_X + HDR_H, 0, 1, SCREEN_W, LCD_COLOR_GRAY);
}

static void menu_draw_items(void)
{
    uchar count = 0;
    const menu_item_t *items = menu_get_items(&count);
    const char *title = menu_get_title();

    lcd_high_SetBackColor(LCD_COLOR_BLACK);

    // Menu title
    lcd_high_SetFont(&Font20);
    lcd_high_SetTextColor(LCD_COLOR_CYAN);
    lcd_high_DisplayStringAt(MENU_X, COL_LEFT, (uchar *)title, LEFT_MODE);

    // Menu items (going downward = increasing X)
    lcd_high_SetFont(&Font16);
    lcd_high_SetTextColor(LCD_COLOR_WHITE);

    for(uchar i = 0; i < count; i++)
    {
        ushort x = ITEM_X + (i * ITEM_STEP);
        lcd_high_DisplayStringAt(x, COL_LEFT + 10, (uchar *)items[i].label, LEFT_MODE);
    }
}

static void menu_draw_log(void)
{
    lcd_high_SetBackColor(LCD_COLOR_BLACK);
    lcd_high_SetFont(&Font16);

    // Log title
    lcd_high_SetTextColor(LCD_COLOR_YELLOW);
    lcd_high_DisplayStringAt(LOG_X, COL_LEFT, (uchar *)"Output:", LEFT_MODE);

    // Log lines (going downward = increasing X)
    lcd_high_SetTextColor(LCD_COLOR_LIGHTGREEN);

    for(uchar i = 0; i < log_count; i++)
    {
        ushort x = LOG_LINE_X + (i * LOG_LINE_STEP);
        lcd_high_DisplayStringAt(x, COL_LEFT + 4, (uchar *)log_buf[i], LEFT_MODE);
    }
}

static void menu_draw_status(void)
{
    char buff[48];

    // Separator above status line
//    lcd_high_FillRect(STAT_X - 4, 0, 1, SCREEN_W, LCD_COLOR_GRAY);

    // Clear status line area
//    lcd_high_FillRect(STAT_X, 0, 16, SCREEN_W, LCD_COLOR_BLACK);

    lcd_high_SetBackColor(LCD_COLOR_BLACK);
    lcd_high_SetFont(&Font16);

    // Battery info
    if(soc != 0xFF)
    {
        lcd_high_SetTextColor(LCD_COLOR_WHITE);
        sprintf(buff, "BAT:%d%%", soc);
        lcd_high_DisplayStringAt(STAT_X, COL_LEFT, (uchar *)buff, LEFT_MODE);

        sprintf(buff, "%dmV", pack_volt);
        lcd_high_DisplayStringAt(STAT_X, 120, (uchar *)buff, LEFT_MODE);

        sprintf(buff, "%dmA   ", pack_curr);
        lcd_high_DisplayStringAt(STAT_X, 220, (uchar *)buff, LEFT_MODE);
    }

    // BMS + charge
    if((batt_status != 0xFFFF) && (batt_status != 0))
    {
        lcd_high_SetTextColor(LCD_COLOR_LIGHTGREEN);
        lcd_high_DisplayStringAt(STAT_X, 320, (uchar *)"BMS:OK ", LEFT_MODE);
    }
    else
    {
        lcd_high_SetTextColor(LCD_COLOR_RED);
        lcd_high_DisplayStringAt(STAT_X, 320, (uchar *)"BMS:N/A", LEFT_MODE);
    }

    if(charge_mode)
    {
        lcd_high_SetTextColor(LCD_COLOR_RED);
        lcd_high_DisplayStringAt(STAT_X, 410, (uchar *)"[CHG]", LEFT_MODE);
    }
    else
    {
        lcd_high_SetTextColor(LCD_COLOR_BLACK);
        lcd_high_DisplayStringAt(STAT_X, 410, (uchar *)"     ", LEFT_MODE);
    }
}

static void menu_draw_footer(void)
{
    // Separator above footer
    lcd_high_FillRect(FTR_X - 2, 0, 1, SCREEN_W, LCD_COLOR_GRAY);

    lcd_high_SetBackColor(LCD_COLOR_BLACK);
    lcd_high_SetFont(&Font16);
    lcd_high_SetTextColor(LCD_COLOR_GRAY);

    lcd_high_DisplayStringAt(FTR_X, COL_LEFT,
                             (uchar *)"F1-F5:select  PWR hold:off", LEFT_MODE);
}

static void menu_clear_content(void)
{
    // Clear the area between header and status (menu + log zone)
    ushort top = HDR_X + HDR_H + 2;
    ushort bot = STAT_X - 6;
    lcd_high_FillRect(top, 0, bot - top, SCREEN_W, LCD_COLOR_BLACK);
}

static void menu_draw(void)
{
    lcd_high_Clear(LCD_COLOR_BLACK);

    menu_draw_header();
    menu_draw_items();
    menu_draw_log();
    menu_draw_status();
    menu_draw_footer();
}

static void menu_redraw_content(void)
{
    menu_clear_content();
    menu_draw_items();
    menu_draw_log();
}

// -----------------------------------------------------------------------
// Actions
// -----------------------------------------------------------------------
static void action_hw_sdram(void)
{
    menu_log_add("SDRAM test........");
    menu_redraw_content();

    int res = sdram_test();

    if(res == 0)
        menu_log_add("SDRAM test........PASS");
    else
    {
        char buff[48];
        sprintf(buff, "SDRAM test........FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_hw_sdcard(void)
{
    menu_log_add("SD Card test......");
    menu_redraw_content();

    int res = test_sd_card();

    if(res == 0)
    {
        menu_log_add("SD Card test......PASS");
        fs_cleanup();
    }
    else
    {
        char buff[48];
        sprintf(buff, "SD Card test......FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_hw_bms(void)
{
    menu_log_add("BMS comms test....");
    menu_redraw_content();

    if((batt_status != 0xFFFF) && (batt_status != 0))
    {
        char buff[48];
        menu_log_add("BMS comms test....PASS");
        sprintf(buff, "  Status: 0x%04x", batt_status);
        menu_log_add(buff);
        sprintf(buff, "  SOC: %d%%  Pack: %dmV", soc, pack_volt);
        menu_log_add(buff);
    }
    else
    {
        char buff[48];
        sprintf(buff, "BMS comms test....FAIL(%04x)", batt_status);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_hw_all(void)
{
    menu_log_clear();
    menu_log_add("=== Running all HW tests ===");
    menu_redraw_content();

    action_hw_sdram();
    action_hw_sdcard();
    action_hw_bms();

    menu_log_add("=== All tests complete ===");
    menu_redraw_content();
}

static void action_fw_app(void)
{
    menu_log_clear();
    menu_log_add("App proc FW update...");
    if(flash_source)
        menu_log_add("Reading radio.bin from USB...");
    else
        menu_log_add("Reading radio.bin from SD...");
    menu_redraw_content();

    uchar res = update_radio();

    if(res == 0)
        menu_log_add("FW update.........PASS");
    else
    {
        char buff[48];
        sprintf(buff, "FW update.........FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_fw_baseband(void)
{
    menu_log_clear();
    menu_log_add("Baseband FW update...");
    if(flash_source)
        menu_log_add("Reading baseband.bin from USB..");
    else
        menu_log_add("Flashing baseband.bin to bank2");
    menu_redraw_content();

    uchar res = update_baseband();

    if(res == 0)
        menu_log_add("Baseband flash....PASS");
    else
    {
        char buff[48];
        sprintf(buff, "Baseband update...FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_bms_status(void)
{
    char buff[48];

    menu_log_clear();
    menu_log_add("=== BMS Status ===");

    sprintf(buff, "Status reg: 0x%04x", batt_status);
    menu_log_add(buff);

    sprintf(buff, "SOC: %d%%", soc);
    menu_log_add(buff);

    sprintf(buff, "Pack voltage: %d mV", pack_volt);
    menu_log_add(buff);

    sprintf(buff, "Pack current: %d mA", pack_curr);
    menu_log_add(buff);

    sprintf(buff, "Charging: %s", charge_mode ? "YES" : "NO");
    menu_log_add(buff);

    menu_redraw_content();
}

static void action_bms_gold_backup(void)
{
    menu_log_clear();
    menu_log_add("BMS gold backup...");
    menu_log_add("Dumping DF to SD card...");
    menu_redraw_content();

    uchar res = bms_gold_backup_boot();

    if(res == 0)
        menu_log_add("Gold backup.......PASS");
    else
    {
        char buff[48];
        sprintf(buff, "Gold backup.......FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_bms_gold_flash(void)
{
    menu_log_clear();
    menu_log_add("BMS gold flash...");
    menu_log_add("Programming from gold.fs...");
    menu_redraw_content();

    uchar res = bms_gold_flash_boot();

    if(res == 0)
        menu_log_add("Gold flash........PASS");
    else
    {
        char buff[48];
        sprintf(buff, "Gold flash........FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_system_info(void)
{
    char buff[48];

    menu_log_clear();
    menu_log_add("=== System Info ===");

    sprintf(buff, "Boot: %d.%d.%d.%d",
            MCHF_L_VER_MAJOR, MCHF_L_VER_MINOR,
            MCHF_L_VER_RELEASE, MCHF_L_VER_BUILD);
    menu_log_add(buff);

    sprintf(buff, "MCU: STM32H747XI");
    menu_log_add(buff);

    sprintf(buff, "SYSCLK: %d MHz",
            (int)(HAL_RCC_GetSysClockFreq()/1000000));
    menu_log_add(buff);

    sprintf(buff, "Boot err: %d", gen_boot_reason_err);
    menu_log_add(buff);

    ulong fw_valid = is_firmware_valid();
    if(fw_valid == 0)
        menu_log_add("Radio FW: valid");
    else
    {
        sprintf(buff, "Radio FW: invalid(%d)", (int)fw_valid);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_boot_radio(void)
{
    if(is_firmware_valid() == 0)
    {
        lcd_high_Clear(LCD_COLOR_BLACK);
        lcd_high_SetBackColor(LCD_COLOR_BLACK);
        lcd_high_SetTextColor(LCD_COLOR_WHITE);
        lcd_high_SetFont(&Font20);
        lcd_high_DisplayStringAt(220, 150, (uchar *)"Booting radio...", LEFT_MODE);

        HAL_Delay(500);
        jump_to_fw(RADIO_FIRM_ADDR);
    }
    else
    {
        menu_log_clear();
        menu_log_add("Cannot boot: no valid firmware!");
        menu_log_add("Flash via Firmware Update menu.");
        menu_redraw_content();
    }
}

// -----------------------------------------------------------------------
// Extended HW test actions
// -----------------------------------------------------------------------
static void action_5v_toggle(void)
{
    int state = test_5v_toggle();

    if(state)
        menu_log_add("5V/8V rail.......ON");
    else
        menu_log_add("5V/8V rail.......OFF");

    menu_redraw_content();
}

static void action_fan_toggle(void)
{
    int state = test_fan_toggle();

    if(state)
        menu_log_add("Fan..............ON");
    else
        menu_log_add("Fan..............OFF");

    menu_redraw_content();
}

static void action_leds_toggle(void)
{
    int state = test_leds_toggle();

    if(state)
        menu_log_add("LEDs (PWR+TX)....ON");
    else
        menu_log_add("LEDs (PWR+TX)....OFF");

    menu_redraw_content();
}

static void action_backlight_cycle(void)
{
    char buff[48];
    int level = test_backlight_cycle();

    static const char *names[] = {
        "OFF", "25%", "50%", "75%", "100%"
    };

    sprintf(buff, "Backlight........%s", names[level]);
    menu_log_add(buff);
    menu_redraw_content();
}

static void action_bq25730_ch224a(void)
{
    uchar bq_ok = 0, ch_ok = 0;

    menu_log_add("I2C scan BQ25730+CH224A..");
    menu_redraw_content();

    test_bq25730_ch224a(&bq_ok, &ch_ok);

    if(bq_ok)
        menu_log_add("BQ25730 (0xD6)...FOUND");
    else
        menu_log_add("BQ25730 (0xD6)...MISSING");

    if(ch_ok)
        menu_log_add("CH224A  (0x44)...FOUND");
    else
        menu_log_add("CH224A  (0x44)...MISSING");

    menu_redraw_content();
}

static void action_codec_i2c(void)
{
    menu_log_add("CS4245 I2C check (0x98).");
    menu_redraw_content();

    int res = test_codec_i2c();

    if(res == 0)
        menu_log_add("CS4245...........PASS");
    else
    {
        char buff[48];
        sprintf(buff, "CS4245...........FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_si5351_i2c(void)
{
    menu_log_add("SI5351 I2C check (0xC0).");
    menu_redraw_content();

    int res = test_si5351_i2c();

    if(res == 0)
        menu_log_add("SI5351...........PASS");
    else
    {
        char buff[48];
        sprintf(buff, "SI5351...........FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_gt911_i2c(void)
{
    menu_log_add("GT911 I2C check........");
    menu_redraw_content();

    int res = test_gt911_i2c();

    if(res == 0)
        menu_log_add("GT911 Touch......PASS");
    else
    {
        char buff[48];
        sprintf(buff, "GT911 Touch......FAIL(%d)", res);
        menu_log_add(buff);
    }

    menu_redraw_content();
}

static void action_gps_check(void)
{
    menu_log_add("GPS check (5V on)......");
    menu_redraw_content();

    int res = test_gps_check();

    if(res == 0)
        menu_log_add("GPS..............PASS");
    else
        menu_log_add("GPS..............FAIL");

    menu_redraw_content();
}

static void action_lora_check(void)
{
    menu_log_add("LoRa check (5V on).....");
    menu_redraw_content();

    int res = test_lora_check();

    if(res == 0)
        menu_log_add("LoRa (SX1262)....PASS");
    else
        menu_log_add("LoRa (SX1262)....FAIL");

    menu_redraw_content();
}

static void action_encoders_read(void)
{
    char buff[48];
    uchar enc1 = 0, enc2 = 0;

    menu_log_add("Sampling encoders (1s)...");
    menu_redraw_content();

    test_encoders(&enc1, &enc2);

    sprintf(buff, "ENC1(vol) count: %d", (signed char)enc1);
    menu_log_add(buff);

    sprintf(buff, "ENC2(frq) count: %d", (signed char)enc2);
    menu_log_add(buff);

    menu_log_add("Rotate knobs, press again");
    menu_redraw_content();
}

// -----------------------------------------------------------------------
// Process a key event in the current menu
// -----------------------------------------------------------------------
static void menu_process_key(uchar key)
{
    if(action_running)
        return;

    switch(menu_id)
    {
        case MENU_MAIN:
        {
            switch(key)
            {
                case MENU_BTN_1:
                    menu_id = MENU_HW_TESTS;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                case MENU_BTN_2:
                    menu_id = MENU_FW_UPDATE;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                case MENU_BTN_3:
                    menu_id = MENU_BMS_TOOLS;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                case MENU_BTN_4:
                    action_running = 1;
                    action_system_info();
                    action_running = 0;
                    break;

                case MENU_BTN_5:
                    action_running = 1;
                    action_boot_radio();
                    action_running = 0;
                    break;

                default:
                    break;
            }
            break;
        }

        case MENU_HW_TESTS:
        {
            switch(key)
            {
                case MENU_BTN_1:
                    menu_id = MENU_HW_CORE;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                case MENU_BTN_2:
                    menu_id = MENU_HW_POWER;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                case MENU_BTN_3:
                    menu_id = MENU_HW_I2C;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                case MENU_BTN_4:
                    menu_id = MENU_HW_PERIPH;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                case MENU_BTN_5:
                    menu_id = MENU_MAIN;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                default:
                    break;
            }
            break;
        }

        case MENU_HW_CORE:
        {
            switch(key)
            {
                case MENU_BTN_1:
                    action_running = 1;
                    action_hw_sdram();
                    action_running = 0;
                    break;

                case MENU_BTN_2:
                    action_running = 1;
                    action_hw_sdcard();
                    action_running = 0;
                    break;

                case MENU_BTN_3:
                    action_running = 1;
                    action_hw_bms();
                    action_running = 0;
                    break;

                case MENU_BTN_4:
                    action_running = 1;
                    action_hw_all();
                    action_running = 0;
                    break;

                case MENU_BTN_5:
                    menu_id = MENU_HW_TESTS;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                default:
                    break;
            }
            break;
        }

        case MENU_HW_POWER:
        {
            switch(key)
            {
                case MENU_BTN_1:
                    action_running = 1;
                    action_5v_toggle();
                    action_running = 0;
                    break;

                case MENU_BTN_2:
                    action_running = 1;
                    action_fan_toggle();
                    action_running = 0;
                    break;

                case MENU_BTN_3:
                    action_running = 1;
                    action_leds_toggle();
                    action_running = 0;
                    break;

                case MENU_BTN_4:
                    action_running = 1;
                    action_backlight_cycle();
                    action_running = 0;
                    break;

                case MENU_BTN_5:
                    menu_id = MENU_HW_TESTS;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                default:
                    break;
            }
            break;
        }

        case MENU_HW_I2C:
        {
            switch(key)
            {
                case MENU_BTN_1:
                    action_running = 1;
                    action_bq25730_ch224a();
                    action_running = 0;
                    break;

                case MENU_BTN_2:
                    action_running = 1;
                    action_codec_i2c();
                    action_running = 0;
                    break;

                case MENU_BTN_3:
                    action_running = 1;
                    action_si5351_i2c();
                    action_running = 0;
                    break;

                case MENU_BTN_4:
                    action_running = 1;
                    action_gt911_i2c();
                    action_running = 0;
                    break;

                case MENU_BTN_5:
                    menu_id = MENU_HW_TESTS;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                default:
                    break;
            }
            break;
        }

        case MENU_HW_PERIPH:
        {
            switch(key)
            {
                case MENU_BTN_1:
                    action_running = 1;
                    action_gps_check();
                    action_running = 0;
                    break;

                case MENU_BTN_2:
                    action_running = 1;
                    action_lora_check();
                    action_running = 0;
                    break;

                case MENU_BTN_3:
                    action_running = 1;
                    action_encoders_read();
                    action_running = 0;
                    break;

                case MENU_BTN_5:
                    menu_id = MENU_HW_TESTS;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                default:
                    break;
            }
            break;
        }

        case MENU_FW_UPDATE:
        {
            switch(key)
            {
                case MENU_BTN_1:
                    action_running = 1;
                    action_fw_app();
                    action_running = 0;
                    break;

                case MENU_BTN_2:
                    action_running = 1;
                    action_fw_baseband();
                    action_running = 0;
                    break;

                case MENU_BTN_3:
                    // Toggle flash source: SD <-> USB
                    flash_source = flash_source ? 0 : 1;
                    menu_log_clear();
                    if(flash_source)
                        menu_log_add("Source: USB Stick");
                    else
                        menu_log_add("Source: SD Card");
                    menu_dirty = 1;
                    break;

                case MENU_BTN_5:
                    menu_id = MENU_MAIN;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                default:
                    break;
            }
            break;
        }

        case MENU_BMS_TOOLS:
        {
            switch(key)
            {
                case MENU_BTN_1:
                    action_running = 1;
                    action_bms_status();
                    action_running = 0;
                    break;

                case MENU_BTN_2:
                    action_running = 1;
                    action_bms_gold_backup();
                    action_running = 0;
                    break;

                case MENU_BTN_3:
                    action_running = 1;
                    action_bms_gold_flash();
                    action_running = 0;
                    break;

                case MENU_BTN_5:
                    menu_id = MENU_MAIN;
                    menu_log_clear();
                    menu_dirty = 1;
                    break;

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
}

// -----------------------------------------------------------------------
// Key event from keypad scanner
// -----------------------------------------------------------------------
void menu_proc_key_event(uchar x, uchar y, uchar hold)
{
	uchar pend = 0xFF;

    if(hold)
        return;

    // Ignore keys during the startup lockout — the boot entry button
    // (F4) is still held when the menu starts, and releasing it during
    // the keypad scan window registers as a short press
    if((sys_timer - menu_start_time) < 1500)
        return;

    // Coordinates to Function menu
    if((x == 1)&&(y == 2))
    	pend = MENU_BTN_1;
    else if((x == 2)&&(y == 2))
    	pend = MENU_BTN_2;
    else if((x == 3)&&(y == 3))
    	pend = MENU_BTN_3;
    else if((x == 1)&&(y == 3))
    	pend = MENU_BTN_4;
    else if((x == 2)&&(y == 3))
    	pend = MENU_BTN_5;

    menu_pending = pend;
}

// -----------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------
void menu_proc_init(void)
{
    menu_id         = MENU_MAIN;
    menu_pending    = 0xFF;
    menu_dirty      = 1;
    log_count       = 0;
    menu_start_time = sys_timer;

    menu_draw();
}

// -----------------------------------------------------------------------
// Periodic handler — called from main loop
// -----------------------------------------------------------------------
void menu_proc(void)
{
    static ulong menu_timer = 0;

    // Process any pending key event
    if(menu_pending != 0xFF)
    {
        uchar key = menu_pending;
        menu_pending = 0xFF;
        menu_process_key(key);
    }

    // Full repaint on menu change
    if(menu_dirty)
    {
        menu_dirty = 0;
        menu_draw();
    }

    // Periodic status update (every 1s)
    if(menu_timer == 0)
        menu_timer = sys_timer;
    else if((menu_timer + 1000) < sys_timer)
    {
        menu_timer = sys_timer;
        menu_draw_status();
    }
}
