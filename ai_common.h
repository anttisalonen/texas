#pragma once

int ai_pool_func(int *num_rounds_played);

int ai_common_load_func(const char *header, float **values, const char *filename);
int ai_common_save_func(const char *header, float **values, const char *filename);


float rand_uniform(); /* between 0 and 1 */
float rand_uniform_clamped(); /* between -1 and 1 */


