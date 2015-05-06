#pragma once

#include "th.h"

enum th_decision ann_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);
int ann_pool_func(void *data);
void ann_data_init(void *data);
int ann_save_func(void *data, const char *filename);
int ann_load_func(void *data, const char *filename);
void ann_modify(void *data, float var);

