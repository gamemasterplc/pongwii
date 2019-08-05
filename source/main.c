#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include "gfxmgr.h"
#include "print.h"
#include "statedefs.h"

StateFunc curr_callback;

int main(int argc, char **argv);

int main(int argc, char **argv)
{
	GFXInit();
	InitPrint();
	PAD_Init();
	#ifdef HW_RVL
	WPAD_Init();
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
	#endif
	next_state = STATE_TITLE;
	state_param = 0;
	while(1)
	{
		if(curr_callback)
		{
			curr_callback();
		}
		if(next_state != STATE_NONE)
		{
			if(next_state >= num_states)
			{
				curr_callback = NULL;
			}
			else
			{
				curr_callback = state_entries[next_state].callback;
				if(state_entries[next_state].init)
				{
					state_entries[next_state].init(state_param);
				}
			}
			next_state = STATE_NONE;
		}
		PAD_ScanPads();
		#ifdef HW_RVL
		WPAD_ScanPads();
		#endif
		SwapBuffers();
	}
}