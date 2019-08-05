#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include "statedefs.h"
#include "print.h"
#include "gfxmgr.h"
#include "alignedalloc.h"
#include "game.h"
#include "gamegfx_tpl.h"
#include "gamegfx.h"

#define PADDLE_WIDTH 16
#define PADDLE_HEIGHT 80
#define PADDLE_SPEED 3.5
#define AI_SPEED_FACTOR 1.1
#define PADDLE_X_OFFSET 16
#define SCORE_X_OFFSET 32
#define SCORE_Y 16
#define SCORE_SIZE 2.0
#define BALL_MAX_SPEED 5.0
#define PLAY_FIELD_MIN_Y 16
#define PLAY_FIELD_MAX_Y (SCREEN_HEIGHT-16)
#define BALL_SIZE 16
#define HIT_Y_SPEED_SCALE ((2*BALL_MAX_SPEED)/PADDLE_HEIGHT)
#define SEPARATOR_WIDTH 6
#define SEPARATOR_SLICE_HEIGHT 30
#define SEPARATOR_GAP_HEIGHT 15
#define BALL_WAIT_TIME 30
#define PAUSE_WAIT_TIME 12
#define NUM_DIGITS 10
#define DIGIT_WIDTH 19
#define NUM_BALL_VERTICES 32
#define FADE_SPEED 15
#define FADE_IN 0
#define FADE_OUT 1
#define FADE_NONE -1

static PaddleObject paddles[MAX_PLAYERS];
static BallObject ball;
static int wait_timer;
static int pause_wait_timer;
static float ai_speeds[3] = { 0.85, 0.95, 1.1 };
static bool paused;
static int paused_player;
static GXTexObj digits_tex_obj;
static void *game_gfx;
static int brightness;
static int fade_dir;

void GameInit(int unused)
{
	int i;
	TPLFile tpl;
	game_gfx = malloc_aligned(32, gamegfx_tpl_size);
	memcpy(game_gfx, gamegfx_tpl, gamegfx_tpl_size);
	TPL_OpenTPLFromMemory(&tpl, (void *)game_gfx, gamegfx_tpl_size);
	TPL_GetTexture(&tpl, scoredigits, &digits_tex_obj);
	GX_InitTexObjFilterMode(&digits_tex_obj, GX_LINEAR, GX_LINEAR);
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(i == 1)
		{
			paddles[i].x = SCREEN_WIDTH-PADDLE_WIDTH-PADDLE_X_OFFSET;
		}
		else
		{
			paddles[i].x = PADDLE_X_OFFSET;
		}
		paddles[i].w = PADDLE_WIDTH;
		paddles[i].h = PADDLE_HEIGHT;
		paddles[i].ai_level = AI_LEVEL_NONE;
		scores[i] = 0;
		if(i >= game_setup.players)
		{
			paddles[i].ai_level = game_setup.ai_level;
		}
	}
	ResetField();
	paused = false;
	pause_wait_timer = 0;
	brightness = 0;
	fade_dir = FADE_IN;
}

void GameCallback()
{
	GameUpdate();
	GameDraw();
}

void GameDraw()
{
	if(!paused)
	{
		DrawSeparator();
		DrawPaddles();
		DrawBall();
		DrawScores();
	}
	else
	{
		DrawPauseScreen();
	}
}

void DrawSeparator()
{
	if(!game_setup.draw_separator)
	{
		return;
	}
	float i;
	SetPrintColor(255, 255, 255, brightness);
	for(i=(SEPARATOR_GAP_HEIGHT/2.0); i<SCREEN_HEIGHT; i+=SEPARATOR_SLICE_HEIGHT)
	{
		PrintRectangle(HALF_SCREEN_WIDTH-(SEPARATOR_WIDTH/2), i, SEPARATOR_WIDTH, SEPARATOR_SLICE_HEIGHT-SEPARATOR_GAP_HEIGHT);
	}
}

void DrawScores()
{
	DrawScore(SCORE_X_OFFSET, SCORE_Y, scores[0], false);
	DrawScore(SCREEN_WIDTH-SCORE_X_OFFSET-DIGIT_WIDTH, SCORE_Y, scores[1], true);
}

void DrawScore(float x, float y, long long score, bool right_align)
{
	int i;
	long long digit_divisor = 1;
	for(i=0; i<10; i++)
	{
		int digit = (score/digit_divisor)%10;
		if(digit != 0||i == 0)
		{
			DrawScoreDigit(x, y, (score/digit_divisor)%10);
			if(right_align)
			{
				x -= DIGIT_WIDTH;
			}
			else
			{
				x += DIGIT_WIDTH;
			}
		}
		digit_divisor *= 10;
	}
}

