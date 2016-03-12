#include <thc.h>
#include <thc_ipc.h>
#include <libfipc.h>
#include <thcinternal.h>
#include "thc_dispatch_test.h"
#include "thread_fn_util.h"
#include <awe-mapper.h>
#include <linux/delay.h>

#define BATCH_INTERVAL 100

static struct fipc_ring_channel* channel;

static unsigned long add_nums_async(unsigned long lhs, unsigned long rhs, unsigned long msg_id, int fn_type)
{
	struct fipc_message *msg;
    struct fipc_message *response;
	unsigned long result;
	if( test_ipc_blocking_send_start(channel, &msg) )
    {
        printk(KERN_ERR "Error getting send message for add_nums_async.\n");
    }
	set_fn_type(msg, fn_type);
	fipc_set_reg0(lhs);
	fipc_set_reg1(rhs);
	THC_MSG_ID(msg)   = msg_id;
    THC_MSG_TYPE(msg) = msg_type_request;
    send_and_get_response(channel, msg, &response);
    fipc_recv_msg_end(channel, response);
	result = fipc_get_reg0(response);
    printk(KERN_ERR "result is %lu\n", result);
	
	return result;
}



int thread1_fn(void* chan)
{
	volatile void ** frame  = (volatile void**)__builtin_frame_address(0);
	volatile void *ret_addr = *(frame + 1);
    int num_transactions    = 0;
	uint32_t id_num;
	*(frame + 1) = NULL;
    channel = chan;

    thc_init();
 	DO_FINISH_(thread1_fn,{
		while (num_transactions < TRANSACTIONS) {
		printk(KERN_ERR "num_transactions: %d\n", num_transactions);

		ASYNC(
            printk(KERN_ERR "id created\n");
			id_num = awe_mapper_create_id();
            printk(KERN_ERR "id returned\n");
		    num_transactions++;
			add_nums_async(num_transactions, 1,(unsigned long) id_num, ADD_2_FN);
		     );
        if( (num_transactions) % THD3_INTERVAL == 0 )
        {
            ASYNC(
            printk(KERN_ERR "id created\n");
                id_num = awe_mapper_create_id();
            printk(KERN_ERR "id returned\n");
		        num_transactions++;
                add_nums_async(num_transactions, 2,(unsigned long) id_num, ADD_10_FN);
                 );
        }
		ASYNC(
            printk(KERN_ERR "id created\n");
			id_num = awe_mapper_create_id();
            printk(KERN_ERR "id returned\n");
		    num_transactions++;
			add_nums_async(num_transactions, 3,(unsigned long) id_num, ADD_2_FN);
		     );
            //msleep(20);
	}
    printk(KERN_ERR "done with transactions\n");
    });
	printk(KERN_ERR "lcd async exiting module and deleting ptstate");
	thc_done();

	*(frame + 1) = ret_addr;

    return 1;
}


