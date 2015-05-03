#pragma once

#include "cards.h"

#define TH_MAX_PLAYERS 10

#define TH_MAX_PLAYER_NAME_LEN 32

enum th_decision {
	DEC_CHECK,
	DEC_FOLD,
	DEC_CALL,
	DEC_RAISE
};

struct texas_holdem;

typedef enum th_decision (*th_decision_func)(const struct texas_holdem *th, int plnum, int raised_to, void *data);

struct poker_player {
	char name[TH_MAX_PLAYER_NAME_LEN];
	int money;
	int paid_bet;
	int active;
	struct card hole_cards[2];
	th_decision_func decide;
	void *decision_data;
};

struct texas_holdem {
	int pot;
	int small_blind;
	struct card_deck deck;
	struct poker_player players[TH_MAX_PLAYERS];
	struct card community_cards[5];
	int num_community_cards;
	int dealer_pos;
};

void th_print_money(const struct texas_holdem *th, int all);
void th_print_community_cards(const struct texas_holdem *th);

void th_init(struct texas_holdem* th, int small_blind);
int th_add_player(struct texas_holdem* th, const char* name, int money, th_decision_func fun, void *decision_data);
int th_remove_player(struct texas_holdem* th, int index);
int th_play_hand(struct texas_holdem *th);

