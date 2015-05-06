#include <stdlib.h>

#include "ai_common.h"

int ai_pool_func(int *num_rounds_played)
{
	(*num_rounds_played)++;
	if(*num_rounds_played >= 10) {
		*num_rounds_played = 0;
		return 0;
	}
	return 1;
}

float rand_uniform()
{
	return rand() / (float)RAND_MAX;
}

float rand_uniform_clamped()
{
	return rand_uniform() * 2.0f - 1.0f;
}


