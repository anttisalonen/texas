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

void print_cards(const struct card *cards, int num_cards)
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
		"♠",
		"♥",
		"♦",
		"♣"
	};

	for(int i = 0; i < num_cards; i++) {
		const char* rank = ranks[cards[i].rank];
		const char* suit = suits[cards[i].suit];
		printf("%s%s ", rank, suit);
	}
	printf("\n");
}

void print_money(const struct texas_holdem *th, int all)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th->players[i].decide && (all || th->players[i].active))
			printf("%s: %d\n", th->players[i].name, th->players[i].money);
	}
}

static enum th_decision human_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	while(1) {
		print_money(th, 0);
		printf("Community cards:\n");
		print_cards(th->community_cards, th->num_community_cards);
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

void event_callback(const struct texas_holdem *th, const struct th_event *ev)
{
	switch(ev->type) {
		case TH_EVENT_DECISION:
			switch(ev->decision) {
				case DEC_CHECK:
					printf("%s checks.\n", th->players[ev->player_index].name);
					break;

				case DEC_FOLD:
					printf("%s folds.\n", th->players[ev->player_index].name);
					break;

				case DEC_CALL:
					printf("%s calls.\n", th->players[ev->player_index].name);
					break;

				case DEC_RAISE:
					printf("%s raises by %d. Pot is now %d.\n",
							th->players[ev->player_index].name,
							ev->raise_amount,
							th->pot);
					break;

			}
			break;

		case TH_EVENT_WIN:
			for(int i = 0; i < TH_MAX_PLAYERS; i++) {
				if(!th->players[i].active)
					continue;
				printf("%s has %s (0x%08lx):\n", th->players[i].name,
						ev->best_hands[i].cat->name, ev->best_hands[i].score);
				print_cards(ev->best_hands[i].hand.cards, 5);
			}

			if(ev->num_winners == 1) {
				printf("%s wins the pot of %d with %s.\n", th->players[ev->winner_index[0]].name,
						th->pot,
						ev->winner_hand_name);
			} else {
				printf("The following players split the pot of %d with %s:\n",
						th->pot,
						ev->winner_hand_name);
				for(int i = 0; i < ev->num_winners; i++) {
					printf("\t%s\n", th->players[ev->winner_index[i]].name);
				}
			}
			break;

		case TH_EVENT_BET_ROUND_BEGIN:
			printf("Community cards:\n");
			print_cards(th->community_cards, th->num_community_cards);
			print_money(th, 0);
			break;
	}
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
	th_init(&th, 1, event_callback);
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
	print_money(&th, 1);

	if(do_save) {
		pool_save_current_players(&pool, &th);
	}

	return 0;
}
