#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>

#include <floatfann.h>

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

#define NUM_POKER_HAND_CARDS 5

struct poker_hand {
	struct card cards[NUM_POKER_HAND_CARDS];
};

const char* rank_to_str(enum card_rank r)
{
	static const char* ranks[] = {
		"Two",
		"Three",
		"Four",
		"Five",
		"Six",
		"Seven",
		"Eight",
		"Nine",
		"Ten",
		"Jack",
		"Queen",
		"King",
		"Ace"
	};

	return ranks[r];
}

int get_kicker(const struct card *cards, int num_cards, int selected_cards, int multiplier)
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

int high_card(const struct poker_hand *h)
{
	return get_kicker(h->cards, NUM_POKER_HAND_CARDS, 0, 0x100000);
}

int find_pair(const struct poker_hand *h, int *selected_cards)
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

int find_three(const struct poker_hand *h, int *selected_cards)
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

int find_four(const struct poker_hand *h, int *kicker)
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

int one_pair(const struct poker_hand *h)
{
	int selected_cards = 0;
	int val = find_pair(h, &selected_cards);
	
	if(val < 0)
		return val;

	int kicker = get_kicker(h->cards, NUM_POKER_HAND_CARDS - 2, selected_cards, 0x10000);

	return val * 0x100000 + kicker;
}

int two_pair(const struct poker_hand *h)
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

int three_of_a_kind(const struct poker_hand *h)
{
	int selected_cards = 0;
	int val = find_three(h, &selected_cards);
	
	if(val < 0)
		return val;

	int kicker = get_kicker(h->cards, NUM_POKER_HAND_CARDS - 3, selected_cards, 0x10000);

	return val * 0x100000 + kicker;
}

int straight_wheel(const struct poker_hand *h)
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

int straight(const struct poker_hand *h)
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

int flush(const struct poker_hand *h)
{
	enum card_suit expected = h->cards[0].suit;
	for(int i = 1; i < NUM_POKER_HAND_CARDS; i++) {
		if(h->cards[i].suit != expected)
			return -1;
	}

	int kicker = get_kicker(h->cards, NUM_POKER_HAND_CARDS, 0, 0x10000);
	return 0x100000 + kicker;
}

int full_house(const struct poker_hand *h)
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

int four_of_a_kind(const struct poker_hand *h)
{
	int kicker = 0;
	int val = find_four(h, &kicker);
	
	if(val < 0)
		return val;

	return val * 0x100000 + kicker;
}

int straight_flush(const struct poker_hand *h)
{
	int val = straight(h);
	if(val < 0)
		return val;

	int val2 = flush(h);
	if(val2 < 0)
		return -1;

	return val * 0x100000;
}

typedef int (*poker_hand_func)(const struct poker_hand* h);

struct poker_hand_category {
	poker_hand_func func;
	char name[32];
};

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

#define NUM_DECK_CARDS 52

struct card_deck {
	struct card cards[NUM_DECK_CARDS];
};

void init_card_deck(struct card_deck* d)
{
	for(int i = RANK_TWO; i <= RANK_ACE; i++) {
		for(int j = SUIT_SPADES; j <= SUIT_CLUBS; j++) {
			d->cards[i + j * (RANK_ACE + 1)].rank = i;
			d->cards[i + j * (RANK_ACE + 1)].suit = j;
		}
	}
}

void print_card(const struct card* c)
{
	static const char* suits[] = {
		"Spades",
		"Hearts",
		"Diamonds",
		"Clubs"
	};

	const char* rank = rank_to_str(c->rank);
	const char* suit = suits[c->suit];
	printf("%s of %s\n", rank, suit);
}

