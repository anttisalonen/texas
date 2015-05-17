#pragma once

#include "th.h"

enum th_decision aim2_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);
int aim2_pool_func(void *data);
void aim2_data_init(const char *type, void *data);
int aim2_save_func(void *data, const char *filename);
int aim2_load_func(void *data, const char *filename);

