// --- This file holds helper functions for effects, alignment, and graphics ---
#include "snake_game.h" // --- NEW --- Include header for globals

// --- Full effect list
textEffect_t getEffect(int i) {
  switch (i) {
    case 0: return PA_SCROLL_LEFT;
    case 1: return PA_NO_EFFECT;
    case 2: return PA_PRINT;
    case 3: return PA_SCROLL_UP;
    case 4: return PA_SCROLL_DOWN;
    case 5: return PA_SCROLL_RIGHT;
    case 6: return PA_SPRITE;
    case 7: return PA_SLICE;
    case 8: return PA_MESH;
    case 9: return PA_FADE;
    case 10: return PA_DISSOLVE;
    case 11: return PA_BLINDS;
    case 12: return PA_RANDOM;
    case 13: return PA_WIPE;
    case 14: return PA_WIPE_CURSOR;
    case 15: return PA_SCAN_HORIZ;
    case 16: return PA_SCAN_HORIZX;
    case 17: return PA_SCAN_VERT;
    case 18: return PA_SCAN_VERTX;
    case 19: return PA_OPENING;
    case 20: return PA_OPENING_CURSOR;
    case 21: return PA_CLOSING;
    case 22: return PA_CLOSING_CURSOR;
    case 23: return PA_SCROLL_UP_LEFT;
    case 24: return PA_SCROLL_UP_RIGHT;
    case 25: return PA_SCROLL_DOWN_LEFT;
    case 26: return PA_SCROLL_DOWN_RIGHT;
    case 27: return PA_GROW_UP;
    case 28: return PA_GROW_DOWN;
    default: return PA_SCROLL_LEFT;
  }
}

// Helper function for text alignment
textPosition_t getAlignment(int i) {
  switch (i) {
    case 0: return PA_RIGHT;  // User "Left" -> Hardware "Right"
    case 1: return PA_CENTER; // Center is Center
    case 2: return PA_LEFT;   // User "Right" -> Hardware "Left"
    default: return PA_RIGHT;
  }
}

// Renders the pixel buffer to the matrix (with X-axis flip)
void renderPixelBufferToMatrix() {
  matrix.displayClear();
  
  for (int x_web = 0; x_web < MATRIX_WIDTH; x_web++) {
    for (int y_web = 0; y_web < MATRIX_HEIGHT; y_web++) {
      if (pixelBuffer[x_web][y_web]) { // Read from buffer at web coordinates
        int x_hw = (MATRIX_WIDTH - 1) - x_web; // Flip X-axis
        int y_hw = y_web; 
        matrix.getGraphicObject()->setPoint(y_hw, x_hw, true); // Write to HW coordinates
      }
    }
  }
}

// Clears the pixel buffer
void initializePixelBuffer() {
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      pixelBuffer[x][y] = false;
    }
  }
}