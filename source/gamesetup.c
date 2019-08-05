#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include "game.h"

GameSetup game_setup =
{
	1, //Players
	10, //Maximum Score
	AI_LEVEL_NORMAL,
	true
};