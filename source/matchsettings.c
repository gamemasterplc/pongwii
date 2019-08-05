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
#include "matchsettings.h"
#include "game.h"

#define X_MARGIN_SIZE 16
#define Y_MARGIN_SIZE 32
#define OPTION_SPACING 24
#define CURSOR_MOVE_DELAY 8
#define FADE_SPEED 15
#define FADE_IN 0
#define FADE_OUT 1
#define FADE_NONE -1

static int widescreen_flag;

char *difficulty_list[] =
{
	"Easy",
	"Normal",
	"Hard"
};

char *yes_no_list[] =
{
	"No",
	"Yes",
};

MatchSetting match_settings_list[] =
{
	{ "Points", NULL, 30, 1, &game_setup.win_score },
	{ "Difficulty", difficulty_list, sizeof(difficulty_list)/sizeof(char *), 0, &game_setup.ai_level },
	{ "Show Separator", yes_no_list, sizeof(yes_no_list)/sizeof(char *), 0, &game_setup.draw_separator },
	{ "Widescreen", yes_no_list, sizeof(yes_no_list)/sizeof(char *), 0, &widescreen_flag }
};

static int num_options = sizeof(match_settings_list)/sizeof(MatchSetting);

static int cursor_pos;
static int cursor_delay;
static int brightness;
static int fade_dir;

void MatchSettingsInit(int unused)
{
	cursor_pos = 0;
	brightness = 0;
	fade_dir = FADE_IN;
	widescreen_flag = GetWidescreenFlag();
}

void MatchSettingsCallback()
{
	MatchSettingsUpdate();
	MatchSettingsDraw();
}

void MatchSettingsDraw()
{
	char option_value[64];
	int i;
	SetPrintColor(255, 255, 255, brightness);
	PrintString("Game Settings", HALF_SCREEN_WIDTH, Y_MARGIN_SIZE, 1.0f, ALIGN_CENTER);
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
		char **val_list = match_settings_list[i].val_list;
		if(val_list)
		{
			sprintf(option_value, "%s", val_list[*match_settings_list[i].modified_value]);
		}
		else
		{
			sprintf(option_value, "%d", *match_settings_list[i].modified_value);
		}
		float y_ofs = i*OPTION_SPACING;
		PrintString(match_settings_list[i].name, X_MARGIN_SIZE, Y_MARGIN_SIZE+(OPTION_SPACING*2)+y_ofs, 1.0f, ALIGN_LEFT);
		PrintString(option_value, SCREEN_WIDTH-X_MARGIN_SIZE, Y_MARGIN_SIZE+(OPTION_SPACING*2)+y_ofs, 1.0f, ALIGN_RIGHT);
	}
	SetPrintColor(255, 255, 255, brightness);
	if(cursor_pos == num_options)
	{
		SetPrintColor(0, 255, 0, brightness);
	}
	PrintString("Return to Title Screen", X_MARGIN_SIZE, SCREEN_HEIGHT-Y_MARGIN_SIZE-OPTION_SPACING, 1.0f, ALIGN_LEFT);
}

void MatchSettingsUpdate()
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
			SetWidescreenFlag(widescreen_flag);
			next_state = STATE_TITLE;
		}
		return;
	}
	int buttons = PAD_ButtonsHeld(0);
	int wii_buttons = WPAD_ButtonsHeld(0);
	int stick_y = PAD_StickY(0);
	int stick_x = PAD_StickX(0);
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
		if((buttons & PAD_BUTTON_LEFT)||(wii_buttons & WPAD_BUTTON_LEFT)||stick_x <= -64)
		{
			(*match_settings_list[cursor_pos].modified_value)--;
			cursor_delay = CURSOR_MOVE_DELAY;
			if(match_settings_list[cursor_pos].val_list)
			{
				if(*match_settings_list[cursor_pos].modified_value < 0)
				{
					(*match_settings_list[cursor_pos].modified_value) = match_settings_list[cursor_pos].num_values-1;
				}
			}
			else
			{
				if(*match_settings_list[cursor_pos].modified_value < match_settings_list[cursor_pos].min_value)
				{
					(*match_settings_list[cursor_pos].modified_value) = match_settings_list[cursor_pos].num_values+match_settings_list[cursor_pos].min_value-1;
				}
			}
		}
		if((buttons & PAD_BUTTON_RIGHT)||(wii_buttons & WPAD_BUTTON_RIGHT)||stick_x >= 64)
		{
			(*match_settings_list[cursor_pos].modified_value)++;
			cursor_delay = CURSOR_MOVE_DELAY;
			if(match_settings_list[cursor_pos].val_list)
			{
				if(*match_settings_list[cursor_pos].modified_value >= match_settings_list[cursor_pos].num_values)
				{
					(*match_settings_list[cursor_pos].modified_value) = 0;
				}
			}
			else
			{
				if(*match_settings_list[cursor_pos].modified_value >= (match_settings_list[cursor_pos].num_values+match_settings_list[cursor_pos].min_value))
				{
					(*match_settings_list[cursor_pos].modified_value) = match_settings_list[cursor_pos].min_value;
				}
			}
		}
	}
	else
	{
		cursor_delay--;
	}
	if(cursor_pos < 0)
	{
		cursor_pos = num_options;
	}
	if(cursor_pos > num_options)
	{
		cursor_pos = 0;
	}
	if(((PAD_ButtonsDown(0) & PAD_BUTTON_A)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)) && cursor_pos == num_options)
	{
		fade_dir = FADE_OUT;
	}
	if((PAD_ButtonsDown(0) & PAD_BUTTON_B)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_B))
	{
		fade_dir = FADE_OUT;
	}
}