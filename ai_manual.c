#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <floatfann.h>

#include "th.h"
#include "ai_common.h"
#include "ai_manual.h"

#define AIM_VERSION_STRING "1"

struct ai_data {
	int num_rounds_played;
	float want_raise;
};

enum th_decision aim_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	if(rand_uniform() < d->want_raise) {
		return DEC_RAISE;
	} else if(!raised_to) {
		return DEC_CALL;
	} else {
		return DEC_FOLD;
	}
}

int aim_pool_func(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	return ai_pool_func(&d->num_rounds_played);
}

int aim_load_func(void *data, const char *filename)
{
	struct ai_data *d = (struct ai_data *)data;
	d->num_rounds_played = 0;
	FILE *fp = fopen(filename, "r");
	if(!fp) {
		fprintf(stderr, "Couldn't open file %s\n", filename);
		return 1;
	}
	char buf[1024];
	memset(buf, 0x00, sizeof(buf));
	int ret = fread(buf, 1, 1024, fp);
	if(ret < 3) {
		fprintf(stderr, "Couldn't read file %s\n", filename);
		fclose(fp);
		return 1;
	}
	fclose(fp);
	ret = sscanf(buf, "AIM " AIM_VERSION_STRING " %f", &d->want_raise);
	return ret != 1;
}

void aim_data_init(void *data)
{
	struct ai_data *d = (struct ai_data *)data;
	d->want_raise = rand_uniform();
	d->num_rounds_played = 0;
}

int aim_save_func(void *data, const char *filename)
{
	struct ai_data *d = (struct ai_data *)data;
	FILE *fp = fopen(filename, "w");
	if(!fp) {
		fprintf(stderr, "Couldn't open file %s\n", filename);
		return 1;
	}
	char buf[1024];
	memset(buf, 0x00, sizeof(buf));
	snprintf(buf, 1023, "AIM " AIM_VERSION_STRING " %f\n", d->want_raise);
	int ret = fwrite(buf, strlen(buf), 1, fp);
	if(ret != 1) {
		fprintf(stderr, "Couldn't write to file %s\n", filename);
	}
	fclose(fp);
	return ret != 1;
}

