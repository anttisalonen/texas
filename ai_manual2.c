#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "th.h"
#include "ai_common.h"
#include "ai_manual.h"

#define AIM_VERSION_STRING "2"

struct ai_data {
	int num_rounds_played;
	float bluff_probability;
	float tightness;
	int want_bluff;
	int bluff_up_to;
	float hand_score;
};

static float aim2_hand_score(const struct texas_holdem *th, int plnum)
{
	const struct card *cards = th->players[plnum].hole_cards;

	float score = cards[0].rank / (float)RANK_ACE;
	if(cards[1].rank < cards[0].rank)
		score = cards[1].rank / (float)RANK_ACE;
	score *= 0.5f;
	if(cards[0].rank == cards[1].rank)
		score *= 2.0f;
	return score;
}

static enum th_decision aim2_decide(const struct texas_holdem *th, int plnum, int raised_to, struct ai_data *d)
{
	int my_total_money = th->players[plnum].paid_bet + th->players[plnum].money;
	int my_raise_limit = 0;
	if(d->hand_score > d->tightness)
		my_raise_limit = d->hand_score * my_total_money;
	if(d->want_bluff) {
		int bluff_raise_limit = d->bluff_up_to * my_total_money;
		if(bluff_raise_limit > my_raise_limit)
			my_raise_limit = bluff_raise_limit;
	}

	int curr_paid = th->players[plnum].paid_bet;
	if(curr_paid >= my_raise_limit) {
		if(raised_to) {
			return DEC_FOLD;
		} else {
			return DEC_CHECK;
		}
	} else if(curr_paid + raised_to + th->small_blind * 4 >= my_raise_limit) {
		if(raised_to) {
			return DEC_CALL;
		} else {
			return DEC_CHECK;
		}
	} else {
		return DEC_RAISE;
	}
}

enum th_decision aim2_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	if(th->num_community_cards == 0) {
		/* first betting round */
		d->want_bluff = rand_uniform() < d->bluff_probability;
		if(d->want_bluff)
			d->bluff_up_to = rand_uniform();
	}
	d->hand_score = aim2_hand_score(th, plnum);
	return aim2_decide(th, plnum, raised_to, d);
}

int aim2_pool_func(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	return ai_pool_func(&d->num_rounds_played);
}

int aim2_load_func(void *data, const char *filename)
{
	struct ai_data *d = (struct ai_data *)data;
	d->num_rounds_played = 0;
	float *values[] = {&d->bluff_probability, &d->tightness, NULL};
	return ai_common_load_func("AIM2 " AIM_VERSION_STRING, values, filename);
}

void aim2_data_init(const char *type, void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	d->bluff_probability = rand_uniform() * 0.1f;
	d->tightness = rand_uniform() * 0.5f + 0.4f;
	d->num_rounds_played = 0;
}

int aim2_save_func(void *data, const char *filename)
{
	struct ai_data *d = (struct ai_data *)data;
	float *values[] = {&d->bluff_probability, &d->tightness, NULL};
	return ai_common_save_func("AIM2 " AIM_VERSION_STRING, values, filename);
}

