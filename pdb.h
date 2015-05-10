#pragma once

#include <db.h>

struct db_player {
	time_t creation_time;
	int hands_played;
	int balance;
	char type[32];
	char parents[2][32];
	int generation;
	int ai_size;
	char ai_data[];
};

DB *db_open(void);
int db_close(DB *db);
int get_db_contents(DB *db, char **names, struct db_player **players);
int db_get_player(DB *db, const char *name, struct db_player *pp, const char *ai_filename);
void load_dbt(DBT *dbt, const void *data, int length);
int db_update_player(DB *db, const char *name, int new_hands_played, int session_balance);

