#pragma once

#include "th.h"

enum th_decision aim_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);
int aim_pool_func(void *data);
void aim_data_init(const char *type, void *data);
int aim_save_func(void *data, const char *filename);
int aim_load_func(void *data, const char *filename);

