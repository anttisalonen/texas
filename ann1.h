#pragma once

#include "th.h"

enum th_decision ann_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);
enum th_decision ann2_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);

