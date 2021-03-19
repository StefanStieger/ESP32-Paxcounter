#ifndef _MANUAL_TIME_H
#define _MANUAL_TIME_H

#include "globals.h"

struct PageData {
  String desc;
  String big;
  bool update = true;
};

time_t manual_timeSync();
void set_timestamp(tmElements_t t);


void init_screen();
bool is_timeAskMode(); 
bool clickAction();
bool nextPage();
PageData getPageData();

#endif