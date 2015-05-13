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
#include "pdb.h"

#define AI_TMP_FILENAME ".ai_ld.tmp"

enum game_ui {
	UI_NCURSES,
	UI_TEXT
};

static char *include_human;
static int speed = 5;

static int seed = 0;
static int start_money = 20;
static int max_rounds = -1;

static enum game_ui ui = UI_NCURSES;

static int num_players;

static struct dummy_ai_data ais[POOL_MAX_PLAYERS];

static struct player_pool pool;

static struct texas_holdem th;

static DB *db;

static int human_pool_func(void *data)
{
	return 1;
}

static enum th_decision human_text_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	return DEC_RAISE;
}

static void event_callback(const struct texas_holdem *th, const struct th_event *ev)
{
	if(ui == UI_NCURSES) {
		ncui_event_callback(th, ev);
	}
}

void parse_params(int argc, char **argv)
{
	seed = time(NULL);

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "--seed")) {
			seed = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--include_human")) {
			include_human = argv[++i];
		}
		else if(!strcmp(argv[i], "--money")) {
			start_money = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--rounds")) {
			max_rounds = atoi(argv[++i]);
		}
		else if(!strcmp(argv[i], "--players")) {
			num_players = atoi(argv[++i]);
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

int init_ui(void)
{
	switch(ui) {
		case UI_NCURSES:
			ncui_init();
			ncui_set_speed(speed);
			ncui_set_human(!!include_human);
			break;

		case UI_TEXT:
			printf("Random seed: %d\n", seed);
			break;
	}
	return 0;
}

void init_game(void)
{
	pool_init(&pool);

	// Reservoir sampling
	struct db_player *players;
	char *available_players;

	db = db_open();
	srand(seed);

	int num_db_entries = get_db_contents(db, &available_players, &players);
	char available_ai_players[num_db_entries][TH_MAX_PLAYER_NAME_LEN];
	memset(available_ai_players, 0x00, sizeof(available_ai_players));
	int num_ai_players = 0;

	if(num_players > num_db_entries) {
		fprintf(stderr, "You asked for %d players but only %d are available in the database.\n",
				num_players, num_db_entries);
		free(available_players);
		free(players);
		exit(1);
	}

	for(int i = 0; i < num_db_entries; i++) {
		if(strcmp(players[i].type, "human")) {
			strncpy(available_ai_players[num_ai_players],
					&available_players[i * TH_MAX_PLAYER_NAME_LEN],
					TH_MAX_PLAYER_NAME_LEN - 1);
			num_ai_players++;
		}
	}

	free(players);
	free(available_players);

	char selected_players[num_players][TH_MAX_PLAYER_NAME_LEN];
	memset(selected_players, 0x00, sizeof(selected_players));

	for(int i = 0; i < num_players; i++) {
		if(i >= num_ai_players) {
			fprintf(stderr, "You asked for %d players but only %d AI are available in the database.\n",
					num_players, num_ai_players);
			exit(1);
		}
		strncpy(selected_players[i],
				available_ai_players[i],
				TH_MAX_PLAYER_NAME_LEN - 1);
	}
	for(int i = num_players; i < num_ai_players; i++) {
		int j = rand() % (i + 1);
		if(j < num_ai_players) {
			strncpy(selected_players[j],
					available_ai_players[i],
					TH_MAX_PLAYER_NAME_LEN - 1);
		}
	}

	if(include_human) {
		strncpy(selected_players[0], include_human, TH_MAX_PLAYER_NAME_LEN - 1);
	}

	ai_config_init();
	for(int i = 0; i < num_players; i++) {
		struct db_player pp;
		int ret = db_get_player(db, selected_players[i], &pp, AI_TMP_FILENAME);
		if(ret) {
			fprintf(stderr, "Couldn't find player \"%s\" in the database.\n",
					selected_players[i]);
			exit(1);
		}

		int human = !strcmp(pp.type, "human");

		if(human && i != 0) {
			/* Could be fixed by creating an input array for reservoir sampling */
			fprintf(stderr, "I don\'t know how to handle randomly picked another human player. Exiting.\n");
			exit(1);
		}

		if(human) {
			pool_add_player(&pool, start_money,
					ui == UI_NCURSES ? human_decision : human_text_decision,
					human_pool_func, NULL,
					selected_players[i], "human");
		} else {
			struct ai_config *conf = get_ai_config(pp.type);
			assert(conf);
			if(conf->ai_load_func) {
				int ret = conf->ai_load_func(&ais[i], AI_TMP_FILENAME);
				assert(!ret);
			}
			pool_add_player(&pool, start_money,
					conf->ai_decision_func, conf->ai_pool_decision_func,
					&ais[i], selected_players[i], conf->ai_name);
			if(conf->ai_load_func && unlink(AI_TMP_FILENAME)) {
				perror("unlink");
				assert(0);
			}
		}

#if 0
		conf->ai_init_func(&ais[i + data_pos]);
		assert(conf->ai_load_func);
		conf->ai_load_func(&ais[i + data_pos]);
		if(conf->ai_modify_func && rand() % 2 == 0)
			conf->ai_modify_func(&ais[i + data_pos], 0.01f);
#endif
	}

	th_init(&th, 1, event_callback);

	struct pool_update pupd;
	pool_update_th(&pool, &th, &pupd);
}

void run_game(void)
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
		if(include_human) {
			for(int i = 0; i < TH_MAX_PLAYERS; i++) {
				if(pupd.seats[i].player &&
						pupd.seats[i].status == POOL_SEAT_FREED &&
						!strncmp(pupd.seats[i].player, include_human, strlen(include_human))) {
					include_human = NULL;
					ncui_set_human(0);
					break;
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

void finish_game(void)
{
	for(int i = 0; i < pool.num_occupants; i++) {
		int ret = db_update_player(db, pool.occupants[i].real_name,
				pool.occupants[i].hands_dealt,
				pool.occupants[i].money - start_money);
		assert(!ret);
	}
	db_close(db);
}

void deinit_ui(void)
{
	if(ui == UI_NCURSES) {
		ncui_deinit();
	}
}

int main(int argc, char **argv)
{
	parse_params(argc, argv);
	init_game();
	if(!init_ui()) {
		run_game();
		finish_game();
		deinit_ui();
	}
	return 0;
}


