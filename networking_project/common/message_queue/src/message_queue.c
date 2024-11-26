#include "message_queue.h"
#include "message_queue_internal.h"

#include <assert.h>
#include <string.h>

eSTATUS message_queue_create(
    MESSAGE_QUEUE        message_queue_out,
    size_t               queue_size,
    size_t               message_size,
    fGENERIC_ALLOCATOR   allocator,
    fGENERIC_DEALLOCATOR deallocator)
{
    sMESSAGE_QUEUE* new_message_queue_ptr;
    void*           new_queue_buffer;
    size_t          new_queue_buffer_size;

    assert(NULL != allocator);
    assert(NULL != deallocator);
    assert(NULL != message_queue_out);

    new_queue_buffer_size = queue_size * message_size;
    new_queue_buffer = allocator(new_queue_buffer_size);
    if (NULL == new_queue_buffer)
    {
        return STATUS_ALLOC_FAILED;
    }

    new_message_queue_ptr = (sMESSAGE_QUEUE*)allocator(sizeof(sMESSAGE_QUEUE));
    if (NULL == new_message_queue_ptr)
    {
        deallocator(new_queue_buffer);
        return STATUS_ALLOC_FAILED;
    }

    memset(new_message_queue_ptr, 0, sizeof(sMESSAGE_QUEUE));
    memset(new_queue_buffer, 0, new_queue_buffer_size);

    new_message_queue_ptr->allocator    = allocator;
    new_message_queue_ptr->deallocator  = deallocator;
    new_message_queue_ptr->queue_buffer = new_queue_buffer;
    new_message_queue_ptr->queue_size   = queue_size;
    new_message_queue_ptr->message_size = message_size;
    new_message_queue_ptr->front_index  = 0;
    new_message_queue_ptr->used_slots   = 0;

    message_queue_out = new_message_queue_ptr;
    return STATUS_SUCCESS;
}


eSTATUS message_queue_put(
    MESSAGE_QUEUE     message_queue,
    const void* const message,
    size_t            message_size)
{
    sMESSAGE_QUEUE* message_queue_ptr;
    size_t          current_message_index;
    size_t          current_message_offset;
    void*           current_message_buffer;

    assert(NULL != message_queue);
    assert(NULL != message);

    message_queue_ptr = (sMESSAGE_QUEUE*)message_queue;
    
    if (message_size > message_queue_ptr->message_size)
    {
        return STATUS_INVALID_ARG;
    }

    if (message_queue_ptr->used_slots >= message_queue_ptr->queue_size)
    {
        return STATUS_NO_SPACE;
    }

    current_message_index  = (message_queue_ptr->front_index + message_queue_ptr->used_slots) % message_queue_ptr->queue_size;
    current_message_offset = current_message_index * message_queue_ptr->message_size;
    current_message_buffer = (void*)((size_t)message_queue_ptr->queue_buffer + current_message_offset);

    memcpy(current_message_buffer,
           message,
           message_queue_ptr->message_size);

    message_queue_ptr->used_slots += 1;
    return STATUS_SUCCESS;
}


eSTATUS message_queue_peek(
    MESSAGE_QUEUE message_queue,
    void* const   message_out_buffer,
    size_t        out_buffer_size)
{
    sMESSAGE_QUEUE* message_queue_ptr;
    size_t          current_message_offset;
    void*           current_message_buffer;

    assert(NULL != message_queue);
    assert(NULL != message_out_buffer);

    message_queue_ptr = (sMESSAGE_QUEUE*)message_queue;

    if (out_buffer_size < message_queue_ptr->message_size)
    {
        return STATUS_INVALID_ARG;
    }

    current_message_offset = message_queue_ptr->front_index * message_queue_ptr->message_size;
    current_message_buffer = (void*)((size_t)message_queue_ptr->queue_buffer + current_message_offset);

    memcpy(message_out_buffer,
           current_message_buffer,
           message_queue_ptr->message_size);

    return STATUS_SUCCESS;
}

    
eSTATUS message_queue_get(
    MESSAGE_QUEUE message_queue,
    void* const   message_out_buffer,
    size_t        out_buffer_size)
{
    sMESSAGE_QUEUE* message_queue_ptr;
    eSTATUS         status;

    assert(NULL != message_queue);

    status = message_queue_peek(message_queue,
                                message_out_buffer,
                                out_buffer_size);
    
    if (STATUS_SUCCESS == status)
    {
        message_queue_ptr = ((sMESSAGE_QUEUE*)message_queue);

        message_queue_ptr->used_slots -= 1;
        message_queue_ptr->front_index = (message_queue_ptr->front_index + 1) % message_queue_ptr->queue_size;
    }

    return status;
}


eSTATUS message_queue_destroy(
    MESSAGE_QUEUE message_queue)
{
    sMESSAGE_QUEUE* message_queue_ptr;

    assert(NULL != message_queue);

    message_queue_ptr = (sMESSAGE_QUEUE*)message_queue;
    message_queue_ptr->deallocator(message_queue_ptr->queue_buffer);
    message_queue_ptr->deallocator(message_queue_ptr);
    // --------- MAKE SURE to NEVER use message_queue/message_queue_ptr after this line
    return STATUS_SUCCESS;
}