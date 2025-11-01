#include "screen_logger.h" // --- NEW --- For M5StickC screen logging

// --- This file holds all the Web Server and WebSocket logic ---

// ---------------- HANDLERS ----------------
void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlPage);
}


// --- WebSocket event handler (with Mode Switch Fix) ---
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      
      char* msg = (char*)data;
      msg[len] = '\0'; 
      
      StaticJsonDocument<512> doc; 
      DeserializationError error = deserializeJson(doc, msg);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      String type = doc["type"];

      if (type == "text_update") {
        currentMode = MODE_TEXT; 
        
        currentMsg = doc["msg"].as<String>();
        currentSpeed = doc["speed"].as<int>();
        currentBrightness = doc["brightness"].as<int>();
        currentEffect = getEffect(doc["effect"].as<int>());
        currentAlign = getAlignment(doc["align"].as<int>()); 
        currentInvert = doc["invert"].as<bool>();
        currentPause = doc["pause"].as<int>();
        currentSpacing = doc["spacing"].as<int>();
        currentScrollSpacing = doc["scrollSpacing"].as<int>();
        displayOn = doc["displayOn"].as<bool>();
        
        if (currentEffect == PA_PRINT) {
            currentEffectOut = PA_PRINT;
        } else {
            currentEffectOut = currentEffect; 
        }

        matrix.setIntensity(currentBrightness);
        matrix.setInvert(currentInvert);
        matrix.setCharSpacing(currentSpacing); 
        matrix.setScrollSpacing(currentScrollSpacing);
        matrix.displayShutdown(!displayOn); 
        
        matrix.displayClear();
        
        if (currentMsg != "") {
          matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffectOut);
        } else {
          matrix.displayReset(); 
        }

        M5.Lcd.fillRect(0, 100, M5.Lcd.width(), 140, BLACK); 
        M5.Lcd.setCursor(0, 100);
        M5.Lcd.setTextColor(MAGENTA);
        M5.Lcd.println("Msg Updated:");
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.println(currentMsg);
        logToScreen("Msg: " + currentMsg);

      } else if (type == "clear_display") {
        currentMsg = ""; 
        currentMode = MODE_TEXT; 
  // Hard reset display engine before switching
  matrix.displaySuspend(true);
  matrix.displayClear();
  matrix.displayReset();
        logToScreen("Display Cleared!");

      // --- CHANGED --- Added Snake Mode
      } else if (type == "set_mode") {
        String modeStr = doc["mode"].as<String>();
        
        matrix.displayClear();
        matrix.displayReset();
        
        if (modeStr == "graphics") { 
          currentMode = MODE_GRAPHICS;
          renderPixelBufferToMatrix(); // Draw pixels
          matrix.displaySuspend(false); // UNPAUSE engine
        }
        else if (modeStr == "snake") { // --- NEW ---
          currentMode = MODE_SNAKE;
          matrix.displaySuspend(false); // UNPAUSE engine
          resetGame(); // Start the game
        }
        else { // Default to text
          currentMode = MODE_TEXT;
          matrix.displaySuspend(false); // UNPAUSE engine
          if (currentMsg != "") { 
            matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffectOut);
          }
        }
        
        M5.Lcd.fillRect(0, 100, M5.Lcd.width(), 140, BLACK);
        M5.Lcd.setCursor(0, 100);
        M5.Lcd.setTextColor(CYAN);
        M5.Lcd.printf("Mode: %s\n", modeStr.c_str());
        logToScreen("Mode: " + modeStr);

      } else if (type == "pixel_update") { 
        int x_web = doc["x"].as<int>();
        int y_web = doc["y"].as<int>();
        bool state = doc["state"].as<bool>();
        
        int x_hw = (MATRIX_WIDTH - 1) - x_web; 
        int y_hw = y_web; 
        
        if (x_web >= 0 && x_web < MATRIX_WIDTH && y_web >= 0 && y_web < MATRIX_HEIGHT) {
          pixelBuffer[x_web][y_web] = state; 
          if (currentMode == MODE_GRAPHICS) {
             matrix.displaySuspend(true);
             matrix.getGraphicObject()->setPoint(y_hw, x_hw, state);
             matrix.displaySuspend(false);
          }
        }

      } else if (type == "graphics_data") { 
        currentMode = MODE_GRAPHICS;
        initializePixelBuffer(); 
        JsonArray pixelsArray = doc["pixels"].as<JsonArray>();
        for (JsonObject pixel : pixelsArray) {
          int x = pixel["x"].as<int>();
          int y = pixel["y"].as<int>();
          if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
            pixelBuffer[x][y] = true;
          }
        }
        matrix.displaySuspend(true);
        renderPixelBufferToMatrix(); 
        matrix.displaySuspend(false);

      } else if (type == "clear_graphics") { 
        initializePixelBuffer();
        if (currentMode == MODE_GRAPHICS) {
          matrix.displaySuspend(true);
          matrix.displayClear();
          matrix.displaySuspend(false);
        }
      } else if (type == "reboot") {
        logToScreen("Rebooting...");
        delay(1000);
        ESP.restart();
      }
    }
  } else if(type == WS_EVT_ERROR){
    Serial.printf("WS Error[%u]: %s\n", client->id(), *(uint16_t*)arg ? "client error" : "server error");
  }
}