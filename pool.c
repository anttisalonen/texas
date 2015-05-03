#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "pool.h"

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
		pool_save_func save_func,
		void *data, const char *type)
{
	assert(pool->num_occupants < POOL_MAX_PLAYERS);

	struct pool_occupant* occ = &pool->occupants[pool->num_occupants];
	snprintf(occ->name, TH_MAX_PLAYER_NAME_LEN, "%d (%s)", pool->num_occupants, type);
	occ->money = start_money;
	occ->table_pos = -1;
	occ->pool_func = pool_func;
	occ->save_func = save_func;
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
				int index = th_add_player(th, occ->name, occ->money,
						occ->decide,
						occ->decision_data);
				printf("POOL: Player %s joins at seat %d.\n", occ->name, index);
				assert(index != -1);
				occ->table_pos = index;
				pool->sitters[index] = this_pos;
				break;
			}
		} while(pool->next_attending != start_attending);
	}
}

int pool_save_current_players(struct player_pool *pool, struct texas_holdem *th)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(th->players[i].money && th->players[i].decision_data) {
			struct pool_occupant *occ = &pool->occupants[pool->sitters[i]];
			if(occ->save_func) {
				occ->save_func(th->players[i].decision_data);
			}
		}
	}
	return 0;
}
