#ifndef STATEDEFS_H
#define STATEDEFS_H

#define STATE_TITLE 0
#define STATE_GAME 1
#define STATE_GAME_OVER 2
#define STATE_MATCH_SETTINGS 3
#define STATE_NONE -1

int next_state;
int state_param;

typedef void (*StateInitFunc)(int);
typedef void (*StateFunc)();

typedef struct state_entry
{
	StateInitFunc init;
	StateFunc callback;
} StateEntry;

extern StateEntry state_entries[];

extern int num_states;

#endif