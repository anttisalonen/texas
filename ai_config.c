#include <stdlib.h>
#include <string.h>

#include "ann1.h"
#include "ai_random.h"
#include "ai_manual.h"
#include "ai_manual2.h"
#include "ai_config.h"
#include "ann_common.h"

struct ai_config ai_configs[6];

void ai_config_init(void)
{
	ai_configs[0] = fann_gen_ai_config("fann");
	ai_configs[1] = (struct ai_config) {
		"random",
		random_decision,
		random_pool_func,
		random_data_init,
		NULL,
		NULL,
		NULL
	};
	ai_configs[2] = (struct ai_config) {
		"manual",
		aim_decision,
		aim_pool_func,
		aim_data_init,
		aim_save_func,
		aim_load_func,
		NULL
	};
	ai_configs[3] = fann_gen_ai_config("fann2");
	ai_configs[4] = (struct ai_config) {
		"manual2",
		aim2_decision,
		aim2_pool_func,
		aim2_data_init,
		aim2_save_func,
		aim2_load_func,
		NULL
	};
	ai_configs[5] = (struct ai_config) {
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};
}

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


