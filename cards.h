#pragma once

#include <stdint.h>

#define NUM_POKER_HAND_CARDS 5

enum card_suit {
	SUIT_SPADES,
	SUIT_HEARTS,
	SUIT_DIAMONDS,
	SUIT_CLUBS
};

enum card_rank {
	RANK_TWO,
	RANK_THREE,
	RANK_FOUR,
	RANK_FIVE,
	RANK_SIX,
	RANK_SEVEN,
	RANK_EIGHT,
	RANK_NINE,
	RANK_TEN,
	RANK_JACK,
	RANK_QUEEN,
	RANK_KING,
	RANK_ACE
};

struct card {
	enum card_suit suit;
	enum card_rank rank;
};

struct poker_hand {
	struct card cards[NUM_POKER_HAND_CARDS];
};

#define NUM_DECK_CARDS 52

struct card_deck {
	struct card cards[NUM_DECK_CARDS];
};

struct poker_play {
	struct poker_hand hand;
	uint64_t score;
	const struct poker_hand_category* cat;
};

typedef int (*poker_hand_func)(const struct poker_hand* h);

struct poker_hand_category {
	poker_hand_func func;
	char name[32];
};

struct card deal_card(struct card_deck* d);
void shuffle_card_deck(struct card_deck* d);
void init_card_deck(struct card_deck* d);
void print_cards(const struct card *cards, int num_cards);
const struct poker_hand_category* find_score(const struct poker_hand* h, uint64_t* score);
void sort_poker_hand(struct poker_hand *h);

