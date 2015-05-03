texas: texas.c cards.c cards.h th.c th.h pool.c pool.h ann1.c ann1.h
	$(CC) -Wall -Werror -std=c99 -g3 -lfann -lm -o texas cards.c th.c pool.c ann1.c texas.c