void DrawScoreDigit(float x, float y, int digit)
{
	Mtx44 projection;
	Mtx modelview;
	int tex_width = GX_GetTexObjWidth(&digits_tex_obj);
	int tex_height = GX_GetTexObjHeight(&digits_tex_obj);
	float texcoord_x = (float)digit/NUM_DIGITS;
	float texcoord_w = (float)DIGIT_WIDTH/tex_width;
	
	GX_InvalidateTexAll();
	guOrtho(projection, 0, SCREEN_HEIGHT, 0, SCREEN_WIDTH, 0, 10);
	GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);
	GX_SetViewport(0, 0, GetRenderWidth(), GetRenderHeight(), 0, 1);
	GX_SetScissor(0, 0, GetRenderWidth(), GetRenderHeight());
	guMtxIdentity(modelview);
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	GX_LoadTexObj(&digits_tex_obj, GX_TEXMAP0);
	GX_SetNumTevStages(1);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZCompLoc(GX_FALSE);
	GX_SetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetNumTexGens(1);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position2f32(x, y);
	GX_Color4u8(255, 255, 255, brightness);
	GX_TexCoord2f32(texcoord_x, 0);
	GX_Position2f32(x+DIGIT_WIDTH, y);
	GX_Color4u8(255, 255, 255, brightness);
	GX_TexCoord2f32(texcoord_x+texcoord_w, 0);
	GX_Position2f32(x+DIGIT_WIDTH, y+tex_height);
	GX_Color4u8(255, 255, 255, brightness);
	GX_TexCoord2f32(texcoord_x+texcoord_w, 1);
	GX_Position2f32(x, y+tex_height);
	GX_Color4u8(255, 255, 255, brightness);
	GX_TexCoord2f32(texcoord_x, 1);
	GX_End();
}

void DrawPaddles()
{
	int i;
	SetPrintColor(255, 255, 255, brightness);
	for(i=0; i<MAX_PLAYERS; i++)
	{
		PrintRectangle(paddles[i].x, paddles[i].y, paddles[i].w, paddles[i].h);
	}
}

void DrawBall()
{
	Mtx44 projection;
	Mtx modelview;
	
	int i;
	GX_InvalidateTexAll();
	guOrtho(projection, 0, SCREEN_HEIGHT, 0, SCREEN_WIDTH, 0, 10);
	GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);
	GX_SetViewport(0, 0, GetRenderWidth(), GetRenderHeight(), 0, 1);
	GX_SetScissor(0, 0, GetRenderWidth(), GetRenderHeight());
	guMtxTrans(modelview, ball.x, ball.y, 0);
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	GX_SetNumTevStages(1);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZCompLoc(GX_FALSE);
	GX_SetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetNumTexGens(1);
	GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, NUM_BALL_VERTICES+2);
	GX_Position2f32(0, 0);
	GX_Color4u8(255, 255, 255, brightness);
	for(i=0; i<=NUM_BALL_VERTICES; i++)
	{
		GX_Position2f32(ball.size*cos(i*M_PI*2/NUM_BALL_VERTICES)/2, ball.size*sin(i*M_PI*2/NUM_BALL_VERTICES)/2);
		GX_Color4u8(255, 255, 255, brightness);
	}
	GX_End();
}

void DrawPauseScreen()
{
	SetPrintColor(255, 255, 255, brightness);
	PrintString("Paused", HALF_SCREEN_WIDTH, HALF_SCREEN_HEIGHT, 1.0, ALIGN_CENTER);
}

void GameUpdate()
{
	int winner = -1;
	int i;
	if(fade_dir != FADE_NONE)
	{
		if(fade_dir == FADE_IN)
		{
			brightness += FADE_SPEED;
		}
		else if(fade_dir == FADE_OUT)
		{
			brightness -= FADE_SPEED;
		}
		if(brightness >= 255)
		{
			brightness = 255;
			fade_dir = FADE_NONE;
		}
		if(brightness <= 0 && fade_dir == FADE_OUT)
		{
			free_aligned(game_gfx);
			next_state = STATE_GAME_OVER;
		}
		return;
	}
	if(!paused)
	{
		if(wait_timer == 0)
		{
			UpdateBall();
			UpdatePaddles();
			if(pause_wait_timer == 0)
			{
				for(i=0; i<MAX_PLAYERS; i++)
				{
					if((PAD_ButtonsDown(i) & PAD_BUTTON_START)||(WPAD_ButtonsDown(i) & WPAD_BUTTON_PLUS))
					{
						paused = true;
						paused_player = i;
						pause_wait_timer = PAUSE_WAIT_TIME;
					}
				}
			}
			else
			{
				pause_wait_timer--;
			}
		}
		else
		{
			ball.size += (BALL_SIZE/(float)BALL_WAIT_TIME);
			wait_timer--;
		}
	}
	else
	{
		if(pause_wait_timer == 0)
		{
			if((PAD_ButtonsDown(paused_player) & PAD_BUTTON_START)||(WPAD_ButtonsDown(paused_player) & WPAD_BUTTON_PLUS))
			{
				paused = false;
				pause_wait_timer = PAUSE_WAIT_TIME;
			}
		}
		else
		{
			pause_wait_timer--;
		}
	}
	if(scores[0] >= game_setup.win_score)
	{
		winner = 0;
	}
	if(scores[1] >= game_setup.win_score && paddles[1].ai_level == AI_LEVEL_NONE)
	{
		winner = 1;
	}
	if(scores[1] >= game_setup.win_score && paddles[1].ai_level != AI_LEVEL_NONE)
	{
		winner = 2;
	}
	if(winner != -1)
	{
		fade_dir = FADE_OUT;
		state_param = winner;
	}
}

