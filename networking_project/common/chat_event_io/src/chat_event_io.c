#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"
#include "chat_event_io_network_fsm.h"

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"
#include "message_queue.h"


static void init_cblk(
    sCHAT_EVENT_IO_CBLK* master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);
    memset(master_cblk_ptr, 0, sizeof(sCHAT_EVENT_IO_CBLK));
    master_cblk_ptr->state = CHAT_EVENT_IO_STATE_READY;

    pthread_mutex_init(master_cblk_ptr->mutex, NULL);
}


static void close_cblk(
    sCHAT_EVENT_IO_CBLK* master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);

    pthread_mutex_destroy(master_cblk_ptr->mutex);
    master_cblk_ptr->deallocator(master_cblk_ptr);
}


eSTATUS chat_event_io_create(
    CHAT_EVENT_IO*            out_new_chat_event_io,
    fGENERIC_ALLOCATOR        allocator,
    eCHAT_EVENT_IO_MODE       mode,
    int                       fd)
{
    sCHAT_EVENT_IO_CBLK*         new_chat_event_io;

    eSTATUS                status;
    sCHAT_EVENT_IO_MESSAGE message;

    assert(NULL != out_new_chat_event_io);
    assert(NULL != allocator);

    new_chat_event_io = (sCHAT_EVENT_IO_CBLK*)allocator(sizeof(sCHAT_EVENT_IO_CBLK));
    if (NULL == new_chat_event_io)
    {
        return STATUS_ALLOC_FAILED;
    }

    init_cblk(new_chat_event_io);
    new_chat_event_io->mode = mode;
    new_chat_event_io->fd   = fd;

    *out_new_chat_event_io = (void*)new_chat_event_io;
    return STATUS_SUCCESS;
}


sCHAT_EVENT_IO_RESULT chat_event_io_populate_writer(
    CHAT_EVENT_IO      chat_event_io_writer,
    const sCHAT_EVENT* event)
{
    sCHAT_EVENT_IO_CBLK*   master_cblk_ptr;
    sCHAT_EVENT_IO_MESSAGE message;
    sCHAT_EVENT_IO_RESULT  result;
    
    assert(NULL != chat_event_io_writer);
    assert(NULL != event);

    if (event->length > CHAT_EVENT_MAX_DATA_SIZE)
    {
        result.event = CHAT_EVENT_IO_EVENT_POPULATE_FAILED;
        return result;
    }

    master_cblk_ptr = (sCHAT_EVENT_IO_CBLK*)chat_event_io_writer;

    message.type = CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER;
    memcpy(&message.params.populate_writer.event,
           event,
           sizeof(message.params.populate_writer.event));
    
    return chat_event_io_operation_entry(master_cblk_ptr, &message);
}


sCHAT_EVENT_IO_RESULT chat_event_io_do_operation(
    CHAT_EVENT_IO chat_event_io,
    sCHAT_EVENT*  out_event)
{
    sCHAT_EVENT_IO_CBLK*   master_cblk_ptr;
    sCHAT_EVENT_IO_MESSAGE message;

    assert(NULL != chat_event_io);
    master_cblk_ptr = (sCHAT_EVENT_IO_CBLK*)chat_event_io;

    message.type = CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE;
    return chat_event_io_operation_entry(master_cblk_ptr, &message);
}


eSTATUS chat_event_io_close(
    CHAT_EVENT_IO chat_event_io)
{
    sCHAT_EVENT_IO_CBLK*   master_cblk_ptr;
    
    assert(NULL != chat_event_io);
    master_cblk_ptr = (sCHAT_EVENT_IO_CBLK*)chat_event_io;
    
    close_cblk(master_cblk_ptr);
    return STATUS_SUCCESS;
}
