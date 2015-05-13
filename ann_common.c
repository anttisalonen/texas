#include <string.h>
#include <assert.h>

#include <floatfann.h>

#include "ai_common.h"
#include "ann_common.h"
#include "th.h"
#include "ann1.h"

static int ann_pool_func(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	return ai_pool_func(&d->num_rounds_played);
}

static void ann_modify(void *data, float var)
{
	struct ai_data *d = (struct ai_data *)data;
	int num_connections = fann_get_total_connections(d->ann);
	struct fann_connection connections[num_connections];
	fann_get_connection_array(d->ann, connections);
	for(int i = 0; i < num_connections; i++) {
		float diff = rand_uniform_clamped() * var;
		float w = connections[i].weight + diff;
		if(w > 1.0f)
			w = 1.0f;
		if(w < -1.0f)
			w = -1.0f;
		connections[i].weight = w;
	}
	fann_set_weight_array(d->ann, connections, num_connections);
}

static int ann_load_func(void *data, const char *filename)
{
	struct ai_data *d = (struct ai_data *)data;
	struct fann **ann = &d->ann;
	*ann = fann_create_from_file(filename);
	return !ann;
}

static int ann_save_func(void *data, const char *filename)
{
	struct ai_data *ai = (struct ai_data *)data;
	fann_save(ai->ann, filename);
	return 0;
}

static int ann_num_inputs(const char *type);
static int ann_num_outputs(const char *type);

static void ann_data_init(const char *type, void *data)
{
	struct ai_data *d = (struct ai_data *)data;

	struct fann **ann = &d->ann;
	int num_layers = rand() % 2 + 3;
	int num_input = ann_num_inputs(type);
	int num_output = ann_num_outputs(type);
	assert(num_input > 0);
	assert(num_output > 0);
	if(num_layers == 3) {
		*ann = fann_create_standard(num_layers, num_input,
				rand() % 8 + 2, num_output);
	} else if(num_layers == 4) {
		*ann = fann_create_standard(num_layers, num_input,
				rand() % 8 + 2, rand() % 8 + 2, num_output);
	}
	fann_randomize_weights(*ann, -1.0f, 1.0f);
	fann_set_activation_function_hidden(*ann, FANN_SIGMOID_SYMMETRIC);
	fann_set_activation_function_output(*ann, FANN_SIGMOID_SYMMETRIC);

	d->num_rounds_played = 0;
}

static struct ai_config fann_gen(const char *name,
		th_decision_func func)
{

	struct ai_config c = {
		name,
		func,
		ann_pool_func,
		ann_data_init,
		ann_save_func,
		ann_load_func,
		ann_modify
	};
	return c;
}

static int ann_num_inputs(const char *type)
{
	if(!strcmp(type, "fann")) {
		return RANK_ACE + 1;
	}

	if(!strcmp(type, "fann2")) {
		return 2 * RANK_ACE + 1;
	}
	assert(0);
	return 0;
}

static int ann_num_outputs(const char *type)
{
	if(!strcmp(type, "fann")) {
		return 3;
	}

	if(!strcmp(type, "fann2")) {
		return 3;
	}
	assert(0);
	return 0;
}

struct ai_config fann_gen_ai_config(const char *name)
{
	if(!strcmp(name, "fann")) {
		return fann_gen("fann", ann_decision);
	}
	if(!strcmp(name, "fann2")) {
		return fann_gen("fann2", ann2_decision);
	}

	assert(0);
	struct ai_config c;
	return c;
}


