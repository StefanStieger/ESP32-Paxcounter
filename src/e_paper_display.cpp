#ifdef HAS_E_PAPER_DISPLAY

#include "globals.h"
#include "manual_time.h"

#if (TIME_SYNC_MANUAL)
  #include "reset.h"
#endif
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_7C.h>
#include <e_paper_choose_board.h>
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes

// local Tag for logging
static const char TAG[] = __FILE__;

const char *printmonth[] = {
  "xxx",
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "May",
  "Jun",
  "Jul",
  "Aug",
  "Sep",
  "Oct",
  "Nov",
  "Dec"
};

uint8_t E_paper_displayIsOn = 0;
Ticker screenUpdate;

void ePaper_init(bool verbose) {
  display.init();
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);

  if (verbose) {

    // show chip information
    #if(VERBOSE == 1)
      esp_chip_info_t chip_info;
      esp_chip_info(&chip_info);

      display.firstPage();
      do {
        display.setCursor(0, 10);
        display.println(String("Software v") + PROGVERSION);
        display.setCursor(0, 20);
        display.println(String("ESP32 ") + chip_info.cores + " cores");
        display.setCursor(0, 30);
        display.println(String("Chip Rev.") + chip_info.revision);
        display.setCursor(0, 40);
        display.println(
          String("WiFi") + 
          ((chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "") +
          ((chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "")
        );
        display.setCursor(0, 50);
        display.println(
          String(spi_flash_get_chip_size() / (1024 * 1024)) + 
          "Mb " +
          ((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.") +
          " Flash"
        );
      }
      while (display.nextPage());

      delay(3000);
    #endif // VERBOSE

    #if(TIME_SYNC_MANUAL == 1) 
      init_screen();
    #endif // TIME_SYNC_MANUAL
  }
  
  draw_page();
  // display.hibernate();
}

bool react_toClick() {
  // screenUpdate.detach();
  if(is_timeAskMode()) {
    bool reloadPage = clickAction();
    // screenUpdate.once_ms(200, draw_withManualTime, reloadPage);
    draw_withManualTime(reloadPage);
  }
  else 
    draw_page();
    // screenUpdate.once_ms(500, draw_page, true);

  return true;
}

void nextManualPage() {
  if(nextPage())
    draw_withManualTime(true);
  else
    draw_page();
}
void refresh_ePaperDisplay(bool nextPage) {
  draw_page(nextPage);
}


void draw_main(time_t t) {
  char time_string [5];
  sprintf(time_string, "%02d:%02d", hour(t), minute(t));

  display.setFullWindow();
  display.firstPage();
  do {
    display.setCursor(0, 10);
    display.setTextSize(3);
    display.println(macs.size());

    display.setTextSize(2);

    display.setCursor(calc_x_rightAlignment(time_string, 10), 10);
    display.println(time_string);


    #if (defined BAT_MEASURE_ADC || defined HAS_PMU || defined HAS_IP5306)
      display.setTextSize(1);
      String battery_text = batt_level == 0 ? "No battery" : String("Battery: ")+batt_level + "%";
      display.setCursor(0, calc_y_bottomAlignment(battery_text, 0));
      display.println(battery_text);
  #endif
  }
  while (display.nextPage());
}

void draw_withManualTime(bool wholePage) {
  screenUpdate.detach();
  PageData p = getPageData();

  if(wholePage)
    draw_manualTime_exec(p.desc, p.big);
  else
    draw_manualTime_partly_exec(p.big);

  if(p.update)
    screenUpdate.once(5, nextManualPage);
}

void draw_manualTime_exec(String msg, String centerMsg) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.println(msg);
    display.setTextSize(3);
    display.setCursor(calc_x_centerAlignment(centerMsg, 40), 40);
    display.println(centerMsg);
  } while (display.nextPage());
}
void draw_manualTime_partly_exec(String centerMsg) {
  display.setPartialWindow(0, 40, display.width(), 20);
  display.firstPage();
  do {
    display.setCursor(calc_x_centerAlignment(centerMsg, 40), 40);
    display.println(centerMsg);
  } while (display.nextPage());
}

void draw_page(bool nextPage) {
  #if(TIME_SYNC_MANUAL == 1)
    if(is_timeAskMode())
      draw_withManualTime(true);
    else
      draw_main(myTZ.toLocal(now()));
  #else
    draw_main(myTZ.toLocal(now()));
  #endif //TIME_SYNC_MANUAL
}

int calc_x_rightAlignment(String str, int16_t y) {
  int16_t x = 0;
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, y, &x1, &y1, &w, &h);
  
  return display.width() - w - x1;
}

int calc_y_bottomAlignment(String str, int16_t x) {
  int16_t y = 0;
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, y, &x1, &y1, &w, &h);
  
  return display.height() - h - y1;
}

int calc_x_centerAlignment(String str, int16_t y) {
  int16_t x = 0;
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, y, &x1, &y1, &w, &h);
  
  return (display.width() - w) / 2;
}
#endif