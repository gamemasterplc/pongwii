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
#include "gameover.h"
#include "game.h"
#include "winner_tpl.h"
#include "winner.h"

#define Y_MARGIN_SIZE 32
#define OPTION_SPACING 24
#define CURSOR_MOVE_DELAY 8
#define FADE_SPEED 15
#define FADE_IN 0
#define FADE_OUT 1
#define FADE_NONE -1

static GXTexObj winner_tex_obj;
static void *winner_gfx;
static int winner_gfx_ids[] = { winp1, winp2, wincpu };
static char *option_list[] = { "Retry", "Title Screen" };
static int option_dest_list[] = { STATE_GAME, STATE_TITLE };

static int num_options = sizeof(option_list)/sizeof(char *);
static int cursor_pos;
static int cursor_delay;
static int brightness;
static int fade_dir;

void GameOverInit(int winner)
{
	TPLFile tpl;
	winner_gfx = malloc_aligned(32, winner_tpl_size);
	memcpy(winner_gfx, winner_tpl, winner_tpl_size);
	TPL_OpenTPLFromMemory(&tpl, (void *)winner_gfx, winner_tpl_size);
	TPL_GetTexture(&tpl, winner_gfx_ids[winner], &winner_tex_obj);
	GX_InitTexObjFilterMode(&winner_tex_obj, GX_LINEAR, GX_LINEAR);
	cursor_pos = 0;
	cursor_delay = 0;
	brightness = 0;
	fade_dir = FADE_IN;
}

void GameOverCallback()
{
	GameOverUpdate();
	GameOverDraw();
}

void DrawWinner()
{
	Mtx44 projection;
	Mtx modelview;
	int tex_width = GX_GetTexObjWidth(&winner_tex_obj);
	int tex_height = GX_GetTexObjHeight(&winner_tex_obj);
	int half_tex_width = tex_width/2;
	
	GX_InvalidateTexAll();
	guOrtho(projection, 0, SCREEN_HEIGHT, 0, SCREEN_WIDTH, 0, 10);
	GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);
	GX_SetViewport(0, 0, GetRenderWidth(), GetRenderHeight(), 0, 1);
	GX_SetScissor(0, 0, GetRenderWidth(), GetRenderHeight());
	guMtxIdentity(modelview);
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	GX_LoadTexObj(&winner_tex_obj, GX_TEXMAP0);
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
	GX_Position2f32(HALF_SCREEN_WIDTH-half_tex_width, Y_MARGIN_SIZE);
	GX_Color4u8(255, 255, 255, brightness);
	GX_TexCoord2f32(0, 0);
	GX_Position2f32(HALF_SCREEN_WIDTH+half_tex_width, Y_MARGIN_SIZE);
	GX_Color4u8(255, 255, 255, brightness);
	GX_TexCoord2f32(1, 0);
	GX_Position2f32(HALF_SCREEN_WIDTH+half_tex_width, Y_MARGIN_SIZE+tex_height);
	GX_Color4u8(255, 255, 255, brightness);
	GX_TexCoord2f32(1, 1);
	GX_Position2f32(HALF_SCREEN_WIDTH-half_tex_width, Y_MARGIN_SIZE+tex_height);
	GX_Color4u8(255, 255, 255, brightness);
	GX_TexCoord2f32(0, 1);
	GX_End();
}

void GameOverDraw()
{
	int i;
	int win_tex_height = GX_GetTexObjHeight(&winner_tex_obj);
	DrawWinner();
	for(i=0; i<num_options; i++)
	{
		if(i == cursor_pos)
		{
			SetPrintColor(0, 255, 0, brightness);
		}
		else
		{
			SetPrintColor(255, 255, 255, brightness);
		}
		float y_ofs = (num_options-i-1)*OPTION_SPACING;
		PrintString(option_list[i], HALF_SCREEN_WIDTH, SCREEN_HEIGHT-Y_MARGIN_SIZE-OPTION_SPACING-y_ofs, 1.0f, ALIGN_CENTER);
	}
	SetPrintColor(255, 255, 255, brightness);
	PrintString("Game Over", HALF_SCREEN_WIDTH, Y_MARGIN_SIZE+win_tex_height, 1.0f, ALIGN_CENTER);
}

void GameOverUpdate()
{
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
			free_aligned(winner_gfx);
			next_state = option_dest_list[cursor_pos];
		}
		return;
	}
	int buttons = PAD_ButtonsHeld(0);
	int wii_buttons = WPAD_ButtonsHeld(0);
	int stick_y = PAD_StickY(0);
	if(cursor_delay == 0)
	{
		if((buttons & PAD_BUTTON_DOWN)||(wii_buttons & WPAD_BUTTON_DOWN)||stick_y <= -64)
		{
			cursor_pos++;
			cursor_delay = CURSOR_MOVE_DELAY;
		}
		if((buttons & PAD_BUTTON_UP)||(wii_buttons & WPAD_BUTTON_UP)||stick_y >= 64)
		{
			cursor_pos--;
			cursor_delay = CURSOR_MOVE_DELAY;
		}
	}
	else
	{
		cursor_delay--;
	}
	if(cursor_pos < 0)
	{
		cursor_pos = num_options-1;
	}
	if(cursor_pos >= num_options)
	{
		cursor_pos = 0;
	}
	if((PAD_ButtonsDown(0) & PAD_BUTTON_A)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_A))
	{
		fade_dir = FADE_OUT;
	}
}