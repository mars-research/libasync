
#ifdef LCD_DOMAINS
#include <lcd_config/pre_hook.h>
#endif

#include <thc_ipc.h>
#include <thc_ipc_types.h>
#include <libfipc_types.h>
#include <awe_mapper.h>
#include <asm/atomic.h>
#include <linux/slab.h>

#ifdef LCD_DOMAINS
#include <lcd_config/post_hook.h>
#endif

#ifndef LINUX_KERNEL_MODULE
#undef EXPORT_SYMBOL
#define EXPORT_SYMBOL(x)
#endif

int 
LIBASYNC_FUNC_ATTR
thc_channel_init(struct thc_channel *chnl, 
		struct fipc_ring_channel *async_chnl)
{
    chnl->state = THC_CHANNEL_LIVE;
    atomic_set(&chnl->refcnt, 1);
    chnl->fipc_channel = async_chnl;

    return 0;
}
EXPORT_SYMBOL(thc_channel_init);

int
LIBASYNC_FUNC_ATTR
thc_channel_init_0(struct thc_channel *chnl,
		struct fipc_ring_channel *async_chnl)
{
    chnl->state = THC_CHANNEL_LIVE;
    atomic_set(&chnl->refcnt, 1);
    chnl->fipc_channel = async_chnl;
    chnl->one_slot = true;

    return 0;
}
EXPORT_SYMBOL(thc_channel_init_0);

static int thc_recv_predicate(struct fipc_message* msg, void* data)
{
	struct predicate_payload* payload_ptr = (struct predicate_payload*)data;
	payload_ptr->msg_type = thc_get_msg_type(msg);

	if (payload_ptr->msg_type == msg_type_request) {
		/*
		 * Ignore requests
		 */
		return 0;
	} else if (payload_ptr->msg_type == msg_type_response) {

		payload_ptr->actual_msg_id = thc_get_msg_id(msg);

		if (payload_ptr->actual_msg_id == 
			payload_ptr->expected_msg_id) {
			/*
			 * Response is for us; tell libfipc we want it
			 */
			return 1;
		} else {
			/*
			 * Response for another awe; we will switch to
			 * them. Meanwhile, the message should stay in
			 * rx. They (the awe we switch to) will pick it up.
			 */
			return 0;
		}
	} else {
		/*
		 * Ignore any other message types
		 */
		return 0;
	}
}

//assumes msg is a valid received message
static int poll_recv_predicate(struct fipc_message* msg, void* data)
{
    struct predicate_payload* payload_ptr = (struct predicate_payload*)data;

    if( thc_get_msg_type(msg) == (uint32_t)msg_type_request )
    {
        payload_ptr->msg_type = msg_type_request;
        return 1;
    }
    else
    {
        payload_ptr->actual_msg_id = thc_get_msg_id(msg);
        return 0; //message not for this awe
    }
}

static void drop_one_rx_msg(struct thc_channel *chnl)
{
	int ret;
	struct fipc_message *msg;

	ret = fipc_recv_msg_start(thc_channel_to_fipc(chnl), &msg);
	if (ret)
		printk(KERN_ERR "thc_ipc_recv_response: failed to drop bad message\n");
	ret = fipc_recv_msg_end(thc_channel_to_fipc(chnl), msg);
	if (ret)
		printk(KERN_ERR "thc_ipc_recv_response: failed to drop bad message (mark as received)\n");
	return;
}

static void try_yield(struct thc_channel *chnl, uint32_t our_request_cookie,
		uint32_t received_request_cookie)
{
	int ret;
	/*
	 * Switch to the pending awe the response belongs to
	 */
	ret = THCYieldToIdAndSave(received_request_cookie,
				our_request_cookie);
	if (ret) {
		/*
		 * Oops, the switch failed
		 */
		printk(KERN_ERR "thc_ipc_recv_response: Invalid request cookie 0x%x received; dropping the message\n",
			received_request_cookie);
		drop_one_rx_msg(chnl);
		return;
	}
	/*
	 * We were woken back up
	 */
	return;
}

