#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_event_io.h"
#include "chat_event_io_fsm.h"

#include <poll.h>
#include <stdint.h>

#include "chat_event.h"
#include "message_queue.h"

// Constants -------------------------------------------------------------------

#define CHAT_EVENT_IO_MESSAGE_QUEUE_SIZE 32


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_EVENT_IO_STATE_READY,
    CHAT_EVENT_IO_STATE_IN_PROGRESS,
    CHAT_EVENT_IO_STATE_FLUSHING,
    CHAT_EVENT_IO_STATE_CLOSED
} eCHAT_EVENT_IO_STATE;


typedef struct
{
    eCHAT_EVENT_IO_STATE state;
    MESSAGE_QUEUE        message_queue;

    eCHAT_EVENT_IO_MODE mode;
    int                 fd;

    fCHAT_EVENT_IO_USER_CBACK user_cback;
    void*                     user_arg;

    fGENERIC_ALLOCATOR   allocator;
    fGENERIC_DEALLOCATOR deallocator;

    sCHAT_EVENT event;
    uint32_t    processed_bytes;
    uint32_t    flush_bytes;
} sCHAT_EVENT_IO_CBLK;


// Functions -------------------------------------------------------------------

void* chat_event_io_thread_entry(
    void* arg);


void chat_event_io_reader_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message);


void chat_event_io_writer_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message);


#ifdef __cplusplus
}
#endif
