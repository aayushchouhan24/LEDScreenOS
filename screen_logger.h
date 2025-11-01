#pragma once
#include <Arduino.h>

void initScreenLogger();
void logToScreen(const String& message);
void handleScreenScroll();
// Programmatic scroll via controller
void screenScrollUp();
void screenScrollDown();