void shuffle_card_deck(struct card_deck* d)
{
	for(int i = 0; i < NUM_DECK_CARDS; i++) {
		int j = rand() % NUM_DECK_CARDS;
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

void deal_poker_hand(struct card_deck* d, struct poker_hand* h)
{
	for(int i = 0; i < NUM_POKER_HAND_CARDS; i++) {
		h->cards[i] = d->cards[i];
	}
	memmove(&d->cards[0], &d->cards[NUM_POKER_HAND_CARDS], sizeof(d->cards[0]) * (NUM_DECK_CARDS - NUM_POKER_HAND_CARDS));
}

struct poker_play {
	struct poker_hand hand;
	uint64_t score;
	const struct poker_hand_category* cat;
};

void get_poker_play(struct poker_play *p)
{
	sort_poker_hand(&p->hand);
	p->cat = find_score(&p->hand, &p->score);
}

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

#define TH_MAX_PLAYERS 10

#define TH_MAX_PLAYER_NAME_LEN 32

enum th_decision {
	DEC_CHECK,
	DEC_FOLD,
	DEC_CALL,
	DEC_RAISE
};

const char *dec_to_str(enum th_decision d)
{
	switch(d) {
		case DEC_CHECK:
			return "checks";
		case DEC_FOLD:
			return "folds";
		case DEC_CALL:
			return "calls";
		case DEC_RAISE:
			return "raises";
		default:
			return "?";
	}
}

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


void th_print_money(const struct texas_holdem *th, int all)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th->players[i].decide && (all || th->players[i].active))
			printf("%s: %d\n", th->players[i].name, th->players[i].money);
	}
}

void th_print_community_cards(const struct texas_holdem *th)
{
	printf("Community cards:\n");
	print_cards(th->community_cards, th->num_community_cards);
}

enum th_decision human_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
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

enum th_decision random_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
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

struct ai_data {
	struct fann *ann;
	int num_rounds_played;
	char filename[32];
};

enum th_decision ann_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	fann_type *calc_out;
	fann_type input[RANK_ACE + 1];

	for(int j = RANK_TWO; j < RANK_ACE; j++) {
		input[j] = 0.0f;
		for(int i = 0; i < 2; i++) {
			if(th->players[plnum].hole_cards[i].rank == j)
				input[j] += 0.25f;
		}
		for(int i = 0; i < th->num_community_cards; i++) {
			if(th->community_cards[i].rank == j)
				input[j] += 0.25f;
		}
	}

	input[RANK_ACE] = (2.0f * (th->pot / (1.0f + th->players[plnum].money * 0.5f))) - 1.0f;
	if(input[RANK_ACE] > 1.0f)
		input[RANK_ACE] = 1.0f;

	struct ai_data *d = (struct ai_data *)data;
	calc_out = fann_run(d->ann, input);

	if(calc_out[0] > 0.333f)
		return DEC_RAISE;
	else if(raised_to > 0 && calc_out[0] < -0.333f)
		return DEC_FOLD;
	else if(raised_to)
		return DEC_CALL;
	else
		return DEC_CHECK;
}

void th_init(struct texas_holdem* th, int small_blind)
{
	memset(th, 0x00, sizeof(*th));
	th->small_blind = small_blind;
}

int th_add_player(struct texas_holdem* th, const char* name, int money, th_decision_func fun, void *decision_data)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th->players[i].money == 0) {
			strncpy(th->players[i].name, name, TH_MAX_PLAYER_NAME_LEN);
			th->players[i].name[TH_MAX_PLAYER_NAME_LEN - 1] = '\0';
			th->players[i].money = money;
			th->players[i].active = 1;
			th->players[i].decide = fun;
			th->players[i].decision_data = decision_data;
			return i;
		}
	}
	return -1;
}

int th_remove_player(struct texas_holdem* th, int index)
{
	th->players[index].active = 0;
	th->players[index].money = 0;
	th->players[index].name[0] = '\0';
	th->players[index].decide = NULL;
	return 0;
}

int th_get_num_active_players(const struct texas_holdem *th)
{
	int num_players = 0;
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th->players[i].active)
			num_players++;
	}
	return num_players;
}

void th_clear_paid_bets(struct texas_holdem *th)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		th->players[i].paid_bet = 0;
	}
}

int th_setup(struct texas_holdem *th)
{
	th->pot = 0;
	th_clear_paid_bets(th);
	init_card_deck(&th->deck);
	shuffle_card_deck(&th->deck);
	th->num_community_cards = 0;
	th->dealer_pos++;
	if(th->dealer_pos >= TH_MAX_PLAYERS)
		th->dealer_pos = 0;

	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th->players[i].decide)
			th->players[i].active = 1;
		if(th->players[i].money < th->small_blind * 2)
			th->players[i].active = 0;
		if(!th->players[i].decide)
			th->players[i].active = 0;
	}

	if(th_get_num_active_players(th) < 4)
		return -1;

	return 0;
}

