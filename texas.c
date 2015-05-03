#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>

#include <floatfann.h>

#include "cards.h"
#include "th.h"
#include "pool.h"
#include "ann1.h"

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

static enum th_decision random_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
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

	int ann_brain_files_pos = 0;
	char ann_brain_files[POOL_MAX_PLAYERS][256];
	memset(ann_brain_files, 0x00, sizeof(ann_brain_files));

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
		} else {
			strncpy(ann_brain_files[ann_brain_files_pos++], argv[i], 256);
		}
	}
	printf("Random seed: %d\n", seed);
	srand(seed);

	int num_ann_brain_files = ann_brain_files_pos;

	struct player_pool pool;
	int num_players = num_random_ais + num_ann_ais + (human ? 1 : 0);

	struct ai_data ais[num_players];
	memset(ais, 0x00, sizeof(ais));
	int data_pos = 0;

	pool_init(&pool);
	if(human) {
		ais[0].ann = NULL;
		pool_add_player(&pool, start_money, human_decision, human_pool_func, NULL, "human");
		data_pos++;
	}
	for(int i = 0; i < num_random_ais; i++) {
		ais[i + data_pos].num_rounds_played = 0;
		ais[i + data_pos].ann = NULL;
		pool_add_player(&pool, start_money, random_decision, ai_pool_func, &ais[i + data_pos], "random");
	}
	data_pos += num_random_ais;

	for(int i = 0; i < num_ann_ais; i++) {
		struct fann **ann = &ais[i + data_pos].ann;
		const char *type = "fann";
		if(ann_brain_files_pos) {
			ann_brain_files_pos--;
			*ann = fann_create_from_file(ann_brain_files[ann_brain_files_pos]);
			type = ann_brain_files[ann_brain_files_pos];
			ann_set_filename(ais[i + data_pos].filename, ann_brain_files[ann_brain_files_pos]);
			printf("FANN AI %s loaded from %s\n", ais[i + data_pos].filename, type);
		} else {
			if(num_ann_brain_files && (rand() % 2 == 1)) {
				int brain_index = rand() % num_ann_brain_files;
				*ann = fann_create_from_file(ann_brain_files[brain_index]);
				ann_modify(*ann);
				type = ann_brain_files[brain_index];
				ann_update_filename(ais[i + data_pos].filename,
						ann_brain_files[brain_index]);
				printf("FANN AI %s modified from %s\n", ais[i + data_pos].filename, type);
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
				ann_new_filename(ais[i + data_pos].filename);
				printf("FANN new AI: %s\n", ais[i + data_pos].filename);
			}
		}
		ais[i + data_pos].num_rounds_played = 0;
		pool_add_player(&pool, start_money, ann_decision, ai_pool_func, &ais[i + data_pos], ais[i + data_pos].filename);
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
		for(int i = 0; i < TH_MAX_PLAYERS; i++) {
			if(th.players[i].money && th.players[i].decision_data) {
				struct ai_data *ai = (struct ai_data *)th.players[i].decision_data;
				if(ai->ann) {
					char filename[256];
					snprintf(filename, 256, "%s.net", ai->filename);
					fann_save(ai->ann, filename);
					printf("Saved file %s.\n", filename);
				}
			}
		}
	}

	return 0;
}
