#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "message_queue.h"

#include <pthread.h>
#include <stddef.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef struct
{
    void* queue_buffer;

    fGENERIC_DEALLOCATOR deallocator;

    size_t message_size;
    size_t queue_size;
    size_t front_index;
    size_t used_slots;

    pthread_mutex_t queue_mutex;
    pthread_cond_t  queue_cond_var;
} sMESSAGE_QUEUE;

// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
