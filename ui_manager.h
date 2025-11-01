#pragma once
#include <Arduino.h>

// App display modes (shared across files)
enum DisplayMode {
  MODE_TEXT,
  MODE_GRAPHICS,
  MODE_SNAKE
};

// UI States
enum UIState {
  UI_HOME,
  UI_APPS,
  UI_POWER_MENU,
  UI_APP_RUNNING,
  UI_SETTINGS,
  UI_TEXT_SETTINGS
};

// App IDs
enum AppID {
  APP_SNAKE,
  APP_TEXT,
  APP_GRAPHICS,
  APP_SETTINGS
};

// Global UI state
extern UIState currentUIState;
extern int selectedAppIndex;
extern int selectedPowerOption;
extern int selectedSettingIndex;
extern int selectedTextSettingIndex;
// Global current app/matrix mode (defined in LedScreen.ino)
extern DisplayMode currentMode;

// UI Functions
void initUI();
void updateUI();
void drawStatusBar();
void drawHomeScreen();
void drawAppsMenu();
void drawPowerMenu();
void drawSettingsMenu();
void drawTextSettingsMenu();

// Navigation
void uiNavigateUp();
void uiNavigateDown();
void uiNavigateLeft();
void uiNavigateRight();
void uiSelect();
void uiBack();
void uiIncrease();
void uiDecrease();
void uiHome();
void uiMenu();
void openTextSettings();

// Status updates
void setWiFiConnected(bool connected);
void setBTConnected(bool connected);
void updateBatteryLevel();
