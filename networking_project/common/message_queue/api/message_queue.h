#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stddef.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef void* MESSAGE_QUEUE_ID;


// Functions -------------------------------------------------------------------

eSTATUS message_queue_create(
    MESSAGE_QUEUE_ID*    out_message_queue_id,
    size_t               queue_size,
    size_t               message_size,
    fGENERIC_ALLOCATOR   allocator,
    fGENERIC_DEALLOCATOR deallocator);


eSTATUS message_queue_put(
    MESSAGE_QUEUE_ID  message_queue_id,
    const void* const message,
    size_t            message_size);


eSTATUS message_queue_peek(
    MESSAGE_QUEUE_ID message_queue_id,
    void* const      message_out_buffer,
    size_t           out_buffer_size);

    
eSTATUS message_queue_get(
    MESSAGE_QUEUE_ID message_queue_id,
    void* const      message_out_buffer,
    size_t           out_buffer_size);


eSTATUS message_queue_destroy(
    MESSAGE_QUEUE_ID message_queue_id);

#ifdef __cplusplus
}
#endif
