#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stddef.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef void* MESSAGE_QUEUE;


// Functions -------------------------------------------------------------------

eSTATUS message_queue_create(
    MESSAGE_QUEUE*       out_message_queue,
    size_t               queue_size,
    size_t               message_size,
    fGENERIC_ALLOCATOR   allocator,
    fGENERIC_DEALLOCATOR deallocator);


eSTATUS message_queue_put(
    MESSAGE_QUEUE     message_queue,
    const void* const message,
    size_t            message_size);


eSTATUS message_queue_peek(
    MESSAGE_QUEUE message_queue,
    void* const   message_out_buffer,
    size_t        out_buffer_size);

    
eSTATUS message_queue_get(
    MESSAGE_QUEUE message_queue,
    void* const   message_out_buffer,
    size_t        out_buffer_size);


eSTATUS message_queue_destroy(
    MESSAGE_QUEUE message_queue);

#ifdef __cplusplus
}
#endif
