#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "pdb.h"

#define DATABASE_FILE "players.db"

void load_dbt(DBT *dbt, const void *data, int length)
{
	memset(dbt, 0x00, sizeof(*dbt));  
	dbt->data = (void *)data;
	dbt->size = length;
}

DB *db_open(void)
{
	DB *db;
	int dbret;

	if ((dbret = db_create(&db, NULL, 0)) != 0) {
		fprintf(stderr, "db_create: %s\n", db_strerror(dbret)); 
		assert(0);
		return NULL;
	}

	if ((dbret = db->open(db, NULL, DATABASE_FILE, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) { 
		fprintf(stderr, "db->open %s: %s\n", DATABASE_FILE, db_strerror(dbret));
		db->close(db, 0);
		assert(0);
		return NULL;
	}

	return db;
}

int db_close(DB *db)
{
	int ret = db->close(db, 0);
	if(ret) {
		fprintf(stderr, "db->close: %s\n", db_strerror(ret));
		assert(0);
	}
	return ret;
}

int get_db_contents(DB *db, char **names, struct db_player **players)
{
	DB_BTREE_STAT *stats;
	stats = malloc(sizeof(DB_BTREE_STAT));
	memset(stats, 0x00, sizeof(*stats));
	int dbret = db->stat(db, NULL, &stats, 0);
	if(dbret != 0) {
		fprintf(stderr, "db->stat: %s\n", db_strerror(dbret));
		db->close(db, 0);
		free(stats);
		exit(1);
	}

	int num_entries = stats->bt_ndata;
	free(stats);
	int entry_num = 0;

	assert(num_entries >= 0);

	*names = calloc(32, num_entries);
	*players = calloc(sizeof(struct db_player), num_entries);

	DBC *cursor;
	dbret = db->cursor(db, NULL, &cursor, 0);
	if(dbret != 0) {
		fprintf(stderr, "db->cursor: %s\n", db_strerror(dbret));
		db->close(db, 0);
		exit(1);
	} else {
		while(entry_num < num_entries) {
			DBT key, value;
			memset(&key, 0x00, sizeof(key));
			memset(&value, 0x00, sizeof(value));

			dbret = cursor->get(cursor, &key, &value, DB_NEXT);
			if(dbret == DB_NOTFOUND) {
				break;
			} else if(dbret != 0) {
				fprintf(stderr, "cursor->get: %s\n", db_strerror(dbret));
				db->close(db, 0);
				exit(1);
			} else {
				strncpy(&(*names)[entry_num * 32], (const char *)key.data, 31);

				struct db_player *pp;
				pp = (struct db_player *)value.data;
				memcpy(&(*players)[entry_num], pp, sizeof(*pp));

				entry_num++;
			}
		}
	}
	cursor->close(cursor);

	assert(entry_num == num_entries);
	return num_entries;
}

static int save_ai(void *data, int size, const char *filename)
{
	if(size == 0)
		return 0;

	FILE *fp = fopen(filename, "wb");
	if(!fp) {
		fprintf(stderr, "Couldn't open file %s\n", filename);
		return 1;
	}
	int ret = fwrite(data, size, 1, fp);
	if(ret != 1) {
		fprintf(stderr, "Couldn't write to file %s\n", filename);
	}
	fclose(fp);
	return ret != 1;
}

int db_get_player(DB *db, const char *name, struct db_player *pp, const char *ai_filename)
{
	DBT key;
	DBT value;
	load_dbt(&key, name, strlen(name) + 1);
	memset(&value, 0x00, sizeof(value));

	int ret = db->get(db, NULL, &key, &value, 0);
	if(ret == 0) {
		memcpy(pp, value.data, sizeof(struct db_player));
		if(ai_filename) {
			return save_ai(value.data + sizeof(struct db_player),
					value.size - sizeof(struct db_player),
					ai_filename);
		}
		return 0;
	} else if(ret == DB_NOTFOUND) {
		return -1;
	} else {
		fprintf(stderr, "db->get: %s\n", db_strerror(ret)); 
	}
	return 1;
}

int db_update_player(DB *db, const char *name, int new_hands_played, int session_balance)
{
	assert(new_hands_played >= 0);
	if(new_hands_played == 0) {
		assert(session_balance == 0);
		return 0;
	}

	struct db_player *pp;

	DBT key;
	DBT value;
	load_dbt(&key, name, strlen(name) + 1);
	memset(&value, 0x00, sizeof(value));

	int ret = db->get(db, NULL, &key, &value, 0);
	if(ret) {
		fprintf(stderr, "db->get %s: %s\n", name, db_strerror(ret)); 
		assert(0);
		return 1;
	} else {
		pp = (struct db_player*)value.data;

		pp->hands_played += new_hands_played;
		pp->balance += session_balance;

		ret = db->put(db, NULL, &key, &value, 0);
		if(ret != 0) {
			fprintf(stderr, "db->put %s: %s\n", name, db_strerror(ret));
			assert(0);
			return 1;
		}
	}

	return 0;
}


