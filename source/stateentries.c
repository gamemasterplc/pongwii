#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include "statedefs.h"
#include "title.h"
#include "game.h"
#include "gameover.h"
#include "matchsettings.h"

StateEntry state_entries[] =
{
	{ TitleInit, TitleCallback },
	{ GameInit, GameCallback },
	{ GameOverInit, GameOverCallback },
	{ MatchSettingsInit, MatchSettingsCallback }
};

int num_states = sizeof(state_entries)/sizeof(StateEntry);