void ResetField()
{
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		paddles[i].y = HALF_SCREEN_HEIGHT-(PADDLE_HEIGHT/2);
	}
	ball.x = HALF_SCREEN_WIDTH;
	ball.y = HALF_SCREEN_HEIGHT;
	ball.dx = 2;
	ball.dy = 2;
	ball.size = 0;
	wait_timer = BALL_WAIT_TIME;
}

bool CheckBallPaddleCollide(BallObject *ball, PaddleObject *paddle)
{
	float left_ball, left_paddle;
	float right_ball, right_paddle;
	float top_ball, top_paddle;
	float bottom_ball, bottom_paddle;

	left_ball = ball->x-(ball->size/2);
	right_ball = ball->x+(ball->size/2);
	top_ball = ball->y-(ball->size/2);
	bottom_ball = ball->y+(ball->size/2);

	left_paddle = paddle->x;
	right_paddle = paddle->x + paddle->w;
	top_paddle = paddle->y;
	bottom_paddle = paddle->y + paddle->h;
	

	if (left_ball > right_paddle) {
		return false;
	}

	if (right_ball < left_paddle) {
		return false;
	}

	if (top_ball > bottom_paddle) {
		return false;
	}

	if (bottom_ball < top_paddle) {
		return false;
	}

	return true;
}

void UpdatePaddles()
{
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(paddles[i].ai_level != AI_LEVEL_NONE)
		{
			UpdateAiPaddle(i);
		}
		else
		{
			UpdatePlayerPaddle(i);
		}
	}
}

void UpdatePlayerPaddle(int paddle_idx)
{
	int buttons = PAD_ButtonsHeld(paddle_idx);
	int wii_buttons = WPAD_ButtonsHeld(paddle_idx);
	int stick_y = PAD_StickY(paddle_idx);
	if((buttons & PAD_BUTTON_DOWN)||(wii_buttons & WPAD_BUTTON_DOWN)||stick_y <= -64)
	{
		paddles[paddle_idx].y += PADDLE_SPEED;
	}
	if((buttons & PAD_BUTTON_UP)||(wii_buttons & WPAD_BUTTON_UP)||stick_y >= 64)
	{
		paddles[paddle_idx].y -= PADDLE_SPEED;
	}
	if(paddles[paddle_idx].y < PLAY_FIELD_MIN_Y)
	{
		paddles[paddle_idx].y = PLAY_FIELD_MIN_Y;
	}
	if(paddles[paddle_idx].y > PLAY_FIELD_MAX_Y-paddles[paddle_idx].h)
	{
		paddles[paddle_idx].y = PLAY_FIELD_MAX_Y-paddles[paddle_idx].h;
	}
}

void UpdateAiPaddle(int paddle_idx)
{
	if(ball.dy >= (PADDLE_SPEED*ai_speeds[paddles[paddle_idx].ai_level]))
	{
		paddles[paddle_idx].y += (PADDLE_SPEED*ai_speeds[paddles[paddle_idx].ai_level]);
	}
	else
	{
		paddles[paddle_idx].y += ball.dy;
	}
	if(paddles[paddle_idx].y < PLAY_FIELD_MIN_Y)
	{
		paddles[paddle_idx].y = PLAY_FIELD_MIN_Y;
	}
	if(paddles[paddle_idx].y > PLAY_FIELD_MAX_Y-paddles[paddle_idx].h)
	{
		paddles[paddle_idx].y = PLAY_FIELD_MAX_Y-paddles[paddle_idx].h;
	}
}

void UpdateBall()
{
	ball.x += ball.dx;
	ball.y += ball.dy;
	if(ball.x < -ball.size)
	{
		scores[1] += 1;
		ResetField();
		return;
	}
	if(ball.x > SCREEN_WIDTH+ball.size)
	{
		scores[0] += 1;
		ResetField();
		return;
	}
	if(ball.y < PLAY_FIELD_MIN_Y+(ball.size/2)||ball.y > PLAY_FIELD_MAX_Y-(ball.size/2))
	{
		ball.y -= ball.dy;
		ball.dy = -ball.dy;
	}
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(CheckBallPaddleCollide(&ball, &paddles[i]))
		{
			//Revert Effect of Velocity to fix Multi-Collision Bug
			ball.x -= ball.dx;
			ball.y -= ball.dy;
			
			if(ball.dx < 0)
			{
				ball.dx -= 1;
			}
			else
			{
				ball.dx += 1;
			}
			ball.dx = -ball.dx;
			float hit_ofs = (paddles[i].y+(paddles[i].h/2))-ball.y;
			ball.dy = hit_ofs*-HIT_Y_SPEED_SCALE;
			if(ball.dx >= BALL_MAX_SPEED)
			{
				ball.dx = BALL_MAX_SPEED;
			}
			else if(ball.dx < -BALL_MAX_SPEED) 
			{
				ball.dx = -BALL_MAX_SPEED;
			}
		}
	}
}