int 
LIBASYNC_FUNC_ATTR 
thc_ipc_recv_response(struct thc_channel *chnl, 
		uint32_t request_cookie, 
		struct fipc_message **response)
{
	struct predicate_payload payload = {
		.expected_msg_id = request_cookie
	};
	int ret;

retry:
        ret = fipc_recv_msg_if(thc_channel_to_fipc(chnl), thc_recv_predicate, 
			&payload, response);
	if (ret == 0) {
		/*
		 * Message for us; remove request_cookie from awe mapper
		 */
                awe_mapper_remove_id(request_cookie);
		return 0;
	} else if (ret == -ENOMSG && payload.msg_type == msg_type_request) {
		/*
		 * Ignore requests; yield so someone else can receive it (msgs
		 * are received in fifo order).
		 */
		goto yield;
	} else if (ret == -ENOMSG && payload.msg_type == msg_type_response) {
		/*
		 * Response for someone else. Try to yield to them.
		 */

		try_yield(chnl, request_cookie, payload.actual_msg_id);
		/*
		 * We either yielded to the pending awe the response
		 * belonged to, or the switch failed.
		 *
		 * Make sure the channel didn't die in case we did go to
		 * sleep.
		 */
		if (unlikely(thc_channel_is_dead(chnl)))
			return -EPIPE; /* someone killed the channel */
		goto retry;
	} else if (ret == -ENOMSG) {
		/*
		 * Unknown or unspecified message type; yield and let someone
		 * else handle it.
		 */
		goto yield;
	} else if (ret == -EWOULDBLOCK) {
		/*
		 * No messages in rx buffer; go to sleep.
		 */
		goto yield;
	} else {
		/*
		 * Error
		 */
		printk(KERN_ERR "thc_ipc_recv_response: fipc returned %d\n", 
			ret);
		return ret;
	}

yield:
	/*
	 * Go to sleep, we will be woken up at some later time
	 * by the dispatch loop or some other awe.
	 */
	THCYieldAndSave(request_cookie);
	/*
	 * We were woken up; make sure the channel didn't die while
	 * we were asleep.
	 */
	if (unlikely(thc_channel_is_dead(chnl)))
                return -EPIPE; /* someone killed the channel */
	else
		goto retry;
}
EXPORT_SYMBOL(thc_ipc_recv_response);

int 
LIBASYNC_FUNC_ATTR 
thc_poll_recv_group(struct thc_channel_group* chan_group, 
		struct thc_channel_group_item** chan_group_item, 
		struct fipc_message** out_msg)
{
    struct thc_channel_group_item *curr_item;
    struct fipc_message* recv_msg;
    int ret;

    list_for_each_entry(curr_item, &(chan_group->head), list)
    {
        ret = thc_ipc_poll_recv(thc_channel_group_item_channel(curr_item), 
                        &recv_msg);
        if( !ret )
        {
            *chan_group_item = curr_item;
            *out_msg         = recv_msg;
            
            return 0;
        }
    }

    return -EWOULDBLOCK;
}
EXPORT_SYMBOL(thc_poll_recv_group);

int 
LIBASYNC_FUNC_ATTR 
thc_ipc_poll_recv(struct thc_channel* chnl,
	struct fipc_message** out_msg)
{
    struct predicate_payload payload;
    int ret;

    while( true )
    {
	if (chnl->one_slot)
	        ret = fipc_recv_msg_if_0(thc_channel_to_fipc(chnl), poll_recv_predicate,
                       &payload, out_msg);
	else
	        ret = fipc_recv_msg_if(thc_channel_to_fipc(chnl), poll_recv_predicate,
                       &payload, out_msg);
        if( !ret ) //message for us
        {
            return 0; 
        }
        else if( ret == -ENOMSG ) //message not for us
        {
            THCYieldToId((uint32_t)payload.actual_msg_id);
            if (unlikely(thc_channel_is_dead(chnl)))
                return -EPIPE; // channel died
        }
        else if( ret == -EWOULDBLOCK ) //no message, return
        {
            return ret;
        }
        else
        {
            printk(KERN_ERR "error in thc_poll_recv: %d\n", ret);
            return ret;
        }
    }
}
EXPORT_SYMBOL(thc_ipc_poll_recv);

