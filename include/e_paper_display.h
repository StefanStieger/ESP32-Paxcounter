#ifndef _E_PAPER_DISPLAY_H
#define _E_PAPER_DISPLAY_H


extern uint8_t E_paper_displayIsOn;

void ePaper_init(bool verbose);

bool react_toClick();
void nextManualPage();

void refresh_ePaperDisplay(bool nextPage = false);
void draw_page(bool nextPage = false);
void draw_main(time_t t);
void draw_withManualTime(bool wholePage);
void draw_manualTime_exec(String msg, String centerMsg);
void draw_manualTime_partly_exec(String centerMsg);
int calc_x_rightAlignment(String str, int16_t y);
int calc_x_centerAlignment(String str, int16_t y);
int calc_y_bottomAlignment(String str, int16_t x);

#endif