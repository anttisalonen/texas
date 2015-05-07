#pragma once

#include "th.h"

enum th_decision human_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);
void event_callback(const struct texas_holdem *th, const struct th_event *ev);
void ncui_set_speed(int s);
void ncui_init(void);
void ncui_deinit(void);


