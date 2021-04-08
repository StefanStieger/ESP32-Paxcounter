
#include "manual_time.h"
#include "reset.h"

enum Pages {
  page_manualTime_start,
  page_hour1,
  page_hour2,
  page_minute1,
  page_minute2
};
const char *monthlist[] = {
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




RTC_DATA_ATTR time_t base_timestamp = 0;

static tmElements_t tm;
static Pages displayPage = page_manualTime_start;
bool is_timeAskMode_var = false;





time_t manual_timeSync() {
  return base_timestamp + uptime() / 1000;
}

void set_timestamp(tmElements_t t) {
  time_t timestamp = myTZ.toUTC(makeTime(t));
  base_timestamp = timestamp;
  setTime(timestamp);
}

bool is_timeAskMode() {
  return is_timeAskMode_var;
}
void init_screen() {
  tm.Hour = TIME_SYNC_MANUAL_DEFAULT_H;
  tm.Minute = TIME_SYNC_MANUAL_DEFAULT_MIN;
  
  #if (TIME_SYNC_MANUAL_SKIP_TIMEOUT == 0)
    displayPage = page_minute2;
    nextPage();
  #else
    tm.Year = 50;
    displayPage = page_manualTime_start;
    is_timeAskMode_var = true;
  #endif
}

bool clickAction() {
  switch(displayPage) {
    case page_manualTime_start:
      displayPage = page_hour1;
      return true;
      break;
    case page_hour1:
      tm.Hour = (tm.Hour+10) % 30; //20 is still allowed.
      break;
    case page_hour2:
      ++tm.Hour;
      if(tm.Hour > 23)
        tm.Hour = 20;
      else if(tm.Hour % 10 == 0)
        tm.Hour -= 10;
      break;
    case page_minute1:
      tm.Minute = (tm.Minute+10) % 60;
      break;
    case page_minute2:
      ++tm.Minute;
      if(tm.Minute % 10 == 0)
        tm.Minute -= 10;
      break;
  }
  return false;
}

bool nextPage() {
  switch(displayPage) {
    case page_manualTime_start:
      displayPage = page_hour1;
    break;
    case page_hour1:
      displayPage = page_hour2;
    break;
    case page_hour2:
      displayPage = page_minute1;
    break;
    case page_minute1:
      displayPage = page_minute2;
    break;
    case page_minute2:
      tm.Second = 0;
      tm.Day = 1;
      tm.Month = 1;
      tm.Year = 30;
      set_timestamp(tm);
      sendTimer.attach(cfg.sendcycle * 2, setSendIRQ);
      is_timeAskMode_var = false;
      
      return false;
    break;
  }

  // screenUpdate.once_ms(500, draw_withManualTime);
  return true;
}

PageData getPageData() {
  PageData p;
  switch(displayPage) {
    case page_manualTime_start:
      p.desc = "Time needs to be set. Press button for selection and then wait "+String(TIME_SYNC_MANUAL_SKIP_TIMEOUT)+" seconds to confirm.";
      p.big = "Press btn";
      p.update = false;
      return p;
      break;
    case page_hour1:
      p.desc = "Select 1st digit of current hour:";
      p.big = tm.Hour;
      break;
    case page_hour2:
      p.desc = "Select 2nd digit of current hour:";
      p.big = String(tm.Hour);
      break;
    case page_minute1:
      p.desc = "Select 1st digit of current minute:";
      p.big = tm.Minute;
      break;
    case page_minute2:
      p.desc = "Select 2nd digit of current minute:";
      p.big = String(tm.Minute);
      break;
  }

  return p;
}