void th_bet(struct texas_holdem* th, unsigned int plnum, int bet)
{
	assert(th->players[plnum].active);
	if(th->players[plnum].money < bet) {
		bet = th->players[plnum].money;
	} else {
		bet -= th->players[plnum].paid_bet;
	}
	th->players[plnum].money -= bet;
	th->pot += bet;
	th->players[plnum].paid_bet += bet;
	printf("%s posts a bet of %d. Pot is now %d.\n",
			th->players[plnum].name, bet,
			th->pot);
}

void th_deal_hole_cards(struct texas_holdem* th)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		for(int j = 0; j < 2; j++) {
			th->players[i].hole_cards[j] = deal_card(&th->deck);
		}
	}
}

int th_get_decision(struct texas_holdem *th, int plnum, int raised_to, int bet_amount)
{
	if(!th->players[plnum].money)
		return raised_to;

	enum th_decision dec = th->players[plnum].decide(th, plnum, raised_to - th->players[plnum].paid_bet,
			th->players[plnum].decision_data);
	printf(" *** %s %s\n", th->players[plnum].name, dec_to_str(dec));

	switch(dec) {
		case DEC_CHECK:
			assert(!raised_to - th->players[plnum].paid_bet); /* check not allowed */
			break;

		case DEC_FOLD:
			th->players[plnum].active = 0;
			break;

		case DEC_CALL:
			th_bet(th, plnum, raised_to);
			break;

		case DEC_RAISE:
			th_bet(th, plnum, raised_to + bet_amount);
			raised_to += bet_amount;
			break;
	}
	return raised_to;
}

void th_betting_round(struct texas_holdem *th, int first_in_line, unsigned int bet_amount, unsigned int min_bet)
{
	if(!min_bet)
		th_clear_paid_bets(th);

	if(th_get_num_active_players(th) < 2)
		return;

	int raised_to;

	th_print_money(th, 0);
	raised_to = min_bet;
	min_bet = 0;
	int first_round = 1;

	int i = first_in_line;
	while(first_round || i != first_in_line) {
		if(th->players[i].active) {
			int old_raised_to = raised_to;
			raised_to = th_get_decision(th, i, raised_to, bet_amount);
			if(raised_to > old_raised_to) {
				first_in_line = i;
				first_round = 0;
			}
		}

		if(first_round && i == first_in_line)
			first_round = 0;

		i++;
		if(i == TH_MAX_PLAYERS)
			i = 0;
	}
}

int th_pl_index(struct texas_holdem *th, int pos)
{
	int i = th->dealer_pos;
	do {
		i++;
		if(i == TH_MAX_PLAYERS)
			i = 0;
		if(th->players[i].active)
			pos--;
		if(pos <= 0)
			return i;
	} while(i != th->dealer_pos);
	assert(0);
}

void th_first_betting_round(struct texas_holdem* th)
{
	th_bet(th, th_pl_index(th, 1), th->small_blind);
	th_bet(th, th_pl_index(th, 2), th->small_blind * 2);
	th_betting_round(th, th_pl_index(th, 3), th->small_blind * 2, th->small_blind * 2);
}

void th_deal_community(struct texas_holdem *th, int num_cards)
{
	for(int i = 0; i < num_cards; i++) {
		struct card c = deal_card(&th->deck);
		th->community_cards[th->num_community_cards] = c;
		th->num_community_cards++;
	}
	th_print_community_cards(th);
}

struct hand_permutations {
	struct poker_hand permutations[21];
	int index;
};

void permutations_init(struct hand_permutations *perms)
{
	perms->index = 0;
}

void permutations_add(struct hand_permutations *perms, struct card *cards)
{
	for(int i = 0; i < NUM_POKER_HAND_CARDS; i++) {
		perms->permutations[perms->index].cards[i] = cards[i];
	}
	perms->index++;
	assert(perms->index <= 21);
}

void permute(struct card *cards, struct hand_permutations *perms)
{
	for(int i = 0; i < 6; i++) {
		for(int j = i + 1; j < 7; j++) {
			int index = 0;
			struct card mycards[5];
			for(int k = 0; k < 7; k++) {
				if(k != i && k != j)
					mycards[index++] = cards[k];
			}
			permutations_add(perms, mycards);
		}
	}

}

