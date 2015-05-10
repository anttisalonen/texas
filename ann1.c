#include <string.h>
#include <assert.h>

#include <floatfann.h>

#include "th.h"
#include "ai_common.h"
#include "ann1.h"

struct ai_data {
	struct fann *ann;
	int num_rounds_played;
};

int ann_pool_func(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	return ai_pool_func(&d->num_rounds_played);
}

enum th_decision ann_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	fann_type *calc_out;
	fann_type input[RANK_ACE + 1];

	for(int j = RANK_TWO; j < RANK_ACE; j++) {
		input[j] = 0.0f;
		for(int i = 0; i < 2; i++) {
			if(th->players[plnum].hole_cards[i].rank == j)
				input[j] += 0.25f;
		}
		for(int i = 0; i < th->num_community_cards; i++) {
			if(th->community_cards[i].rank == j)
				input[j] += 0.25f;
		}
	}

	input[RANK_ACE] = (2.0f * (th->pot / (1.0f + th->players[plnum].money * 0.5f))) - 1.0f;
	if(input[RANK_ACE] > 1.0f)
		input[RANK_ACE] = 1.0f;

	struct ai_data *d = (struct ai_data *)data;
	calc_out = fann_run(d->ann, input);

	if(calc_out[0] > calc_out[1] && calc_out[0] > calc_out[2])
		return DEC_RAISE;
	else if(raised_to > 0 && calc_out[1] > calc_out[2])
		return DEC_CALL;
	else if(raised_to > 0)
		return DEC_FOLD;
	else
		return DEC_CHECK;
}

void ann_modify(void *data, float var)
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

int ann_load_func(void *data, const char *filename)
{
	struct ai_data *d = (struct ai_data *)data;
	struct fann **ann = &d->ann;
	*ann = fann_create_from_file(filename);
	return !ann;
}

void ann_data_init(void *data)
{
	struct ai_data *d = (struct ai_data *)data;

	struct fann **ann = &d->ann;
	int num_layers = rand() % 2 + 3;
	int num_input = RANK_ACE + 1;
	int num_output = 3;
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

int ann_save_func(void *data, const char *filename)
{
	struct ai_data *ai = (struct ai_data *)data;
	fann_save(ai->ann, filename);
	return 0;
}

