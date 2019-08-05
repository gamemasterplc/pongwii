#ifndef MATCHSETTINGS_H
#define MATCHSETTINGS_H

typedef struct match_setting
{
	char *name;
	char **val_list; //Null Means its a Number;
	int num_values;
	int min_value;
	int *modified_value;
} MatchSetting;

void MatchSettingsInit(int unused);

void MatchSettingsCallback();

void MatchSettingsDraw();

void MatchSettingsUpdate();

#endif