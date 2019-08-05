#ifndef GAME_H
#define GAME_H

#define MAX_PLAYERS 2
#define AI_LEVEL_NONE -1
#define AI_LEVEL_EASY 0
#define AI_LEVEL_NORMAL 1
#define AI_LEVEL_HARD 2

typedef struct ball_object
{
	float x;
	float y;
	float dx; //Velocity X
	float dy; //Velocity Y
	float size;
} BallObject;

typedef struct paddle_object
{
	float x;
	float y;
	float w;
	float h;
	int ai_level;
} PaddleObject;

typedef struct game_setup
{
	int players;
	int win_score;
	int ai_level;
	int draw_separator;
} GameSetup;

extern GameSetup game_setup;
void GameInit(int unused);
void GameCallback();
void GameDraw();
void DrawSeparator();
void DrawScores();
void DrawScore(float x, float y, long long score, bool right_align);
void DrawScoreDigit(float x, float y, int digit);
void DrawPaddles();
void DrawBall();
void DrawPauseScreen();
void GameUpdate();
bool CheckBallPaddleCollide(BallObject *ball, PaddleObject *paddle);
void UpdatePaddles();
void UpdatePlayerPaddle(int paddle_idx);
void UpdateAiPaddle(int paddle_idx);
void ResetField();
void UpdateBall();

int scores[MAX_PLAYERS];

#endif