void th_showdown(struct texas_holdem *th)
{
	uint64_t best_score = 0;
	int best_player = 0;
	int tied_players = 0;
	struct poker_play best_hands[TH_MAX_PLAYERS];
	memset(best_hands, 0x00, sizeof(best_hands));

	for(int pl = 0; pl < TH_MAX_PLAYERS; pl++) {
		if(!th->players[pl].active)
			continue;

		struct hand_permutations perms;
		permutations_init(&perms);
		struct card cards[7];
		for(int i = 0; i < 5; i++) {
			cards[i] = th->community_cards[i];
		}
		for(int i = 0; i < 2; i++) {
			cards[i + 5] = th->players[pl].hole_cards[i];
		}

		permute(cards, &perms);
		for(int i = 0; i < 21; i++) {
			struct poker_play this_p;
			this_p.hand = perms.permutations[i];
			this_p.cat = find_score(&this_p.hand, &this_p.score);
			if(this_p.score > best_hands[pl].score) {
				best_hands[pl] = this_p;
				if(this_p.score > best_score) {
					best_score = this_p.score;
					best_player = pl;
					tied_players = 0;
				} else if(this_p.score == best_score) {
					tied_players |= 1 << best_player;
					tied_players |= 1 << pl;
				}
			}
		}
		sort_poker_hand(&best_hands[pl].hand);
		printf("%s has %s (0x%08lx):\n", th->players[pl].name, best_hands[pl].cat->name, best_hands[pl].score);
		print_cards(best_hands[pl].hand.cards, 5);
	}

	if(!tied_players) {
		printf("%s wins the pot of %d with %s.\n", th->players[best_player].name,
				th->pot,
				best_hands[best_player].cat->name);
		th->players[best_player].money += th->pot;
	} else {
		int num_tied_players = 0;
		printf("The following players split the pot of %d with %s:\n",
				th->pot,
				best_hands[best_player].cat->name);
		for(int i = 0; i < TH_MAX_PLAYERS; i++) {
			if((1 << i) & tied_players) {
				printf("\t%s\n", th->players[i].name);
				num_tied_players++;
			}
		}
		int split_pot = th->pot / num_tied_players;
		for(int i = 0; i < TH_MAX_PLAYERS; i++) {
			if((1 << i) & tied_players) {
				num_tied_players--;
				if(num_tied_players == 0) {
					th->players[i].money += th->pot;
				} else {
					th->players[i].money += split_pot;
					th->pot -= split_pot;
				}
			}
		}
	}
}

int th_play_hand(struct texas_holdem *th)
{
	if(th_setup(th))
		return -1;

	th_deal_hole_cards(th);
	th_first_betting_round(th);
	th_deal_community(th, 3);
	th_betting_round(th, th_pl_index(th, 1), th->small_blind * 2, 0);
	th_deal_community(th, 1);
	th_betting_round(th, th_pl_index(th, 1), th->small_blind * 4, 0);
	th_deal_community(th, 1);
	th_betting_round(th, th_pl_index(th, 1), th->small_blind * 4, 0);

	th_showdown(th);

	return 0;
}

#define POOL_MAX_PLAYERS 10240

typedef int (*pool_decision_func)(void *data);

int human_pool_func(void *data)
{
	return 1;
}

int ai_pool_func(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	d->num_rounds_played++;
	if(d->num_rounds_played >= 10) {
		d->num_rounds_played = 0;
		return 0;
	}
	return 1;
}


struct pool_occupant {
	char name[TH_MAX_PLAYER_NAME_LEN];
	int money;
	int table_pos;
	int left_rounds;
	th_decision_func decide;
	pool_decision_func pool_func;
	void *decision_data;
};

struct player_pool {
	struct pool_occupant occupants[POOL_MAX_PLAYERS];
	int num_occupants;
	int sitters[TH_MAX_PLAYERS];
	int next_attending;
};

void pool_init(struct player_pool *pool)
{
	memset(pool, 0x00, sizeof(*pool));
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		pool->sitters[i] = -1;
	}
}

