#pragma once // Prevents file from being included multiple times

// Define the Direction enum here so all files can see it
enum Direction { 
  DIR_UP, 
  DIR_DOWN, 
  DIR_LEFT, 
  DIR_RIGHT 
};

// Declare all our global game variables and functions
extern bool isGameOver;

void resetGame();
void snakeGameLoop();
void setSnakeDirection(Direction newDir);
extern void renderPixelBufferToMatrix();
extern textPosition_t getAlignment(int i);
extern textEffect_t getEffect(int i);