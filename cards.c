#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cards.h"

static int get_kicker(const struct card *cards, int num_cards, int selected_cards, int multiplier)
{
	enum card_rank rank = RANK_TWO;
	int kicker = 0;
	int sel_index = -1;
	for(int i = 0; i < NUM_POKER_HAND_CARDS; i++) {
		if(cards[i].rank >= rank) {
			if(!((1 << i) & selected_cards)) {
				rank = cards[i].rank;
				sel_index = i;
			}
		}
	}

	if(sel_index != -1)
		selected_cards |= 1 << sel_index;

	if(num_cards > 1)
		kicker = get_kicker(cards, num_cards - 1, selected_cards, multiplier / 0x10);

	return rank * multiplier + kicker;
}

static int high_card(const struct poker_hand *h)
{
	return get_kicker(h->cards, NUM_POKER_HAND_CARDS, 0, 0x100000);
}

static int find_pair(const struct poker_hand *h, int *selected_cards)
{
	int val = -1;
	int sel_index1, sel_index2;
	for(int i = 0; i < NUM_POKER_HAND_CARDS - 1; i++) {
		if((1 << i) & *selected_cards) {
			continue;
		}
		for(int j = i + 1; j < NUM_POKER_HAND_CARDS; j++) {
			if((1 << j) & *selected_cards) {
				continue;
			}

			if(h->cards[i].rank == h->cards[j].rank) {
				if((int)h->cards[i].rank > val) {
					val = h->cards[i].rank;
					sel_index1 = i;
					sel_index2 = j;
				}
			}
		}
	}
	if(val != -1)
		*selected_cards = (1 << sel_index1) | (1 << sel_index2);

	return val;
}

static int find_three(const struct poker_hand *h, int *selected_cards)
{
	int val = -1;
	int sel_index1, sel_index2, sel_index3;
	for(int i = 0; i < NUM_POKER_HAND_CARDS - 2; i++) {
		if((1 << i) & *selected_cards) {
			continue;
		}
		for(int j = i + 1; j < NUM_POKER_HAND_CARDS - 1; j++) {
			if((1 << j) & *selected_cards) {
				continue;
			}
			if(h->cards[i].rank != h->cards[j].rank) {
				continue;
			}

			for(int k = j + 1; k < NUM_POKER_HAND_CARDS; k++) {
				if((1 << k) & *selected_cards) {
					continue;
				}
				if(h->cards[k].rank != h->cards[j].rank) {
					continue;
				}

				if((int)h->cards[i].rank > val) {
					val = h->cards[i].rank;
					sel_index1 = i;
					sel_index2 = j;
					sel_index3 = k;
				}
			}
		}
	}

	if(val != -1)
		*selected_cards = (1 << sel_index1) |
			(1 << sel_index2) |
			(1 << sel_index3);

	return val;
}

void sort_poker_hand(struct poker_hand *h)
{
	for(int i = 0; i < NUM_POKER_HAND_CARDS - 1; i++) {
		for(int j = i; j < NUM_POKER_HAND_CARDS; j++) {
			if(h->cards[i].rank < h->cards[j].rank) {
				struct card c = h->cards[i];
				h->cards[i] = h->cards[j];
				h->cards[j] = c;
			}
		}
	}
}

static int find_four(const struct poker_hand *h, int *kicker)
{
	struct poker_hand ph = *h;
	sort_poker_hand(&ph);
	if(ph.cards[0].rank == ph.cards[3].rank) {
		*kicker = ph.cards[4].rank;
		return ph.cards[1].rank;
	} else if(ph.cards[1].rank == ph.cards[4].rank) {
		*kicker = ph.cards[0].rank;
		return ph.cards[1].rank;
	}

	return -1;
}

static int one_pair(const struct poker_hand *h)
{
	int selected_cards = 0;
	int val = find_pair(h, &selected_cards);
	
	if(val < 0)
		return val;

	int kicker = get_kicker(h->cards, NUM_POKER_HAND_CARDS - 2, selected_cards, 0x10000);

	return val * 0x100000 + kicker;
}

static int two_pair(const struct poker_hand *h)
{
	int selected_cards = 0;
	int val = find_pair(h, &selected_cards);
	
	if(val < 0)
		return -1;

	int val2 = find_pair(h, &selected_cards);
	
	if(val2 < 0)
		return -1;

	int kicker = get_kicker(h->cards, NUM_POKER_HAND_CARDS - 4, selected_cards, 0x1000);

	return val * 0x100000 + val2 * 0x10000 + kicker;
}

