#include <stdlib.h>

#include "ai_common.h"
#include "ai_random.h"

struct ai_data {
	int num_rounds_played;
};

enum th_decision random_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	if(!raised_to) {
		int val = rand() % 2;
		return val ? DEC_RAISE : DEC_CHECK;
	}
	else {
		int val = rand() % 3;
		return val + 1;
	}
}

int random_pool_func(void *data)
{
	struct ai_data *d = (struct ai_data *)data;

	return ai_pool_func(&d->num_rounds_played);
}

void random_data_init(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	d->num_rounds_played = 0;
}


