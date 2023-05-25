#ifndef _SETTINGS_H_
#define _SETTINGS_H_

void settings_default();
bool settings_load();
void settings_init();
void settings_save();
void settings_saveLater();
void settings_saveTask(bool);

#endif