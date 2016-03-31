/*
 * rpc.h
 *
 * Internal defs
 *
 * Copyright: University of Utah
 */
#ifndef LIBFIPC_RPC_TEST_H
#define LIBFIPC_RPC_TEST_H

#include <libfipc.h>

enum fn_type {
	NULL_INVOCATION, 
	ADD_CONSTANT, 
	ADD_NUMS, 
	ADD_3_NUMS,
	ADD_4_NUMS, 
	ADD_5_NUMS, 
	ADD_6_NUMS
};

/* must be divisible by 6... because I call 6 functions in the callee.c */
#define TRANSACTIONS 60

/* thread main functions */
int callee(void *_callee_channel_header);
int caller(void *_caller_channel_header);

static inline
int
get_fn_type(struct fipc_message *msg)
{
	return fipc_get_flags(msg) >> THC_RESERVED_MSG_FLAG_BITS;
}

static inline
void
set_fn_type(struct fipc_message *msg, enum fn_type type)
{
	uint32_t flags = fipc_get_flags(msg);
	/* ensure type is in range */
	type &= (1 << (32 - THC_RESERVED_MSG_FLAG_BITS)) - 1;
	/* erase old type */
	flags &= ((1 << THC_RESERVED_MSG_FLAG_BITS) - 1);
	/* install new type */
	flags |= (type << THC_RESERVED_MSG_FLAG_BITS)
	fipc_set_flags(msg, flags);
}

#endif /* LIBFIPC_RPC_TEST_H */
