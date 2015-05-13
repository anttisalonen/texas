#pragma once

#include "th.h"
#include "pool.h"

struct dummy_ai_data {
	char dummy[256];
};

typedef const char *(*ai_get_filename_func)(void *data);
typedef void (*ai_init_func)(const char *type, void *data);
typedef int (*ai_save_func)(void *data, const char *filename);
typedef int (*ai_load_func)(void *data, const char *filename);
typedef void (*ai_modify_func)(void *data, float var);

struct ai_config {
	const char *ai_name;
	th_decision_func ai_decision_func;
	pool_decision_func ai_pool_decision_func;
	ai_init_func ai_init_func;
	ai_save_func ai_save_func;
	ai_load_func ai_load_func;
	ai_modify_func ai_modify_func;
};

void ai_config_init(void);
struct ai_config *get_ai_config(const char *name);
int get_ai_config_index(const char *name);
struct ai_config *get_ai_config_by_index(int i);

