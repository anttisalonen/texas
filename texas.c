#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "cards.h"
#include "th.h"
#include "pool.h"
#include "ai_config.h"
#include "ncui.h"

enum game_ui {
	UI_NCURSES,
	UI_TEXT
};

struct ai_data {
	char dummy[256];
};

static int human = 0;
static int speed = 5;

static int seed = 0;
static int start_money = 20;
static int max_rounds = -1;
static int do_save = 0;

static enum game_ui ui = UI_NCURSES;

static int ai_params_pos[32];
static char ai_filenames[32][POOL_MAX_PLAYERS][256];
static int num_ais[32];

static struct ai_data ais[POOL_MAX_PLAYERS];

static struct player_pool pool;

static struct texas_holdem th;

static int human_pool_func(void *data)
{
	return 1;
}

void event_auto_callback(const struct texas_holdem *th, const struct th_event *ev)
{
}

static int ai_ind(const char *n)
{
	int index = get_ai_config_index(n);
	if(index == -1) {
		fprintf(stderr, "Unknown AI: %s\n", n);
		exit(1);
	}
	return index;
}

static void ai_create_filename(char *to, const char *pattern)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	snprintf(to, 255, "ais/%s_%02d%02d%02d%04ld",
			pattern, tm.tm_hour,
			tm.tm_min, tm.tm_sec, tv.tv_usec / 1000);
}

static void modify_ai_filename(char *to)
{
	char buf[256];
	char *dot = strrchr(to, '_');
	assert(dot);
	*dot = '\0';
	int ret = sscanf(to, "ais/%s", buf);
	assert(ret == 1);
	ai_create_filename(to, buf);
}

static void generate_ai_filename(char *filename)
{
	char randstr[9];
	memset(randstr, 0x00, sizeof(randstr));
	for(int j = 0; j < 8; j++) {
		randstr[j] = rand() % 20 + 'a';
	}
	ai_create_filename(filename, randstr);
}

void parse_params(int argc, char **argv)
{
	seed = time(NULL);

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
		else if(!strncmp(argv[i], "--ai_num_", 9)) {
			int ind = ai_ind(argv[i] + 9);
			num_ais[ind] = atoi(argv[++i]);
		}
		else if(!strncmp(argv[i], "--ai_file_", 10)) {
			int ind = ai_ind(argv[i] + 10);
			strncpy(ai_filenames[ind][ai_params_pos[ind]++], argv[++i], 256);
		}
		else if(!strcmp(argv[i], "--save")) {
			do_save = 1;
		}
		else if(!strcmp(argv[i], "--speed")) {
			speed = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--ui")) {
			ui = atoi(argv[++i]);
		} else {
			fprintf(stderr, "Unknown argument '%s'.\n", argv[i]);
			exit(1);
		}
	}
}

void init_ui()
{
	switch(ui) {
		case UI_NCURSES:
			ncui_init();
			ncui_set_speed(speed);
			break;

		case UI_TEXT:
			if(human) {
				fprintf(stderr, "Cannot play in text UI mode.\n");
				exit(1);
			}
			printf("Random seed: %d\n", seed);
			break;
	}
}

void init_game()
{
	srand(seed);

	int data_pos = 0;

	pool_init(&pool);
	if(human) {
		pool_add_player(&pool, start_money, human_decision, human_pool_func, NULL, "human");
		data_pos++;
	}

	{
		int ind = 0;
		while(1) {
			struct ai_config *conf = get_ai_config_by_index(ind);
			if(!conf->ai_name)
				break;

			for(int i = 0; i < num_ais[ind]; i++) {
				if(!ai_filenames[ind][i][0]) {
					generate_ai_filename(ai_filenames[ind][i]);
					conf->ai_init_func(&ais[i + data_pos]);
				} else {
					assert(conf->ai_load_func);
					printf("Loading AI type \"%s\" from %s\n",
							conf->ai_name,
							ai_filenames[ind][i]);
					conf->ai_load_func(&ais[i + data_pos],
						ai_filenames[ind][i]);
					if(conf->ai_modify_func && rand() % 2 == 0) {
						printf("Modifying %s\n",
								ai_filenames[ind][i]);
						modify_ai_filename(ai_filenames[ind][i]);
						conf->ai_modify_func(&ais[i + data_pos], 0.01f);
					}
				}
				pool_add_player(&pool, start_money,
						conf->ai_decision_func, conf->ai_pool_decision_func,
						&ais[i + data_pos], conf->ai_name);
			}
			data_pos += num_ais[ind];
			ind++;
		}
	}

	th_init(&th, 1, ui == UI_NCURSES ? event_callback : event_auto_callback);

	struct pool_update pupd;
	pool_update_th(&pool, &th, &pupd);
}

void run_game()
{
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
		struct pool_update pupd;
		pool_update_th(&pool, &th, &pupd);
		if(human) {
			for(int i = 0; i < TH_MAX_PLAYERS; i++) {
				if(pupd.seats[i].player &&
						pupd.seats[i].status == POOL_SEAT_FREED &&
						!strcmp(pupd.seats[i].player, "0 (huma")) {
					human = 0;
				}
			}
		}
	}

	if(ui != UI_NCURSES) {
		printf("Final score after %d rounds:\n", num_rounds);
		for(int i = 0; i < TH_MAX_PLAYERS; i++) {
			if(th.players[i].decide) {
				printf("%s: %d\n", th.players[i].name, th.players[i].money);
			}
		}
	}
}

void finish_game()
{
	if(do_save) {
		int ind = 0;
		int data_pos = 0;
		while(1) {
			struct ai_config *conf = get_ai_config_by_index(ind);
			if(!conf->ai_name)
				break;

			for(int i = 0; i < num_ais[ind]; i++) {
				printf("Saving AI type \"%s\" to %s\n", conf->ai_name,
						ai_filenames[ind][i]);
				conf->ai_save_func(&ais[i + data_pos],
						ai_filenames[ind][i]);
			}
			data_pos += num_ais[ind];
			ind++;
		}
	}
}

void deinit_ui()
{
	if(ui == UI_NCURSES) {
		ncui_deinit();
	}
}

int main(int argc, char **argv)
{
	parse_params(argc, argv);
	init_ui();
	init_game();
	run_game();
	finish_game();
	deinit_ui();
	return 0;
}


