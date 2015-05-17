#pragma once

#include "th.h"

void card_to_text(const struct card *card, char *buf);

enum th_decision human_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);
void ncui_event_callback(const struct texas_holdem *th, const struct th_event *ev);
void ncui_set_speed(int s);
void ncui_set_human(int h);
void ncui_init(void);
void ncui_deinit(void);


