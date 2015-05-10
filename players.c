#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "ai_config.h"
#include "pdb.h"

void add_player(DB *db, const char *name, const char *type, int ai_size, void *ai_data)
{
	DBT key, value;
	struct db_player *pp;
	int dbret;

	int total_size = sizeof(struct db_player) + ai_size;

	pp = malloc(total_size);
	assert(pp);
	memset(pp, 0x00, total_size);

	pp->creation_time = time(NULL);

	strcpy(pp->type, type);
	load_dbt(&key, (void *)name, strlen(name) + 1);
	if(ai_size) {
		pp->ai_size = ai_size;
		memcpy(pp->ai_data, ai_data, ai_size);
	}
	load_dbt(&value, pp, total_size);

	dbret = db->put(db, NULL, &key, &value, DB_NOOVERWRITE);
	if(dbret == DB_KEYEXIST) {
		fprintf(stderr, "%s already exists.\n", name);
	} else if(dbret != 0) {
		fprintf(stderr, "db->put %s: %s\n", name, db_strerror(dbret));
		db_close(db);
		exit(1);
	} else {
		printf("%s (%s, %d) successfully added.\n", name, type, ai_size);
	}

	free(pp);
}

void create_ai_name(char *buf)
{
	for(int j = 0; j < 8; j++) {
		buf[j] = rand() % 20 + 'a';
	}
}

int main(int argc, char **argv)
{
	const char *human_name = NULL;
	const char *ai_type = NULL;
	int num_ais = 0;
	int list = 0;

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--human")) {
			human_name = argv[++i];
		} else if(!strcmp(argv[i], "--ai")) {
			ai_type = argv[++i];
			num_ais = atoi(argv[++i]);
		} else if(!strcmp(argv[i], "--list")) {
			list = 1;
		} else {
			fprintf(stderr, "Unknown parameter \"%s\".\n", argv[i]);
			exit(1);
		}
	}

	DB *db = db_open();

	if(list) {
		char *keys = NULL;
		struct db_player *values = NULL;
		int num_entries;

		num_entries = get_db_contents(db, &keys, &values);

		for(int i = 0; i < num_entries; i++) {
			const char *key = &keys[i * 32];
			struct db_player *pp = &values[i];

			char timebuf[256];
			ctime_r(&pp->creation_time, timebuf);
			char *eol = strrchr(timebuf, '\n');
			if(eol)
				*eol = '\0';
			printf("%-20s: %-8s (%s)\n", key, pp->type,
					timebuf);
		}
		free(keys);
		free(values);

	}

	if(human_name) {
		add_player(db, human_name, "human", 0, NULL);
	}

	if(ai_type) {
		for(int i = 0; i < num_ais; i++) {
			char ai_name[32];
			memset(ai_name, 0x00, sizeof(ai_name));
			create_ai_name(ai_name);
			int ai_size = 0;
			void *ai_data = NULL;

			struct ai_config *aic = get_ai_config(ai_type);
			if(!aic) {
				fprintf(stderr, "Unknown AI type: %s\n", ai_type);
				db_close(db);
				exit(1);
			} else {
				struct dummy_ai_data d;

				if(aic->ai_save_func) {
					aic->ai_init_func(&d);
					int ret = aic->ai_save_func(&d, ".ai.tmp");
					if(ret) {
						fprintf(stderr, "ai_save_func(%s): %d\n",
								ai_type, ret);
						if(unlink(".ai.tmp"))
							perror("unlink");
						db_close(db);
						exit(1);
					}
					FILE *f = fopen(".ai.tmp", "rb");
					fseek(f, 0, SEEK_END);
					long fsize = ftell(f);
					fseek(f, 0, SEEK_SET);

					char *buf = malloc(fsize);
					fread(buf, fsize, 1, f);
					fclose(f);

					ai_size = fsize;
					ai_data = buf;

					if(unlink(".ai.tmp")) {
						perror("unlink");
						db_close(db);
						exit(1);
					}
				}
			}

			add_player(db, ai_name, ai_type, ai_size, ai_data);
			if(ai_data) {
				free(ai_data);
			}
		}
	}

	db_close(db);
	return 0;
}
