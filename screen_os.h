#pragma once
#include <Arduino.h>

// Simple OS views
enum OSView {
  VIEW_HOME,
  VIEW_APPS,
  VIEW_POWER,
  VIEW_APP_DRAW,
  VIEW_APP_TEXT,
  VIEW_APP_SNAKE,
  VIEW_APP_CLOCK,
  VIEW_APP_YT
};

void initOS();
void osLoop();

// Controller event hooks
void osOnDpad(uint8_t dirCode); // 0x00 up, 0x44 down, 0x66 left, 0x22 right
void osOnButtonX();
void osOnButtonB();
void osOnButtonY();
void osOnMenu();
void osOnHome();
