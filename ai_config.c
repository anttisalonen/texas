#include <stdlib.h>
#include <string.h>

#include "ann1.h"
#include "ai_random.h"
#include "ai_manual.h"
#include "ai_config.h"

struct ai_config ai_configs[] = {
	{
		"fann",
		ann_decision,
		ann_pool_func,
		ann_data_init,
		ann_save_func,
		ann_load_func,
		ann_modify
	},
	{
		"random",
		random_decision,
		random_pool_func,
		random_data_init,
		NULL,
		NULL,
		NULL
	},
	{
		"manual",
		aim_decision,
		aim_pool_func,
		aim_data_init,
		aim_save_func,
		aim_load_func,
		NULL
	},
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	}
};

struct ai_config *get_ai_config(const char *name)
{
	for(int i = 0; ai_configs[i].ai_name != NULL; i++) {
		if(!strcmp(name, ai_configs[i].ai_name))
			return &ai_configs[i];
	}
	return NULL;
}

int get_ai_config_index(const char *name)
{
	for(int i = 0; ai_configs[i].ai_name != NULL; i++) {
		if(!strcmp(name, ai_configs[i].ai_name))
			return i;
	}
	return -1;
}

struct ai_config *get_ai_config_by_index(int i)
{
	return &ai_configs[i];
}


