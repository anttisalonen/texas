#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ai_common.h"

int ai_pool_func(int *num_rounds_played)
{
	(*num_rounds_played)++;
	if(*num_rounds_played >= 10) {
		int get_up = rand_uniform() < 0.1f;
		if(get_up) {
			*num_rounds_played = 0;
			return 0;
		}
	}
	return 1;
}

float rand_uniform()
{
	return rand() / (float)RAND_MAX;
}

float rand_uniform_clamped()
{
	return rand_uniform() * 2.0f - 1.0f;
}

int ai_common_load_func(const char *header, float **values, const char *filename)
{
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
	char fmtstr[1024];
	memset(fmtstr, 0x00, sizeof(fmtstr));
	if(strncmp(header, buf, strlen(header))) {
		fprintf(stderr, "Couldn't parse header in %s\n", filename);
		printf("'%s' '%s'\n", buf, header);
		assert(0);
		return 1;
	}
	char *pos = &buf[0] + strlen(header) + 1;
	char *endptr;
	while(*values) {
		float val = strtof(pos, &endptr);
		if(pos == endptr) {
			fprintf(stderr, "Error parsing float in %s\n", filename);
			return 1;
		}
		pos = endptr;
		**values = val;
		values++;
	}

	return 0;
}

int ai_common_save_func(const char *header, float **values, const char *filename)
{
	FILE *fp = fopen(filename, "w");
	if(!fp) {
		fprintf(stderr, "Couldn't open file %s\n", filename);
		return 1;
	}
	int ret = fwrite(header, strlen(header), 1, fp);
	if(ret != 1) {
		fprintf(stderr, "Couldn't write header to file %s\n", filename);
		fclose(fp);
		return 1;
	}

	char buf[1024];
	memset(buf, 0x00, sizeof(buf));
	while(*values) {
		sprintf(buf, " %f", **values);
		if((fwrite(buf, strlen(buf), 1, fp)) != 1) {
			fprintf(stderr, "Couldn't write float to file %s\n", filename);
			fclose(fp);
			return 1;
		}
		values++;
	}
	fclose(fp);
	return 0;
}


