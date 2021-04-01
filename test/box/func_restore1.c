#include <stdio.h>
#include <stdbool.h>
#include <msgpuck.h>

#include "module.h"

static int
echo_num(box_function_ctx_t *ctx, const char *args,
	 const char *args_end, unsigned int num)
{
	char res[16];
	char *end = mp_encode_uint(res, num);
	box_return_mp(ctx, res, end);
	return 0;
}


int
echo_1(box_function_ctx_t *ctx, const char *args, const char *args_end)
{
	return echo_num(ctx, args, args_end, 1);
}

int
echo_2(box_function_ctx_t *ctx, const char *args, const char *args_end)
{
	return echo_num(ctx, args, args_end, 2);
}

int
echo_3(box_function_ctx_t *ctx, const char *args, const char *args_end)
{
	return echo_num(ctx, args, args_end, 3);
}
