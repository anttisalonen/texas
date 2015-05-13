#include <string.h>
#include <assert.h>

#include <floatfann.h>

#include "th.h"
#include "ann1.h"
#include "ann_common.h"

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

	if(calc_out[0] > calc_out[1] && calc_out[0] > calc_out[2])
		return DEC_RAISE;
	else if(raised_to > 0 && calc_out[1] > calc_out[2])
		return DEC_CALL;
	else if(raised_to > 0)
		return DEC_FOLD;
	else
		return DEC_CHECK;
}

enum th_decision ann2_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	fann_type *calc_out;
	fann_type input[2 * RANK_ACE + 1];

	for(int j = RANK_TWO; j < RANK_ACE; j++) {
		input[j] = 0.0f;
		for(int i = 0; i < 2; i++) {
			if(th->players[plnum].hole_cards[i].rank == j)
				input[j] += 0.5f;
		}
	}

	for(int j = RANK_TWO; j < RANK_ACE; j++) {
		input[j + RANK_ACE] = 0.0f;
		for(int i = 0; i < th->num_community_cards; i++) {
			if(th->community_cards[i].rank == j)
				input[j + RANK_ACE] += 0.25f;
		}
	}

	input[2 * RANK_ACE] = (2.0f * (th->pot / (1.0f + th->players[plnum].money * 0.5f))) - 1.0f;
	if(input[2 * RANK_ACE] > 1.0f)
		input[2 * RANK_ACE] = 1.0f;

	struct ai_data *d = (struct ai_data *)data;
	calc_out = fann_run(d->ann, input);

	if(calc_out[0] > calc_out[1] && calc_out[0] > calc_out[2])
		return DEC_RAISE;
	else if(raised_to > 0 && calc_out[1] > calc_out[2])
		return DEC_CALL;
	else if(raised_to > 0)
		return DEC_FOLD;
	else
		return DEC_CHECK;
}