static int three_of_a_kind(const struct poker_hand *h)
{
	int selected_cards = 0;
	int val = find_three(h, &selected_cards);
	
	if(val < 0)
		return val;

	int kicker = get_kicker(h->cards, NUM_POKER_HAND_CARDS - 3, selected_cards, 0x10000);

	return val * 0x100000 + kicker;
}

static int straight_wheel(const struct poker_hand *h)
{
	int have_wheel = h->cards[0].rank == RANK_ACE &&
		h->cards[1].rank == RANK_FIVE &&
		h->cards[2].rank == RANK_FOUR &&
		h->cards[3].rank == RANK_THREE &&
		h->cards[4].rank == RANK_TWO;
	if(have_wheel)
		return RANK_FIVE * 0x100000;
	else
		return -1;
}

static int straight(const struct poker_hand *h)
{
	struct poker_hand ph = *h;
	sort_poker_hand(&ph);
	enum card_rank first = ph.cards[0].rank;
	enum card_rank expected = first - 1;
	for(int i = 1; i < NUM_POKER_HAND_CARDS; i++) {
		if(ph.cards[i].rank != expected)
			return straight_wheel(&ph);
		expected--;
	}

	return first * 0x100000;
}

static int flush(const struct poker_hand *h)
{
	enum card_suit expected = h->cards[0].suit;
	for(int i = 1; i < NUM_POKER_HAND_CARDS; i++) {
		if(h->cards[i].suit != expected)
			return -1;
	}

	int kicker = get_kicker(h->cards, NUM_POKER_HAND_CARDS, 0, 0x10000);
	return 0x100000 + kicker;
}

static int full_house(const struct poker_hand *h)
{
	int selected_cards = 0;
	int val = find_three(h, &selected_cards);
	
	if(val < 0)
		return val;

	int val2 = find_pair(h, &selected_cards);
	
	if(val2 < 0)
		return val2;

	return val * 0x100000 + val2 * 0x10000;
}

static int four_of_a_kind(const struct poker_hand *h)
{
	int kicker = 0;
	int val = find_four(h, &kicker);
	
	if(val < 0)
		return val;

	return val * 0x100000 + kicker;
}

static int straight_flush(const struct poker_hand *h)
{
	int val = straight(h);
	if(val < 0)
		return val;

	int val2 = flush(h);
	if(val2 < 0)
		return -1;

	return val * 0x100000;
}

static const struct poker_hand_category poker_hand_categories[] = {
	{ high_card, "High card" },
	{ one_pair,  "One pair" },
	{ two_pair,  "Two pair" },
	{ three_of_a_kind,  "Three of a kind" },
	{ straight,  "Straight" },
	{ flush,  "Flush" },
	{ full_house,  "Full house" },
	{ four_of_a_kind,  "Four of a kind" },
	{ straight_flush,  "Straight flush" },
};

const struct poker_hand_category* find_score(const struct poker_hand* h, uint64_t* score)
{
	int val = -1;
	int cat = -1;
	for(int i = 0; i < sizeof(poker_hand_categories)/sizeof(poker_hand_categories[0]); i++) {
		int this_val = poker_hand_categories[i].func(h);
		if(this_val >= 0) {
			val = this_val;
			cat = i;
		}
	}
	*score = cat * 0x1000000 + val;
	return &poker_hand_categories[cat];
}

void init_card_deck(struct card_deck* d)
{
	for(int i = RANK_TWO; i <= RANK_ACE; i++) {
		for(int j = SUIT_SPADES; j <= SUIT_CLUBS; j++) {
			d->cards[i + j * (RANK_ACE + 1)].rank = i;
			d->cards[i + j * (RANK_ACE + 1)].suit = j;
		}
	}
}

void shuffle_card_deck(struct card_deck* d)
{
	// Fisher-Yates
	for(int i = NUM_DECK_CARDS - 1; i >= 1; i--) {
		int j = rand() % (i + 1);
		struct card c = d->cards[i];
		d->cards[i] = d->cards[j];
		d->cards[j] = c;
	}
}

struct card deal_card(struct card_deck* d)
{
	struct card c = d->cards[0];
	memmove(&d->cards[0], &d->cards[1], sizeof(d->cards[0]) * (NUM_DECK_CARDS - 1));
	return c;
}


