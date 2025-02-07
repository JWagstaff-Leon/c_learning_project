#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"
#include "chat_event_io_network_fsm.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"
#include "message_queue.h"


static void init_cblk(
    sCHAT_EVENT_IO_CBLK* master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);
    memset(master_cblk_ptr, 0, sizeof(sCHAT_EVENT_IO_CBLK));

    master_cblk_ptr->reader.state = CHAT_EVENT_IO_STATE_READY;
    master_cblk_ptr->writer.state = CHAT_EVENT_IO_STATE_READY;
}


static void close_cblk(
    sCHAT_EVENT_IO_CBLK* master_cblk_ptr)
{
    generic_deallocator(master_cblk_ptr);
}


eSTATUS chat_event_io_create(
    CHAT_EVENT_IO*            out_new_chat_event_io)
{
    sCHAT_EVENT_IO_CBLK* new_chat_event_io;

    assert(NULL != out_new_chat_event_io);

    new_chat_event_io = (sCHAT_EVENT_IO_CBLK*)generic_allocator(sizeof(sCHAT_EVENT_IO_CBLK));
    if (NULL == new_chat_event_io)
    {
        return STATUS_ALLOC_FAILED;
    }

    init_cblk(new_chat_event_io);

    *out_new_chat_event_io = (void*)new_chat_event_io;
    return STATUS_SUCCESS;
}


sCHAT_EVENT_IO_RESULT chat_event_io_populate_writer(
    CHAT_EVENT_IO      chat_event_io_writer,
    const sCHAT_EVENT* event)
{
    sCHAT_EVENT_IO_MESSAGE message;
    sCHAT_EVENT_IO_RESULT  result;

    assert(NULL != chat_event_io_writer);
    assert(NULL != event);

    if (event->length > CHAT_EVENT_MAX_DATA_SIZE)
    {
        result.event = CHAT_EVENT_IO_EVENT_POPULATE_FAILED;
        return result;
    }

    message.type = CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER;
    memcpy(&message.params.populate_writer.event,
           event,
           sizeof(message.params.populate_writer.event));

    return chat_event_io_dispatch_message((sCHAT_EVENT_IO_CBLK*)chat_event_io_writer,
                                          &message);
}


sCHAT_EVENT_IO_RESULT chat_event_io_read_from_fd(
    CHAT_EVENT_IO chat_event_io,
    int           fd)
{
    sCHAT_EVENT_IO_MESSAGE message;

    assert(NULL != chat_event_io);
    assert(NULL != out_event);

    message.type = CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE;

    message.params.operate.fd   = fd;
    message.params.operate.mode = CHAT_EVENT_IO_OPERATION_MODE_READ;

    return chat_event_io_dispatch_message((sCHAT_EVENT_IO_CBLK*)chat_event_io,
                                          &message);
}


sCHAT_EVENT_IO_RESULT chat_event_io_write_to_fd(
    CHAT_EVENT_IO      chat_event_io,
    int                fd)
{
    sCHAT_EVENT_IO_MESSAGE message;

    assert(NULL != chat_event_io);

    message.type = CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE;

    message.params.operate.fd   = fd;
    message.params.operate.mode = CHAT_EVENT_IO_OPERATION_MODE_WRITE;

    return chat_event_io_dispatch_message((sCHAT_EVENT_IO_CBLK*)chat_event_io,
                                          &message);
}


eSTATUS chat_event_io_close(
    CHAT_EVENT_IO chat_event_io)
{
    assert(NULL != chat_event_io);
    close_cblk((sCHAT_EVENT_IO_CBLK*)chat_event_io);
    return STATUS_SUCCESS;
}
