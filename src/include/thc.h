/* Barrelfish THC language extensions */

#ifndef _THC_H_
#define _THC_H_

#ifndef LIBASYNC_FUNC_ATTR
#define LIBASYNC_FUNC_ATTR
#endif

#ifdef LINUX_KERNEL
#include <linux/types.h>
#include <linux/printk.h>
#define assert(XX) do {							\
		if (!(XX)) {						\
			printk("assertion failure at %s:%d\n",		\
				__FILE__, __LINE__);			\
		}							\
	} while(0)
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#ifndef BARRELFISH
typedef int errval_t;
#define SYS_ERR_OK   0
#define THC_CANCELED 1
#endif

#if (defined(linux) || defined(BARRELFISH))
//  #ifndef noinline
//      #define noinline __attribute__((noinline))
//  #endif
#endif

// The implementation of do..finish relies on shadowing so that 
// _fb_info always refers to the closest enclosing do..finish block.
#pragma GCC diagnostic ignored "-Wshadow"

/*......................................................................*/

// Macros for use from user code
//
// Cancel.  _TAG is the name of an enclosig DO_FINISH block.

#define CANCEL(_TAG)                                                    \
  do {                                                                  \
    _thc_do_cancel_request(_fb_info_ ## _TAG);                          \
  }  while (0)

// DO_FINISH block. Non-cancellable:

#define DO_FINISH_NX(_CODE) DO_FINISH__(__,_CODE,1)

// Cancellable do-finish, with tag:

#define DO_FINISH_(_TAG,_CODE) DO_FINISH__(_TAG,_CODE,0)

// Cancellable do-finish, no tag:

#define DO_FINISH(_CODE) DO_FINISH__(__,_CODE,0)

// ASYNC implementation.  __COUNTER__ is a GCC extension that will 
// allocate a unique integer ID on each expansion.  Hence use it once 
// here and use the resulting expanded value in ASYNC_:

#define ASYNC(_BODY) ASYNC_(_BODY, __COUNTER__)

#ifdef CONFIG_LAZY_THC

// DO_FINISH implementation:

#define DO_FINISH__(_TAG,_CODE,_IS_NX)					\
  do {                                                                  \
    finish_t _fb;                                                       \
    finish_t *_fb_info __attribute__((unused)) = &_fb;                  \
    finish_t *_fb_info_ ## _TAG __attribute__((unused)) = _fb_info;     \
    void *_fb_curr_stack __attribute__((unused));			\
    FORCE_FRAME_POINTER_USE;						\
    GET_STACK_POINTER(_fb_info->old_sp);				\
    _thc_startfinishblock(_fb_info, _IS_NX);				\
    do { _CODE } while (0);                                             \
    GET_STACK_POINTER(_fb_curr_stack);					\
    _thc_endfinishblock(_fb_info, _fb_curr_stack);			\
    if (_fb_info->old_sp != _fb_curr_stack) {				\
      RESTORE_OLD_STACK_POINTER(_fb_info->old_sp);			\
      _thc_pendingfree();						\
    }									\
} while (0)

// The basic idea for ASYNC_ is that the contents of the block becomes
// a nested function.  For the lazy implementation, we create an AWE for
// the continuation of the block and store a reference to it on the nested
// function's stackframe.  We then mark this as an async by replacing the
// return address of the nested function with a special marker.  If we block,
// the runtime system looks through the stack for this marker, allocating 
// a lazy stack for any AWE continuations it finds.
//
// We are careful to avoid taking the address of the nested function.
// This prevents GCC trying to generate stack-allocated trampoline functions
// (this is not implemented on Beehive where the I and D caches are not
// coherent).
//
// There are several hacks:
//
// 1. The nested function is given an explicit name in the generated 
//    assembly language code (NESTED_FN_STRING(_C)).  This means that 
//    inline asm can refer to it without needing to take the address
//    of the nested function.
//
// 2. The nested function specifically jumps to the point after the async
//    continuation rather than returning normally, since (i) we have fiddled
//    with the return address to make it a marker and (ii), changing it back 
//    then returning normally confuses the branch prediction hardware leading
//    to an increase in async cost by about 40 cycles (25 cycles -> 65 cycles) 
//
#define ASYNC_(_BODY, _C)						\
  do {									\
    awe_t _awe;                                                         \
    extern void * CONT_RET_FN_NAME(_C) (void);	         		\
									\
    /* Define nested function containing the body */			\
                                                                        \
    /* XXX: With these attributes, we're no longer portable with other  \
     * compilers ... */                                                 \
    auto void                                                           \
    noinline				                                \
    __attribute__((optimize("no-ipa-sra","no-ipa-cp")))                 \
    _thc_nested_async(FORCE_ARGS_STACK awe_t *awe)                      \
    __asm__(NESTED_FN_STRING(_C));                                      \
									\
    void								\
    noinline				                                \
    _thc_nested_async(FORCE_ARGS_STACK awe_t *awe) {			\
      void *_my_fb = _fb_info;						\
      _awe.current_fb = _my_fb;						\
      INIT_LAZY_AWE(awe, &_thc_lazy_awe_marker);			\
      do { _BODY; } while (0);						\
      /* If return address is NULLed then we blocked */			\
      if (__builtin_return_address(0) == NULL) {			\
	/* thc_startasync is performed lazily, we should run */		\
	/* _thc_endasync if we blocked*/				\
	_thc_endasync(_my_fb, __builtin_frame_address(0)+(2*__WORD_SIZE));\
      }									\
      /* Otherwise, return */						\
      RETURN_CONT(CONT_RET_FN_STRING(_C));				\
    }									\
									\
    _awe.status     = LAZY_AWE;						\
    _awe.lazy_stack = NULL;						\
    _awe.pts        = NULL;						\
									\
    SCHEDULE_CONT(&_awe, _thc_nested_async);                            \
    __asm__ volatile (							\
      "      .globl  " CONT_RET_FN_STRING(_C) "\n\t"			\
      " " CONT_RET_FN_STRING(_C) ":            \n\t"			\
    );                                                                  \
  } while (0)

#else // EAGER_THC

// DO_FINISH implementation:

#define DO_FINISH__(_TAG,_CODE,_IS_NX)					\
  do {                                                                  \
    finish_t _fb;                                                       \
    finish_t *_fb_info __attribute__((unused)) = &_fb;                  \
    finish_t *_fb_info_ ## _TAG __attribute__((unused)) = _fb_info;     \
    _thc_startfinishblock(_fb_info, _IS_NX);				\
    do { _CODE } while (0);                                             \
    _thc_endfinishblock(_fb_info, NULL);				\
} while (0)

// The basic idea for ASYNC_ is that the contents of the block becomes
// a nested function.  We create an AWE for the continuation of the
// block (passing it to the runtime system via SCHEDULE_CONT).
// We then execute the block on a new stack.  This is a bit cumbersome:
//
// - Allocate the stack memory _thc_allocstack
//
// - Define a "swizzle" function that will transfer execution onto the
//   new stack, capturing the stack and target function address from
//   its environment
// 
// We are careful to avoid taking the address of the nested function.
// This prevents GCC trying to generate stack-allocated trampoline functions
// (this is not implemented on Beehive where the I and D caches are not
// coherent).
//
// There are several hacks:
//
// 1. The nested function is given an explicit name in the generated 
//    assembly language code (NESTED_FN_STRING(_C)).  This means that 
//    inline asm can refer to it without needing to take the address
//    of the nested function.
//
// 2. The swizzle function passes control to the nested function assuming
//    that the calling conventions are the same for the two functions.
//    In particular, the swizzle function is called with the same
//    static chain as the underlying nested function.
#define ASYNC_(_BODY, _C)						\
  do {									\
    awe_t _awe;                                                         \
    void *_new_stack = _thc_allocstack();		       		\
    /* Define nested function containing the body */			\
    auto void _thc_nested_async(void) __asm__(NESTED_FN_STRING(_C));    \
    __attribute__((used)) void noinline _thc_nested_async(void) {       \
      void *_my_fb = _fb_info;						\
      void *_my_stack = _new_stack;                                     \
      _thc_startasync(_my_fb, _my_stack);                               \
      do { _BODY; } while (0);						\
      _thc_endasync(_my_fb, _my_stack);					\
      assert(0 && "_thc_endasync returned");				\
    }									\
                                                                        \
    /* Define function to enter _nested on a new stack */               \
    auto void _swizzle(void) __asm__(SWIZZLE_FN_STRING(_C));            \
    SWIZZLE_DEF(_swizzle, _new_stack, NESTED_FN_STRING(_C));            \
    _awe.current_fb = _fb_info;						\
                                                                        \
    /* Add AWE for our continuation, then run "_nested" on new stack */	\
    if (!SCHEDULE_CONT(&_awe)) {                                        \
      _swizzle();							\
      assert(0 && "_swizzle returned");					\
    }                                                                   \
  } while (0)

#endif // CONFIG_LAZY_THC

// Helper macros used in creating the assembly language name for the
// nested function

#define THC_STR_(X) #X
#define THC_STR(X) THC_STR_(X)
#define NESTED_FN_NAME(C) _thc_nested_async_ ## C
#define NESTED_FN_STRING(C) THC_STR(NESTED_FN_NAME(C))
#define SWIZZLE_FN_NAME(C) _thc_swizzle_ ## C
#define SWIZZLE_FN_STRING(C) THC_STR(SWIZZLE_FN_NAME(C))
#define CONT_RET_FN_NAME(C) _thc_cont_return_ ## C
#define CONT_RET_FN_STRING(C) THC_STR(CONT_RET_FN_NAME(C))

/*......................................................................*/

// Prototypes for functions to be called by user code (as opposed to
// by the implementations of compiler intrinsics)

// Initialize the runtime system for the given thread:
//
//   - execute "fn(args)" within it, as the first AWE
//
//   - return the result of "fn(args)" when it completes
//
//   - call "idle_fn(idle_args)" whenver there is no work
//     (or assert-fail if idle and idle_fn is NULL)

typedef int (*THCFn_t)(void *);
typedef void (*THCIdleFn_t)(void *);
//extern int THCRun(THCFn_t fn,
//                  void *args,
//                  THCIdleFn_t idle_fn,
//                  void *idle_args);

// An AWE is an asynchronous work element.  It runs to completion,
// possibly producing additional AWEs which may be run subsequently.
typedef struct awe_t awe_t;

// Invoke this before using any other code in the thc interface
int thc_global_init(void);

// Invoke this when you are finished using the thc code
void thc_global_fini(void);

// Invoke this to initialize the context for a *single thread*
void thc_init(void);

// Invoke this to destroy the context for a *single thread*
void thc_done(void);
void thc_done_with_pts(void *pts);

// Finish the current AWE, and initialize (*awe_ptr_ptr) with a pointer
// to an AWE for its continuation.  Typically, this will be stashed
// away in a data structure from which it will subsequently be fetched
// and passed to THCSchedule.
void THCSuspend(awe_t **awe_ptr_ptr);

// As THCSuspend, but execute "then_fn(arg)" before returning to the
// scheduler.
typedef void (*THCThenFn_t)(void *);
void THCSuspendThen(awe_t **awe_ptr_ptr, THCThenFn_t then, void *arg);

// Schedule execution of a given AWE at the head/tail of the queue.
void THCSchedule(awe_t *awe_ptr);
void THCScheduleBack(awe_t *awe_ptr);

// Finish the current AWE, returning to the scheduler.
//
// NOTE: If we were waiting for this awe to finish in order to exit
// a do-finish block (there is a do-finish awe that is waiting for the
// do-finish's awe count to reach zero), this *will not* schedule that
// awe. I'm not sure why this wasn't done in the first place.
//
// And so, TODO: clean up enclosing do-finish if we are the last awe.
void THCFinish(void);

// Invoke this to signal to all awe's that they should exit. Right now,
// exiting is voluntary: An awe should call THCShouldStop to detect
// if should terminate. Likely, the awe will want to call THCAbort (see
// below).
void THCStopAllAwes(void);

// Returns non-zero if the calling awe should terminate as soon as
// possible. The awe should probably call THCAbort.
int THCShouldStop(void);

// Like THCFinish, but if we are the last awe in our enclosing do-finish,
// and the do-finish is waiting for us, we schedule the do-finish awe.
//
// XXX: This should only be called in exceptional conditions. It doesn't
// try to free stacks like _thc_endasync. (The stacks will be freed when
// the runtime exits. It's possible to check if we are on a "lazy stack"
// and free it when we're in the LAZY case, but for EAGER, there's no
// marker we can walk back to.)
void THCAbort(void);

// Yields and saves awe_ptr to correspond to the provided id number
void THCYieldAndSave(uint32_t id_num);

// Finish the current AWE, creating a new AWE from its continuation, and
// passing this immediately to the scheduler.
void THCYield(void);

// Hint that the runtime system should switch from the current AWE to the
// indicated awe_ptr.  (Currently, the implementation does this immediately
// if awe_ptr runs on the same thread as the caller.  It puts the continuation
// to THCYieldTo on the run-queue.)
void THCYieldTo(awe_t *awe_ptr);

// Same as THCYieldTo except that an id number is provided to locate a 
// particular awe that was set to correspond to this id_num.
//
// id_to is the id that corresponds to the awe that should be yielded to.
//
// id_from is the id that corresponds to the awe that should save the
// current context.
//
// If id_to is invalid, returns -EINVAL, zero otherwise.
int THCYieldToIdAndSave(uint32_t id_to, uint32_t id_from);

// Yields to without saving curent awe id.
// 
// If there is no awe associated with id_to, returns non-zero.
int THCYieldToId(uint32_t id_to);

// Cancellation actions.  These are executed in LIFO order when cancellation 
// occurs.  Once cancellation has been requested, it is assumed that no
// further cancellation actions will be added.  Cancellation actions can be 
// added and removed in any order (not just LIFO) -- in practice this occurs
// when they are added/removed in different async branches.
//
// The structure of a cancel_item_t should be treated as opaque: it is 
// defined here so that its size is known, and hence so that it can be
// stack-allocated by callers to THCAdd/RemoveCancelItem.
typedef void (*THCCancelFn_t)(void *);
typedef struct cancel_item_t cancel_item_t;
struct cancel_item_t {
  THCCancelFn_t   fn;
  void           *arg;
  int             was_run;
  cancel_item_t  *prev;
  cancel_item_t  *next;
};

void THCAddCancelItem(cancel_item_t *ci, THCCancelFn_t fn, void *arg);
void THCRemoveCancelItem(cancel_item_t *ci);
int THCCancelItemRan(cancel_item_t *ci);

// Test for cancellation request
int THCIsCancelRequested(void);

// Dump debugging stats (if available, otherwise no-op)
void THCDumpStats(int clear_stats);
void THCIncSendCount(void);
void THCIncRecvCount(void);

// Dump all stacks that are in use
//
// XXX NOTE: You will most likely need to modify dump_stack. For example,
// in the LCD microkernel, normally the stack walk stops after we reach
// a certain address boundary. But when you call this function, you need
// the stack walk to continue until you reach a null frame address or
// return address. You basically want your stack walk to look like this:
//
//      while (frame_address != NULL && ret_address != NULL) {
//           ... walk next frame ...
//      }
void THCDumpAllStacks(void);

/*......................................................................*/

#include "thcsync.h"
#include "thcinternal.h"

#endif // _THC_H_
