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

// white needs to be defined
#define GxEPD_WHITE 0xFFFF

// HEX code of WLAN bitmap

const unsigned char wifi_bitmap [] PROGMEM = {
	// 'wifi-32, 32x32px
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x0f, 0xf0, 0x00, 0x00, 0xff, 0xff, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xe0, 
	0x1f, 0xf8, 0x1f, 0xf8, 0x3f, 0xc0, 0x03, 0xfc, 0x7e, 0x1f, 0xf8, 0xfe, 0xfc, 0xff, 0xff, 0x3f, 
	0xf9, 0xff, 0xff, 0x9f, 0x63, 0xff, 0xff, 0xe6, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xc7, 0xe3, 0xf0, 
	0x0f, 0x3f, 0xfc, 0xf0, 0x06, 0x7f, 0xfe, 0x60, 0x00, 0xff, 0xff, 0x00, 0x01, 0xfc, 0x3f, 0x80, 
	0x00, 0xf7, 0xef, 0x00, 0x00, 0xcf, 0xf3, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x1e, 0x78, 0x00, 
	0x00, 0x0f, 0xf0, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x01, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char ble_bitmap [] PROGMEM = {
	// 'bluetooth-64, 32x32px
	0x00, 0x0f, 0xf0, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0xff, 0xff, 0x00, 0x01, 0xfe, 0xff, 0x80, 
	0x01, 0xfe, 0x7f, 0x80, 0x03, 0xfe, 0x3f, 0xc0, 0x03, 0xfe, 0x1f, 0xe0, 0x07, 0xfe, 0x0f, 0xe0, 
	0x07, 0xfe, 0x07, 0xe0, 0x07, 0xde, 0x63, 0xe0, 0x07, 0x8e, 0x71, 0xf0, 0x07, 0xc6, 0x63, 0xf0, 
	0x0f, 0xe0, 0x07, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf8, 0x1f, 0xf0, 0x0f, 0xfc, 0x1f, 0xf0, 
	0x0f, 0xfc, 0x3f, 0xf0, 0x0f, 0xf8, 0x1f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xe0, 0x07, 0xf0, 
	0x07, 0xc6, 0x63, 0xf0, 0x07, 0x8e, 0x71, 0xf0, 0x07, 0xde, 0x63, 0xe0, 0x07, 0xfe, 0x07, 0xe0, 
	0x07, 0xfe, 0x0f, 0xe0, 0x03, 0xfe, 0x1f, 0xe0, 0x03, 0xfe, 0x3f, 0xc0, 0x01, 0xfe, 0x7f, 0x80, 
	0x01, 0xfe, 0xff, 0x80, 0x00, 0xff, 0xff, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x0f, 0xf0, 0x00
};

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
bool clickTimer_active = false;


RTC_DATA_ATTR time_t statisticsPointer = 0;
RTC_DATA_ATTR int16_t statistics[STATISTICS_MAX_ENTRIES];
RTC_DATA_ATTR int16_t statistics_lastValue = 0;


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
  else {
    int sum = macs_wifi + macs_ble;
    statistics[statisticsPointer] = sum - statistics_lastValue;
    statistics_lastValue = sum;
    statisticsPointer = (statisticsPointer+1)%100;
  }
  
  draw_page();
  // display.hibernate();
}

bool react_toClick() {
  if(is_timeAskMode()) {
    bool reloadPage = clickAction();
    if(!clickTimer_active) {
      clickTimer_active = true;
      screenUpdate.detach();
      screenUpdate.once_ms(100, draw_withManualTime, reloadPage);
    }
    
    
    // draw_withManualTime(reloadPage);
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
    #if (defined BAT_MEASURE_ADC || defined HAS_PMU || defined HAS_IP5306)
      if(batt_level < BATTERY_LEVEL_LOW_TRIGGER) {
        display.setTextSize(2);
        display.setCursor(calc_x_centerAlignment("Please recharge", 20), 20);
        display.println("Please recharge");

        display.setTextSize(2);
        String battery_text = batt_level == 0 ? "No battery" : String("Battery: ")+batt_level + "%";
        display.setCursor(0, calc_y_bottomAlignment(battery_text, 0));
        display.println(battery_text);
        // draw line at the bottom
        display.drawLine(0, display.height() - 15, display.width(), display.height() - 15, GxEPD_BLACK);
        continue;
      }
    #endif
    
    // display wifi bitmap
    #if(WIFICOUNTER == 1)
      display.drawBitmap(10, 10, wifi_bitmap, 32, 32, GxEPD_BLACK);
      display.setCursor(50, 20);
      display.setTextSize(2.5);
     display.println(macs_wifi);
    #endif
    
    // display ble bitmap
    #if(BLECOUNTER == 1)
      display.drawBitmap(10, 50, ble_bitmap, 32, 32, GxEPD_BLACK);
      display.setCursor(50, 60);
      display.println(macs_ble);
    #endif

    // display time - button right corner
    display.setTextSize(1.5);
    String time_text = String("Referencetime: ") + time_string;
    display.setCursor(calc_x_rightAlignment(time_text, 10), calc_y_bottomAlignment(time_text, 0));
    display.println(time_text);

    // display y-axis label - COUNT
    display.setTextSize(1.5);
    display.setCursor((display.width() / 2) + STATISTICS_MAX_ENTRIES - 15, 8);
    display.println(String("Count"));    

    // display x-axis label - TIME
    display.setTextSize(1.5);
    display.setCursor((display.width() / 2) - 25, display.height() - 30);
    display.println(String("Time"));      

    // draw vertical line in the middle between battery status and time
    display.drawFastVLine(display.width() / 2, display.height() - 15, display.height(), GxEPD_BLACK);

    // draw x- and y-axes
    // y-line
    display.drawLine((display.width() / 2) + STATISTICS_MAX_ENTRIES, display.height() - 25, (display.width() / 2) + STATISTICS_MAX_ENTRIES, 20, GxEPD_BLACK);
    // x-line (short)
    // display.drawLine((display.width() / 2) - 15, display.height() - 15, (display.width() / 2), display.height() - 15, GxEPD_BLACK);

    // draw barchart of counts as feedback for participants
    int startX = (display.width() / 2);
    int startY = display.height() - 25;
    for(int8_t i=0; i<STATISTICS_MAX_ENTRIES; ++i) {
      int x = startX + i;
      display.drawLine(x, startY, x, startY - (statistics[(statisticsPointer+i) % STATISTICS_MAX_ENTRIES] * GRAPH_SCALE_FACTOR), GxEPD_BLACK);
    }
    
    // display battery status
    #if (defined BAT_MEASURE_ADC || defined HAS_PMU || defined HAS_IP5306)
      display.setTextSize(1.5);
      String battery_text = batt_level == 0 ? "No battery" : String("Battery: ")+batt_level + "%";
      display.setCursor(5, calc_y_bottomAlignment(battery_text, 0));
      display.println(battery_text);

      // draw line at the bottom
      display.drawLine(0, display.height() - 15, display.width(), display.height() - 15, GxEPD_BLACK);
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
    screenUpdate.once(TIME_SYNC_MANUAL_SKIP_TIMEOUT, nextManualPage);
  clickTimer_active = false;
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