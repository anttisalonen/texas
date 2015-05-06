#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <poll.h>

#include <ncurses.h>

#include "cards.h"
#include "th.h"
#include "pool.h"
#include "ai_config.h"

#define PROMPT_Y 30
#define PROMPT_X 1

static int human = 0;
static int speed = 5;

enum game_ui {
	UI_NCURSES,
	UI_TEXT
};

static void print_cards(int y, int x, const struct card *cards, int num_cards)
{
	static const char* ranks[] = {
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"10",
		"J",
		"Q",
		"K",
		"A"
	};

	static const char* suits[] = {
		"Spades",
		"Hearts",
		"Diamonds",
		"Clubs"
	};

	for(int i = 0; i < num_cards; i++) {
		const char* rank = ranks[cards[i].rank];
		const char* suit = suits[cards[i].suit];
		mvprintw(y + i, x, "%s of %s", rank, suit);
	}
}

static void print_community_cards(const struct texas_holdem *th)
{
	print_cards(10, 11, th->community_cards, th->num_community_cards);
}

static void print_pot(const struct texas_holdem *th)
{
	mvprintw(16, 16, "Pot: %d", th->pot);
}

static enum th_decision human_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);

static void get_player_coordinates(int plnum, int *y, int *x)
{
	*x = 5 + (plnum % 5) * 16;
	*y = plnum < 5 ? 2 : 20;
}

static void print_player(const struct texas_holdem *th, int player_index, int show_all_holes)
{
	int x, y;
	get_player_coordinates(player_index, &y, &x);

	mvprintw(y,     x, "%s", th->players[player_index].name);
	mvprintw(y + 1, x, "Money: %d   ", th->players[player_index].money);
	if(show_all_holes || th->players[player_index].decide == human_decision) {
		print_cards(y + 2, x, th->players[player_index].hole_cards, 2);
	}
	if(!th->players[player_index].active) {
		mvprintw(y + 4, x, "folds");
	}
}

static void print_players(const struct texas_holdem *th, int show_all_holes)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(!th->players[i].decide)
			continue;

		print_player(th, i, show_all_holes);
	}
}

static enum th_decision human_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	while(1) {
		print_players(th, !human);
		print_community_cards(th);

		if(raised_to)
			mvprintw(PROMPT_Y, PROMPT_X, "(f)old, c(a)ll, (r)aise?\n");
		else
			mvprintw(PROMPT_Y, PROMPT_X, "(c)heck, (r)aise?\n");
		refresh();

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
			default:
				break;
		}
	}
}

static int human_pool_func(void *data)
{
	return 1;
}

void print_end_of_round(const struct texas_holdem *th, const struct th_event *ev, int wait)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(!th->players[i].active)
			continue;
		print_player(th, i, 1);
		if(wait) {
			poll(NULL, 0, 100 * speed);
			refresh();
		}
		int y, x;
		get_player_coordinates(i, &y, &x);
		mvprintw(y + 4, x, "%s", ev->best_hands[i].cat->name);
		if(wait) {
			poll(NULL, 0, 100 * speed);
			refresh();
		}
	}
	if(!wait)
		refresh();
}

void event_auto_callback(const struct texas_holdem *th, const struct th_event *ev)
{
}

void event_callback(const struct texas_holdem *th, const struct th_event *ev)
{
	int x, y;
	get_player_coordinates(ev->player_index, &y, &x);
	clear();
	print_players(th, !human);
	print_community_cards(th);
	print_pot(th);

	switch(ev->type) {
		case TH_EVENT_DECISION:
			switch(ev->decision) {
				case DEC_CHECK:
					mvprintw(y + 4, x, "checks");
					break;

				case DEC_FOLD:
					mvprintw(y + 4, x, "folds");
					break;

				case DEC_CALL:
					mvprintw(y + 4, x, "calls");
					break;

				case DEC_RAISE:
					mvprintw(y + 4, x, "raises by %d", ev->raise_amount);
					break;
			}
			break;

		case TH_EVENT_END_OF_ROUND:
			print_end_of_round(th, ev, 1);
			break;

		case TH_EVENT_WIN:
			print_end_of_round(th, ev, 0);
			for(int i = 0; i < ev->num_winners; i++) {
				get_player_coordinates(ev->winner_index[i], &y, &x);
				mvprintw(y + 5, x, "wins %d",
						ev->winner_money[i]);
			}

			poll(NULL, 0, 100 * speed);
			refresh();
			{
				mvprintw(PROMPT_Y, PROMPT_X, "Hit Enter to continue");
				char buf[256];
				refresh();
				read(STDIN_FILENO, buf, 256);
			}
			break;

		case TH_EVENT_BET_ROUND_BEGIN:
			print_community_cards(th);
			print_players(th, !human);
			poll(NULL, 0, 100 * speed);
			refresh();
			break;
	}
	poll(NULL, 0, 100 * speed);
	refresh();
}

int ai_ind(const char *n)
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

int main(int argc, char **argv)
{
	int seed = time(NULL);
	int start_money = 20;
	int max_rounds = -1;
	int do_save = 0;

	enum game_ui ui = UI_NCURSES;

	int ai_params_pos[32];
	memset(ai_params_pos, 0x00, sizeof(ai_params_pos));
	char ai_filenames[32][POOL_MAX_PLAYERS][256];
	memset(ai_filenames, 0x00, sizeof(ai_filenames));
	int num_ais[32];
	memset(num_ais, 0x00, sizeof(num_ais));

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

	switch(ui) {
		case UI_NCURSES:
			initscr();
			noecho();
			curs_set(0);
			mvprintw(1, 0, "Random seed: %d", seed);
			break;

		case UI_TEXT:
			if(human) {
				fprintf(stderr, "Cannot play in text UI mode.\n");
				exit(1);
			}
			printf("Random seed: %d\n", seed);
			break;
	}

	srand(seed);

	struct player_pool pool;

	int data_pos = 0;

	struct ai_data {
		char dummy[256];
	};

	struct ai_data ais[POOL_MAX_PLAYERS];

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

	struct texas_holdem th;
	th_init(&th, 1, ui == UI_NCURSES ? event_callback : event_auto_callback);

	struct pool_update pupd;
	pool_update_th(&pool, &th, &pupd);

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

	if(do_save) {
		int ind = 0;
		data_pos = 0;
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

	if(ui == UI_NCURSES) {
		endwin();
	} else {
		printf("Final score after %d rounds:\n", num_rounds);
		for(int i = 0; i < TH_MAX_PLAYERS; i++) {
			if(th.players[i].decide) {
				printf("%s: %d\n", th.players[i].name, th.players[i].money);
			}
		}
	}

	return 0;
}
