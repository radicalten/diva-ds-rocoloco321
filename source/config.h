#pragma once

#define CONFIGPATH "/divaDS/settings"

typedef struct {
	int speed;
	int opacity;
	bool cache;
	bool cache_bg;
	bool mines;
	bool debug;
} settings_t;

extern settings_t gSettings;

void config_load();
void config_save();
void config_init();
void config_validate();