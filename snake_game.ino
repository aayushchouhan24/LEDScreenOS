// --- Holds all the logic for the Snake game ---
#include "snake_game.h"
#include "screen_logger.h" // --- NEW --- For M5StickC screen logging

// Game settings
#define GAME_SPEED 150 // Milliseconds between snake moves

// Game variables
struct Point { int x; int y; };
Point snakeBody[MATRIX_WIDTH * MATRIX_HEIGHT]; // Max possible snake length
int snakeLength;
Point food;
Direction currentDirection;
bool isGameOver;
unsigned long lastMoveTime;

// --- Game Logic ---

void resetGame() {
  matrix.displayClear();
  snakeLength = 3;
  // Start snake in the middle, moving right
  snakeBody[0] = {MATRIX_WIDTH / 2, MATRIX_HEIGHT / 2};
  snakeBody[1] = {MATRIX_WIDTH / 2 - 1, MATRIX_HEIGHT / 2};
  snakeBody[2] = {MATRIX_WIDTH / 2 - 2, MATRIX_HEIGHT / 2};
  currentDirection = DIR_RIGHT;
  isGameOver = false;
  spawnFood();
  logToScreen("SNAKE GAME!");
  // Draw first frame immediately to avoid residual text/graphics
  drawSnake();
  matrix.displayAnimate();
}

void spawnFood() {
  bool onSnake;
  do {
    onSnake = false;
    food.x = random(0, MATRIX_WIDTH);
    food.y = random(0, MATRIX_HEIGHT);
    // Check if food spawned on the snake
    for (int i = 0; i < snakeLength; i++) {
      if (food.x == snakeBody[i].x && food.y == snakeBody[i].y) {
        onSnake = true;
        break;
      }
    }
  } while (onSnake);
}

void drawSnake() {
  matrix.displayClear();
  // Draw food (with X-axis flip)
  int foodX_hw = (MATRIX_WIDTH - 1) - food.x;
  matrix.getGraphicObject()->setPoint(food.y, foodX_hw, true);

  // Draw snake (with X-axis flip)
  for (int i = 0; i < snakeLength; i++) {
    int x_hw = (MATRIX_WIDTH - 1) - snakeBody[i].x; // Flip X-axis
    int y_hw = snakeBody[i].y;
    matrix.getGraphicObject()->setPoint(y_hw, x_hw, true);
  }
}

void setSnakeDirection(Direction newDir) {
  // Prevent snake from reversing on itself
  if (newDir == DIR_UP && currentDirection != DIR_DOWN) currentDirection = newDir;
  if (newDir == DIR_DOWN && currentDirection != DIR_UP) currentDirection = newDir;
  if (newDir == DIR_LEFT && currentDirection != DIR_RIGHT) currentDirection = newDir;
  if (newDir == DIR_RIGHT && currentDirection != DIR_LEFT) currentDirection = newDir;
}

void moveSnake() {
  // Create new head based on direction
  Point newHead = snakeBody[0];
  switch (currentDirection) {
    case DIR_UP: newHead.y--; break;
    case DIR_DOWN: newHead.y++; break;
    case DIR_LEFT: newHead.x--; break;
    case DIR_RIGHT: newHead.x++; break;
  }

  // --- Check for Collisions ---
  // 1. Wall collision
  if (newHead.x < 0 || newHead.x >= MATRIX_WIDTH || newHead.y < 0 || newHead.y >= MATRIX_HEIGHT) {
    isGameOver = true;
    logToScreen("Game Over!");
    return;
  }
  // 2. Self collision
  for (int i = 0; i < snakeLength; i++) {
    if (newHead.x == snakeBody[i].x && newHead.y == snakeBody[i].y) {
      isGameOver = true;
      logToScreen("Game Over!");
      return;
    }
  }

  // --- Move Snake ---
  // Shift all segments down
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeBody[i] = snakeBody[i - 1];
  }
  snakeBody[0] = newHead; // Add new head

  // --- Check for Food ---
  if (newHead.x == food.x && newHead.y == food.y) {
    snakeLength++; // Grow snake
    logToScreen("Score: " + String(snakeLength - 3));
    spawnFood(); // Spawn new food
  }
}

void snakeGameLoop() {
  if (isGameOver) {
    // On game over, a button press will restart via the BLE handler
    return; 
  }

  // Main game timer
  if (millis() - lastMoveTime > GAME_SPEED) {
    lastMoveTime = millis();
    moveSnake();
    if (!isGameOver) {
      drawSnake();
      matrix.displayAnimate(); // Update the display only when the snake moves
    }
  }
}