#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_event_io.h"
#include "chat_event_io_fsm.h"

#include <poll.h>
#include <pthread.h>
#include <stdint.h>

#include "chat_event.h"
#include "message_queue.h"

// Constants -------------------------------------------------------------------

#define CHAT_EVENT_IO_MESSAGE_QUEUE_SIZE 32


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_EVENT_IO_OPERATOR_STATE_READY,
    CHAT_EVENT_IO_OPERATOR_STATE_IN_PROGRESS,
    CHAT_EVENT_IO_OPERATOR_STATE_FLUSHING
} eCHAT_EVENT_IO_OPERATOR_STATE;


typedef struct
{
    eCHAT_EVENT_IO_OPERATOR_STATE state;

    sCHAT_EVENT event;
    uint32_t    processed_bytes;
    uint32_t    flush_bytes;
} sCHAT_EVENT_IO_OPERATOR;


typedef struct
{
    sCHAT_EVENT_IO_OPERATOR reader;
    sCHAT_EVENT_IO_OPERATOR writer;
} sCHAT_EVENT_IO_CBLK;


// Functions -------------------------------------------------------------------

eCHAT_EVENT_IO_RESULT chat_event_io_operation_entry(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message);


eCHAT_EVENT_IO_RESULT chat_event_io_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message);
    

eCHAT_EVENT_IO_RESULT chat_event_io_reader_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message);


eCHAT_EVENT_IO_RESULT chat_event_io_writer_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message);


#ifdef __cplusplus
}
#endif
