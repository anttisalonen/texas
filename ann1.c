#include <string.h>
#include <time.h>
#include <assert.h>

#include <floatfann.h>

#include "th.h"
#include "ann1.h"

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

int ai_pool_func(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	d->num_rounds_played++;
	if(d->num_rounds_played >= 10) {
		d->num_rounds_played = 0;
		return 0;
	}
	return 1;
}

void ann_modify(struct fann *ann)
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

void ann_set_filename(char *to, char *filename)
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

void ann_update_filename(char *to, const char *old)
{
	char buf[256];
	char old_copy[256];
	strncpy(old_copy, old, 256);
	char *dot = strrchr(old_copy, '_');
	assert(dot);
	*dot = '\0';
	int ret = sscanf(old_copy, "ai_%s", buf);
	assert(ret == 1);
	printf("%s\n", buf);
	ann_create_filename(to, buf);
}

void ann_new_filename(char *filename)
{
	char randstr[9];
	memset(randstr, 0x00, sizeof(randstr));
	for(int j = 0; j < 8; j++) {
		randstr[j] = rand() % 20 + 'a';
	}
	ann_create_filename(filename, randstr);
}


