#pragma once

#include "th.h"

#define POOL_MAX_PLAYERS 128

typedef int (*pool_decision_func)(void *data);

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

enum pool_seat_status {
	POOL_SEAT_FREED,
	POOL_SEAT_TAKEN,
};

struct pool_seat_update {
	enum pool_seat_status status;
	const char *player;
};

struct pool_update {
	struct pool_seat_update seats[TH_MAX_PLAYERS];
};

void pool_init(struct player_pool *pool);
void pool_add_player(struct player_pool *pool, int start_money,
		th_decision_func decide,
		pool_decision_func pool_func,
		void *data, const char *type);
void pool_update_th(struct player_pool *pool, struct texas_holdem *th, struct pool_update *update);

