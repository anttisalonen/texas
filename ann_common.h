#pragma once

#include <floatfann.h>

#include "ai_config.h"

struct ai_data {
	struct fann *ann;
	int num_rounds_played;
	int num_inputs;
	int num_outputs;
};

struct ai_config fann_gen_ai_config(const char *name);

