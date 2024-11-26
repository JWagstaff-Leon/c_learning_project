#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "message_queue.h"

#include <stddef.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef struct
{
    void* queue_buffer;

    fGENERIC_ALLOCATOR   allocator;
    fGENERIC_DEALLOCATOR deallocator;

    size_t message_size;
    size_t queue_size;
    size_t front_index;
    size_t used_slots;
} sMESSAGE_QUEUE;

// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
