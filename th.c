#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "th.h"

void th_init(struct texas_holdem* th, int small_blind, th_event_callback evc)
{
	memset(th, 0x00, sizeof(*th));
	th->small_blind = small_blind;
	th->event_callback = evc;
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

static int th_get_num_active_players(const struct texas_holdem *th)
{
	int num_players = 0;
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th->players[i].active)
			num_players++;
	}
	return num_players;
}

static void th_clear_paid_bets(struct texas_holdem *th)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		th->players[i].paid_bet = 0;
	}
}

static int th_setup(struct texas_holdem *th)
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

static void th_bet(struct texas_holdem* th, unsigned int plnum, int bet)
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
}

static void th_deal_hole_cards(struct texas_holdem* th)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		for(int j = 0; j < 2; j++) {
			th->players[i].hole_cards[j] = deal_card(&th->deck);
		}
	}
}

static int th_get_decision(struct texas_holdem *th, int plnum, int raised_to, int bet_amount)
{
	if(!th->players[plnum].money)
		return raised_to;

	enum th_decision dec = th->players[plnum].decide(th, plnum, raised_to - th->players[plnum].paid_bet,
			th->players[plnum].decision_data);

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

	struct th_event ev;
	ev.type = TH_EVENT_DECISION;
	ev.decision = dec;
	if(dec == DEC_RAISE)
		ev.raise_amount = raised_to;
	ev.player_index = plnum;
	th->event_callback(th, &ev);

	return raised_to;
}

static void th_betting_round(struct texas_holdem *th, int first_in_line, unsigned int bet_amount, unsigned int min_bet)
{
	if(!min_bet)
		th_clear_paid_bets(th);

	if(th_get_num_active_players(th) < 2)
		return;

	int raised_to;

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

static int th_pl_index(struct texas_holdem *th, int pos)
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

static void th_first_betting_round(struct texas_holdem* th)
{
	th_bet(th, th_pl_index(th, 1), th->small_blind);
	th_bet(th, th_pl_index(th, 2), th->small_blind * 2);
	th_betting_round(th, th_pl_index(th, 3), th->small_blind * 2, th->small_blind * 2);
}

static void th_deal_community(struct texas_holdem *th, int num_cards)
{
	for(int i = 0; i < num_cards; i++) {
		struct card c = deal_card(&th->deck);
		th->community_cards[th->num_community_cards] = c;
		th->num_community_cards++;
	}
}

struct hand_permutations {
	struct poker_hand permutations[21];
	int index;
};

static void permutations_init(struct hand_permutations *perms)
{
	perms->index = 0;
}

static void permutations_add(struct hand_permutations *perms, struct card *cards)
{
	for(int i = 0; i < NUM_POKER_HAND_CARDS; i++) {
		perms->permutations[perms->index].cards[i] = cards[i];
	}
	perms->index++;
	assert(perms->index <= 21);
}

static void permute(struct card *cards, struct hand_permutations *perms)
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

static void th_showdown(struct texas_holdem *th)
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
	}

	struct th_event ev;
	ev.type = TH_EVENT_WIN;
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		ev.best_hands[i] = best_hands[i];
	}
	ev.winner_hand_name = best_hands[best_player].cat->name;

	if(!tied_players) {
		th->players[best_player].money += th->pot;
		ev.num_winners = 1;
		ev.winner_index[0] = best_player;
		ev.winner_money[0] = th->pot;
	} else {
		int num_tied_players = 0;
		for(int i = 0; i < TH_MAX_PLAYERS; i++) {
			if((1 << i) & tied_players) {
				num_tied_players++;
			}
		}

		ev.num_winners = num_tied_players;
		int winner_entry = 0;

		int split_pot = th->pot / num_tied_players;
		for(int i = 0; i < TH_MAX_PLAYERS; i++) {
			if((1 << i) & tied_players) {
				num_tied_players--;
				ev.winner_index[winner_entry] = i;
				if(num_tied_players == 0) {
					th->players[i].money += th->pot;
					ev.winner_money[winner_entry] = th->pot;
				} else {
					th->players[i].money += split_pot;
					th->pot -= split_pot;
					ev.winner_money[winner_entry] = split_pot;
				}
				winner_entry++;
			}
		}

	}

	th->event_callback(th, &ev);
}

int th_play_hand(struct texas_holdem *th)
{
	if(th_setup(th))
		return -1;

	th_deal_hole_cards(th);
	th_first_betting_round(th);
	th_deal_community(th, 3);
	struct th_event ev;
	ev.type = TH_EVENT_BET_ROUND_BEGIN;
	th->event_callback(th, &ev);
	th_betting_round(th, th_pl_index(th, 1), th->small_blind * 2, 0);
	th_deal_community(th, 1);
	th->event_callback(th, &ev);
	th_betting_round(th, th_pl_index(th, 1), th->small_blind * 4, 0);
	th_deal_community(th, 1);
	th->event_callback(th, &ev);
	th_betting_round(th, th_pl_index(th, 1), th->small_blind * 4, 0);

	th_showdown(th);

	return 0;
}