void pool_add_player(struct player_pool *pool, int start_money,
		th_decision_func decide,
		pool_decision_func pool_func,
		void *data, const char *type)
{
	assert(pool->num_occupants < POOL_MAX_PLAYERS);

	struct pool_occupant* occ = &pool->occupants[pool->num_occupants];
	snprintf(occ->name, TH_MAX_PLAYER_NAME_LEN, "%d (%s)", pool->num_occupants, type);
	occ->money = start_money;
	occ->table_pos = -1;
	occ->pool_func = pool_func;
	occ->decide = decide;
	occ->decision_data = data;

	pool->num_occupants++;
}

void pool_update_th(struct player_pool *pool, struct texas_holdem *th)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th->players[i].decide) {
			struct pool_occupant *occ = &pool->occupants[pool->sitters[i]];
			occ->money = th->players[i].money;
			printf("POOL: Player %s now has %d money.\n", occ->name, occ->money);
		}
	}

	int free_spots = 0;
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		struct pool_occupant *occ = &pool->occupants[pool->sitters[i]];

		if(!th->players[i].decide) {
			assert(pool->sitters[i] == -1);
			free_spots++;
			printf("POOL: seat %d is free.\n", i);
		}
		else if(th->players[i].decide &&
				 (th->players[i].money < th->small_blind * 2 ||
					!occ->pool_func(occ->decision_data))) {
			assert(occ->table_pos == i);
			assert(pool->sitters[i] != -1);
			th_remove_player(th, i);
			free_spots++;
			printf("POOL: Player %s got up from seat %d.\n", occ->name, i);
			occ->table_pos = -1;
			pool->sitters[i] = -1;
		}
	}

	for(int i = 0; i < free_spots; i++) {
		int start_attending = pool->next_attending;
		do {
			int this_pos = pool->next_attending;
			struct pool_occupant *occ = &pool->occupants[this_pos];
			pool->next_attending++;
			if(pool->next_attending >= pool->num_occupants) {
				pool->next_attending = 0;
			}
			if(occ->table_pos == -1 && occ->money >= th->small_blind * 2) {
				int index = th_add_player(th, occ->name, occ->money, occ->decide, occ->decision_data);
				printf("POOL: Player %s joins at seat %d.\n", occ->name, index);
				assert(index != -1);
				occ->table_pos = index;
				pool->sitters[index] = this_pos;
				break;
			}
		} while(pool->next_attending != start_attending);
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

	int ann_brain_files_pos = 0;
	char ann_brain_files[POOL_MAX_PLAYERS][32];
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
		} else {
			strncpy(ann_brain_files[ann_brain_files_pos++], argv[i], 32);
		}
	}
	printf("Random seed: %d\n", seed);
	srand(seed);

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
			strncpy(ais[i + data_pos].filename, ann_brain_files[ann_brain_files_pos], 32);
		} else {
			int num_layers = rand() % 2 + 2;
			int num_input = RANK_ACE + 1;
			int num_neurons_hidden = rand() % 2 + 2;
			int num_output = 1;
			*ann = fann_create_standard(num_layers, num_input,
					num_neurons_hidden, num_output);
		}
		fann_randomize_weights(*ann, -1.0f, 1.0f);
		fann_set_activation_function_hidden(*ann, FANN_SIGMOID_SYMMETRIC);
		fann_set_activation_function_output(*ann, FANN_SIGMOID_SYMMETRIC);
		ais[i + data_pos].num_rounds_played = 0;
		pool_add_player(&pool, start_money, ann_decision, ai_pool_func, &ais[i + data_pos], type);
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
	int save_pos = 0;
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th.players[i].money && th.players[i].decision_data) {
			struct ai_data *ai = (struct ai_data *)th.players[i].decision_data;
			if(ai->ann && ai->filename[0] == '\0') {
				char filename[32];
				while(1) {
					char randstr[8];
					memset(randstr, 0x00, sizeof(randstr));
					for(int j = 0; j < 4; j++) {
						randstr[j] = rand() % 20 + 'a';
					}
					snprintf(filename, 32, "ai_%d_%s.net",
							save_pos++, randstr);
					struct stat buffer;
					if(stat(filename, &buffer) != 0) {
						break;
					}
				}
				fann_save(ai->ann, filename);
				printf("Saved file %s.\n", filename);
			}
		}
	}

	return 0;
}
