#pragma once

#include "th.h"

enum th_decision random_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);
int random_pool_func(void *data);
void random_data_init(void *data);


