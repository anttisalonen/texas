#pragma once

struct ai_data {
	struct fann *ann;
	int num_rounds_played;
	char filename[256];
};

enum th_decision ann_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data);
int ai_pool_func(void *data);
void ann_modify(struct fann *ann);
void ann_set_filename(char *to, char *filename);
void ann_update_filename(char *to, const char *old);
void ann_new_filename(char *filename);

