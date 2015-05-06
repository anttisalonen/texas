#pragma once

#include "th.h"
#include "pool.h"

typedef const char *(*ai_get_filename_func)(void *data);
typedef void (*ai_init_func)(void *data, const char *param);

struct ai_config {
	const char *ai_name;
	th_decision_func ai_decision_func;
	pool_decision_func ai_pool_decision_func;
	ai_init_func ai_init_func;
	pool_save_func ai_save_func;
	ai_get_filename_func ai_filename_func;
};

struct ai_config *get_ai_config(const char *name);
int get_ai_config_index(const char *name);
struct ai_config *get_ai_config_by_index(int i);
