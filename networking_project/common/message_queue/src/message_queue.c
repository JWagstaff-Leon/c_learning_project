#include "message_queue.h"
#include "message_queue_internal.h"

#include <assert.h>
#include <pthread.h>
#include <string.h>

#include "common_types.h"

#include <stdio.h>

eSTATUS message_queue_create(
    MESSAGE_QUEUE*       out_message_queue,
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
    assert(NULL != out_message_queue);

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

    new_message_queue_ptr->deallocator  = deallocator;

    new_message_queue_ptr->queue_buffer = new_queue_buffer;
    
    new_message_queue_ptr->queue_size   = queue_size;
    new_message_queue_ptr->message_size = message_size;
    
    new_message_queue_ptr->front_index  = 0;
    new_message_queue_ptr->used_slots   = 0;

    pthread_mutex_init(&new_message_queue_ptr->queue_mutex,
                       NULL);
    pthread_cond_init(&new_message_queue_ptr->queue_cond_var, NULL);

    *out_message_queue = (MESSAGE_QUEUE)new_message_queue_ptr;
    return STATUS_SUCCESS;
}


eSTATUS message_queue_put(
    MESSAGE_QUEUE message_queue,
    const void*   message,
    size_t        message_size)
{
    sMESSAGE_QUEUE* message_queue;
    size_t          current_message_index;
    size_t          current_message_offset;
    void*           current_message_buffer;
    eSTATUS         return_status = STATUS_SUCCESS;

    assert(NULL != message_queue);
    assert(NULL != message);

    message_queue = (sMESSAGE_QUEUE*)message_queue;
    pthread_mutex_lock(&message_queue->queue_mutex);

    if (message_size > message_queue->message_size)
    {
        return_status = STATUS_INVALID_ARG;
        goto func_exit;
    }

    if (message_queue->used_slots >= message_queue->queue_size)
    {
        return_status = STATUS_NO_SPACE;
        goto func_exit;
    }

    current_message_index  = (message_queue->front_index + message_queue->used_slots) % message_queue->queue_size;
    current_message_offset = current_message_index * message_queue->message_size;
    current_message_buffer = (void*)((size_t)message_queue->queue_buffer + current_message_offset);

    memcpy(current_message_buffer,
           message,
           message_queue->message_size);

    message_queue->used_slots += 1;
    return_status = STATUS_SUCCESS;

    pthread_cond_signal(&message_queue->queue_cond_var);

func_exit:
    pthread_mutex_unlock(&message_queue->queue_mutex);
    return return_status;
}


static eSTATUS get_locked_queue_next_msg(
    sMESSAGE_QUEUE* message_queue,
    void*           message_out_buffer,
    size_t          out_buffer_size)
{
    size_t current_message_offset;
    void*  current_message_buffer;

    if (out_buffer_size < message_queue->message_size)
    {
        return STATUS_INVALID_ARG;
    }

    if (message_queue->used_slots <= 0)
    {
        return STATUS_EMPTY;
    }

    current_message_offset = message_queue->front_index * message_queue->message_size;
    current_message_buffer = (void*)((size_t)message_queue->queue_buffer + current_message_offset);

    memcpy(message_out_buffer,
           current_message_buffer,
           message_queue->message_size);
        
    return STATUS_SUCCESS;
}


eSTATUS message_queue_peek(
    MESSAGE_QUEUE message_queue,
    void*         message_out_buffer,
    size_t        out_buffer_size)
{
    sMESSAGE_QUEUE* message_queue;
    eSTATUS         status = STATUS_SUCCESS;

    assert(NULL != message_queue);
    assert(NULL != message_out_buffer);

    message_queue = (sMESSAGE_QUEUE*)message_queue;
    pthread_mutex_lock(&message_queue->queue_mutex);

    status = get_locked_queue_next_msg(message_queue,
                                       message_out_buffer,
                                       out_buffer_size);

    pthread_mutex_unlock(&message_queue->queue_mutex);
    return status;
}

    
eSTATUS message_queue_get(
    MESSAGE_QUEUE message_queue,
    void*         message_out_buffer,
    size_t        out_buffer_size)
{
    sMESSAGE_QUEUE* message_queue;
    eSTATUS         status;

    assert(NULL != message_queue);
    message_queue = (sMESSAGE_QUEUE*)message_queue;

    pthread_mutex_lock(&message_queue->queue_mutex);

    while (message_queue->used_slots < 1)
    {
        // TODO make this use a timeout
        pthread_cond_wait(&message_queue->queue_cond_var, &message_queue->queue_mutex);
    }

    status = get_locked_queue_next_msg(message_queue,
                                       message_out_buffer,
                                       out_buffer_size);
    assert(STATUS_EMPTY != status);
    
    if (STATUS_SUCCESS == status)
    {
        message_queue = ((sMESSAGE_QUEUE*)message_queue);

        message_queue->used_slots -= 1;
        message_queue->front_index = (message_queue->front_index + 1) % message_queue->queue_size;
    }

    pthread_mutex_unlock(&message_queue->queue_mutex);
    return status;
}


eSTATUS message_queue_destroy(
    MESSAGE_QUEUE message_queue)
{
    sMESSAGE_QUEUE* message_queue;

    assert(NULL != message_queue);
    message_queue = (sMESSAGE_QUEUE*)message_queue;

    pthread_mutex_destroy(&message_queue->queue_mutex);
    pthread_cond_destroy(&message_queue->queue_cond_var);
    message_queue->deallocator(message_queue->queue_buffer);

    message_queue->deallocator(message_queue);
    return STATUS_SUCCESS;
}