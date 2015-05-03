#include <string.h>
#include <time.h>
#include <assert.h>

#include <floatfann.h>

#include "th.h"
#include "ai_common.h"
#include "ann1.h"

struct ai_data {
	struct fann *ann;
	int num_rounds_played;
	char filename[128];
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

static void ann_modify(struct fann *ann)
{
	int num_connections = fann_get_total_connections(ann);
	struct fann_connection connections[num_connections];
	fann_get_connection_array(ann, connections);
	for(int i = 0; i < num_connections; i++) {
		float diff = ((rand() % 1000) * 0.0002f) - 0.1f;
		float w = connections[i].weight + diff;
		if(w > 1.0f)
			w = 1.0f;
		if(w < -1.0f)
			w = -1.0f;
		connections[i].weight = w;
	}
	fann_set_weight_array(ann, connections, num_connections);
}

static void ann_set_filename(char *to, const char *filename)
{
	char buf[256];
	strncpy(buf, filename, 256);
	while(1) {
		char *dot = strrchr(buf, '.');
		if(!dot)
			break;
		*dot = '\0';
	}
	snprintf(to, 255, "%s", buf);
}

static void ann_create_filename(char *to, const char *old)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	snprintf(to, 255, "ai_%s_%02d%02d%02d%04ld",
			old, tm.tm_hour,
			tm.tm_min, tm.tm_sec, tv.tv_usec / 1000);
}

static void ann_update_filename(char *to, const char *old)
{
	char buf[256];
	char old_copy[256];
	strncpy(old_copy, old, 256);
	char *dot = strrchr(old_copy, '_');
	assert(dot);
	*dot = '\0';
	int ret = sscanf(old_copy, "ai_%s", buf);
	assert(ret == 1);
	ann_create_filename(to, buf);
}

static void ann_new_filename(char *filename)
{
	char randstr[9];
	memset(randstr, 0x00, sizeof(randstr));
	for(int j = 0; j < 8; j++) {
		randstr[j] = rand() % 20 + 'a';
	}
	ann_create_filename(filename, randstr);
}

void ann_data_init(void *data, const char *param)
{
	struct ai_data *d = (struct ai_data *)data;

	struct fann **ann = &d->ann;
	if(param[0]) {
		*ann = fann_create_from_file(param);
		ann_set_filename(d->filename, param);
		printf("FANN AI %s loaded from %s\n", d->filename, param);
		if(rand() % 2 == 0) {
			ann_modify(*ann);
			ann_update_filename(d->filename, param);
			printf("FANN AI %s modified from %s\n", d->filename, param);
		}
	} else {
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
		ann_new_filename(d->filename);
		printf("FANN new AI: %s\n", d->filename);
	}
	d->num_rounds_played = 0;
}

int ann_save_func(void *data)
{
	struct ai_data *ai = (struct ai_data *)data;
	char filename[256];
	snprintf(filename, 256, "%s.net", ai->filename);
	fann_save(ai->ann, filename);
	printf("FANN Saved file %s.\n", filename);
	return 0;
}

const char *ann_get_type(void *data)
{
	struct ai_data *ai = (struct ai_data *)data;
	return ai->filename;
}

