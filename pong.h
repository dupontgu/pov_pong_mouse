#define RENDER_SCORE_FRAMES_MAX 80

typedef struct
{
  float x, y;
  float velX, velY;
} Ball;

typedef struct
{
  float x, y;
  float velY;
  int score;
} Paddle;

typedef struct
{
  Ball ball;
  Paddle lPaddle;
  Paddle rPaddle;
  float maxYVel;
  int screenHeight;
  int screenWidth;
  int ballSize;
  int paddleHeight;
  int paddleWidth;
  int halfPaddleHeight;
  int renderScoreCounter;
  uint8_t subFrame;
} Game;

typedef struct
{
  float x, y;
} Frame;

void initGame(Game *gameHolder, float maxBallVelocity, float screenHeight, float screenWidth, int ballSize, int paddleHeight, int paddleWidth) {
  Paddle left = { 10, (screenHeight - paddleHeight) / 2, 0, 0 };
  gameHolder->lPaddle = left;
  Paddle right = { screenWidth - 20, (screenHeight - paddleHeight) / 2, 0, 0 };
  gameHolder->rPaddle = right;
  // TODO - pass in ball x velocity as argument
  Ball ball = { .x = screenWidth / 2, .y = screenHeight / 2, .velX = -5, .velY = 1.6 };
  gameHolder->ball = ball;
  gameHolder->maxYVel = maxBallVelocity;
  gameHolder->screenHeight = screenHeight;
  gameHolder->screenWidth = screenWidth;
  gameHolder->ballSize = ballSize;
  gameHolder->paddleHeight = paddleHeight;
  gameHolder->paddleWidth = paddleWidth;
  gameHolder->halfPaddleHeight = paddleHeight / 2;
  gameHolder->renderScoreCounter = 0;
}

int distFromPaddleCenter(Game *game, Ball *ball, Paddle *paddle) {
  return ball->y - (paddle->y + game->halfPaddleHeight);
}

float yVelocityFromPaddleIntersect(Game *game, int distFromPaddleCenter) {
  return ((float)distFromPaddleCenter / (float)game->halfPaddleHeight) * game->maxYVel;
}

void movePaddle(Game *game, Paddle *paddle) {
  paddle->y += paddle->velY;

  // Keep paddle within screen bounds
  if (paddle->y < 0) {
    paddle->y = 0;
  } else if (paddle->y > game->screenHeight - game->paddleHeight) {
    paddle->y = game->screenHeight - game->paddleHeight;
  }
}

void moveBall(Game *game) {
  Ball *ball = &(game->ball);
  Paddle *leftPaddle = &(game->lPaddle);
  Paddle *rightPaddle = &(game->rPaddle);
  ball->x += ball->velX;
  ball->y += ball->velY;

  // Bounce off top and bottom walls
  if (ball->y <= 0 || ball->y >= game->screenHeight - game->ballSize) {
    ball->velY = -ball->velY;
  }

  int leftPaddleYDist = distFromPaddleCenter(game, ball, leftPaddle);
  int rightPaddleYDist = distFromPaddleCenter(game, ball, rightPaddle);

  // Bounce off paddles
  if (ball->x <= leftPaddle->x + game->paddleWidth && ball->y + game->ballSize >= leftPaddle->y && abs(leftPaddleYDist) < game->halfPaddleHeight) {
    ball->velX = -ball->velX;
    ball->x = leftPaddle->x + game->paddleWidth;
    ball->velY = yVelocityFromPaddleIntersect(game, leftPaddleYDist);
  } else if (ball->x + game->ballSize >= rightPaddle->x && ball->y + game->ballSize >= rightPaddle->y && abs(rightPaddleYDist) < game->halfPaddleHeight) {
    ball->velX = -ball->velX;
    ball->x = rightPaddle->x - game->ballSize;
    ball->velY = yVelocityFromPaddleIntersect(game, rightPaddleYDist);
  }

  bool pointScored = false;
  // Reset ball if it goes past the paddles
  if (ball->x < 0) {
    pointScored = true;
    game->rPaddle.score++;
    Serial.println("r score");
  } else if (ball->x > game->screenWidth) {
    pointScored = true;
    game->lPaddle.score++;
    Serial.println("l score");
  }

  if (pointScored) {
    game->renderScoreCounter = RENDER_SCORE_FRAMES_MAX;
    ball->x = game->screenWidth / 2;
    ball->y = game->screenHeight / 2;
    ball->velX = -ball->velX;
    ball->velY = 1;
  }
}

// quick an dirty AI, can be used with either/both paddles
// call with the desired paddle, then pass the returned velocity value back in with the next "tick"
float paddleAutoPilot(Game *game, Paddle *paddle, float maxVelocity, int randomSeed) {
  float guessVelocity = 0.0;
  int scoreSeed = game->lPaddle.score + game->rPaddle.score + randomSeed;
  bool targetOffsetNegative = (scoreSeed % 2) == 1;
  int targetOffset = ((scoreSeed + 1) * 7) % game->halfPaddleHeight;
  if (targetOffsetNegative) {
    targetOffset = -targetOffset;
  }
  float yDistFromBall = game->ball.y - (paddle->y + game->halfPaddleHeight + targetOffset);
  if (yDistFromBall > maxVelocity) {
    guessVelocity = maxVelocity;
  } else if (yDistFromBall < -maxVelocity) {
    guessVelocity = -maxVelocity;
  }
  return guessVelocity;
}

void tick(Game *game, float leftPaddleVel, float rightPaddleVel) {
  if (game->renderScoreCounter > 0) {
    game->renderScoreCounter -= 1;
  } else {
    game->lPaddle.velY = leftPaddleVel;
    game->rPaddle.velY = rightPaddleVel;
    movePaddle(game, &(game->lPaddle));
    movePaddle(game, &(game->rPaddle));
    moveBall(game);
  }
}

Frame nextSubframe(Game *game) {
  Frame frame;
  if (game->renderScoreCounter > 0) {
    // score is rendered by showing the mouse cursor in three spots
    // outer spots show the bounds, inner spot indicates which player is winning
    // the closer the inner spot is to either edge indicates how much that player is up by
    // when the game is tied, the inner spot should sit right in the middle
    float pointRatio = (float)(game->rPaddle.score + 1) / (float)(game->lPaddle.score + 1);
    if (pointRatio < 1.0) {
      pointRatio /= 2.0;
    } else {
      pointRatio = 1.0 - ((1.0 / pointRatio) / 2.0);
    }
    switch (game->subFrame++) {
      case 0:
        frame.x = 0;
        frame.y = 0.5;
        break;
      case 1:
        frame.x = pointRatio;
        frame.y = 0.5;
        break;
      case 2:
        frame.x = 1;
        frame.y = 0.5;
      default:
        game->subFrame = 0;
    }
  } else {
    switch (game->subFrame++) {
      case 0:
        frame.x = game->lPaddle.x / (float)game->screenWidth;
        frame.y = (game->lPaddle.y + game->halfPaddleHeight) / (float)game->screenHeight;
        break;
      case 1:
        frame.x = game->ball.x / (float)game->screenWidth;
        frame.y = game->ball.y / (float)game->screenHeight;
        break;
      case 2:
        frame.x = game->rPaddle.x / (float)game->screenWidth;
        frame.y = (game->rPaddle.y + game->halfPaddleHeight) / (float)game->screenHeight;
      default:
        game->subFrame = 0;
    }
  }
  return frame;
}
