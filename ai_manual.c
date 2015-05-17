#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <floatfann.h>

#include "th.h"
#include "ai_common.h"
#include "ai_manual.h"

#define AIM_VERSION_STRING "1"

struct ai_data {
	int num_rounds_played;
	float want_raise;
};

enum th_decision aim_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	if(rand_uniform() < d->want_raise) {
		return DEC_RAISE;
	} else if(!raised_to) {
		return DEC_CALL;
	} else {
		return DEC_FOLD;
	}
}

int aim_pool_func(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	return ai_pool_func(&d->num_rounds_played);
}

int aim_load_func(void *data, const char *filename)
{
	struct ai_data *d = (struct ai_data *)data;
	d->num_rounds_played = 0;
	float *values[] = {&d->want_raise, NULL};
	return ai_common_load_func("AIM " AIM_VERSION_STRING, values, filename);
}

void aim_data_init(const char *type, void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	d->want_raise = rand_uniform();
	d->num_rounds_played = 0;
}

int aim_save_func(void *data, const char *filename)
{
	struct ai_data *d = (struct ai_data *)data;
	float *values[] = {&d->want_raise, NULL};
	return ai_common_save_func("AIM " AIM_VERSION_STRING, values, filename);
}

