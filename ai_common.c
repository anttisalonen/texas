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


