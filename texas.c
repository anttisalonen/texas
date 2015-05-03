#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>

#include "cards.h"
#include "th.h"
#include "pool.h"
#include "ann1.h"
#include "ai_random.h"

static enum th_decision human_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	while(1) {
		th_print_money(th, 0);
		th_print_community_cards(th);
		printf("Your cards:\n");
		print_cards(th->players[plnum].hole_cards, 2);

		if(raised_to)
			printf("(f)old, c(a)ll, (r)aise?\n");
		else
			printf("(c)heck, (r)aise?\n");
		char buf[256];
		memset(buf, 0x00, sizeof(buf));
		read(STDIN_FILENO, buf, 256);
		switch(buf[0]) {
			case 'c':
				if(!raised_to)
					return DEC_CHECK;
				break;
			case 'f':
				if(raised_to)
					return DEC_FOLD;
				break;
			case 'a':
				if(raised_to)
					return DEC_CALL;
				break;
			case 'r':
				return DEC_RAISE;
			case 'q':
				exit(0);
				break;
			default:
				break;
		}
	}
}

static int human_pool_func(void *data)
{
	return 1;
}


int main(int argc, char **argv)
{
	printf("-----\n");

	int seed = time(NULL);
	int human = 0;
	int start_money = 20;
	int max_rounds = -1;
	int num_random_ais = 10;
	int num_ann_ais = 0;
	int do_save = 0;

	int ann_params_pos = 0;
	char ann_params[POOL_MAX_PLAYERS][256];
	memset(ann_params, 0x00, sizeof(ann_params));

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-s")) {
			seed = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--human")) {
			human = 1;
		}
		else if(!strcmp(argv[i], "--money")) {
			start_money = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--rounds")) {
			max_rounds = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--random")) {
			num_random_ais = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--ann")) {
			num_ann_ais = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--save")) {
			do_save = 1;
		}
		else if(!strcmp(argv[i], "--ann_param")) {
			strncpy(ann_params[ann_params_pos++], argv[++i], 256);
		}
	}
	printf("Random seed: %d\n", seed);
	srand(seed);

	struct player_pool pool;

	int data_pos = 0;

	struct ai_data {
		char dummy[256];
	};

	struct ai_data ais[POOL_MAX_PLAYERS];

	pool_init(&pool);
	if(human) {
		pool_add_player(&pool, start_money, human_decision, human_pool_func, NULL, NULL, "human");
		data_pos++;
	}
	for(int i = 0; i < num_random_ais; i++) {
		random_data_init(&ais[i + data_pos]);
		pool_add_player(&pool, start_money, random_decision, random_pool_func, NULL, &ais[i + data_pos], "random");
	}
	data_pos += num_random_ais;

	for(int i = 0; i < num_ann_ais; i++) {
		ann_data_init(&ais[i + data_pos], ann_params[i]);
		const char *ann_type = ann_get_type(&ais[i + data_pos]);
		char type[32];
		memset(type, 0x00, sizeof(type));
		snprintf(type, 31, "fann (%s)", ann_type);
		pool_add_player(&pool, start_money, ann_decision, ann_pool_func, ann_save_func, &ais[i + data_pos], type);
	}


	struct texas_holdem th;
	th_init(&th, 1);
	pool_update_th(&pool, &th);

	int num_rounds = 0;
	while(1) {
		if(th_play_hand(&th))
			break;
		num_rounds++;
		if(max_rounds > -1) {
			max_rounds--;
			if(max_rounds <= 0)
				break;
		}
		pool_update_th(&pool, &th);
	}
	printf("Final score after %d rounds:\n", num_rounds);
	th_print_money(&th, 1);

	if(do_save) {
		pool_save_current_players(&pool, &th);
	}

	return 0;
}
