texas: texas.c cards.c cards.h th.c th.h pool.c pool.h ai_common.c ai_common.h ai_random.c ai_random.h ann1.c ann1.h
	$(CC) -Wall -Werror -std=c99 -g3 -lfann -lm -o texas cards.c th.c pool.c ai_common.c ai_random.c ann1.c texas.c