int
LIBASYNC_FUNC_ATTR
thc_ipc_call(struct thc_channel *chnl,
	struct fipc_message *request,
	struct fipc_message **response)
{
    uint32_t request_cookie;
    int ret;
    /*
     * Send request
     */
    ret = thc_ipc_send_request(chnl, request, &request_cookie);
    if (ret) {
        printk(KERN_ERR "thc_ipc_call: error sending request\n");
        goto fail1;
    }
    /*
     * Receive response
     */
    ret = thc_ipc_recv_response(chnl, request_cookie, response);
    if (ret) {
        printk(KERN_ERR "thc_ipc_call: error receiving response\n");
        goto fail2;
    }

    return 0;

fail2:
    awe_mapper_remove_id(request_cookie);
fail1:
    return ret;
}
EXPORT_SYMBOL(thc_ipc_call);

int
LIBASYNC_FUNC_ATTR
thc_ipc_send_request(struct thc_channel *chnl,
		struct fipc_message *request,
		uint32_t *request_cookie)
{
    uint32_t msg_id;
    int ret;
    /*
     * Get an id for our current awe, and store in request.
     */
    ret = awe_mapper_create_id(&msg_id);
    if (ret) {
        printk(KERN_ERR "thc_ipc_send: error getting request cookie\n");
	goto fail0;
    }
    thc_set_msg_type(request, msg_type_request);
    thc_set_msg_id(request, msg_id);
    /*
     * Send request
     */
    ret = fipc_send_msg_end(thc_channel_to_fipc(chnl), request);
    if (ret) {
        printk(KERN_ERR "thc: error sending request");
        goto fail1;	
    }

    *request_cookie = msg_id;

    return 0;

fail1:
    awe_mapper_remove_id(msg_id);
fail0:
    return ret;
}
EXPORT_SYMBOL(thc_ipc_send_request);

int
LIBASYNC_FUNC_ATTR
thc_ipc_reply(struct thc_channel *chnl,
	uint32_t request_cookie,
	struct fipc_message *response)
{
    thc_set_msg_type(response, msg_type_response);
    thc_set_msg_id(response, request_cookie);
    return fipc_send_msg_end(thc_channel_to_fipc(chnl), response);
}
EXPORT_SYMBOL(thc_ipc_reply);

int 
LIBASYNC_FUNC_ATTR 
thc_channel_group_init(struct thc_channel_group* channel_group)
{
    INIT_LIST_HEAD(&(channel_group->head));
    channel_group->size = 0;

    return 0;
}
EXPORT_SYMBOL(thc_channel_group_init);

int
LIBASYNC_FUNC_ATTR
thc_channel_group_item_init(struct thc_channel_group_item *item,
			struct thc_channel *chnl,
			int (*dispatch_fn)(struct thc_channel_group_item*, 
					struct fipc_message*))
{
    INIT_LIST_HEAD(&item->list);
    item->channel = chnl;
    item->dispatch_fn = dispatch_fn;

    return 0;
}
EXPORT_SYMBOL(thc_channel_group_item_init);

int 
LIBASYNC_FUNC_ATTR 
thc_channel_group_item_add(struct thc_channel_group* channel_group, 
                          struct thc_channel_group_item* item)
{
    list_add_tail(&(item->list), &(channel_group->head));
    channel_group->size++;

    return 0;
}
EXPORT_SYMBOL(thc_channel_group_item_add);

void
LIBASYNC_FUNC_ATTR 
thc_channel_group_item_remove(struct thc_channel_group* channel_group, 
			struct thc_channel_group_item* item)
{
    list_del_init(&(item->list));
    channel_group->size--;
}
EXPORT_SYMBOL(thc_channel_group_item_remove);

int 
LIBASYNC_FUNC_ATTR 
thc_channel_group_item_get(struct thc_channel_group* channel_group, 
                               int index, 
                               struct thc_channel_group_item **out_item)
{

    int curr_index = 0;
    list_for_each_entry((*out_item), &(channel_group->head), list)
    {
        if( curr_index == index )
        {
            return 0;
        }
        curr_index++;
    }

    return 1;
}
EXPORT_SYMBOL(thc_channel_group_item_get);
