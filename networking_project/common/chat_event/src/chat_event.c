#include "chat_event.h"

#include <assert.h>
#include <stdio.h>


eSTATUS chat_event_copy(
    sCHAT_EVENT*       restrict dst,
    const sCHAT_EVENT* restrict src)
{
    int snprintf_status;

    dst->type = src->type;
    dst->origin.id = src->origin.id;

    snprintf_status = snprintf(dst->origin.name,
                               sizeof(dst->origin.name),
                               "%s",
                               src->origin.name);
    if (snprintf_status < 0)
    {
        return STATUS_FAILURE;
    }

    snprintf_status = snprintf(dst->data,
                               sizeof(dst->data),
                               "%s",
                               src->data);
    if (snprintf_status < 0)
    {
        snprintf_status = 0;
    }
    if (snprintf_status > CHAT_EVENT_MAX_DATA_SIZE)
    {
        return STATUS_NO_SPACE;
    }
    dst->length = snprintf_status;

    return STATUS_SUCCESS;
}


eSTATUS chat_event_fill_content(
    sCHAT_EVENT*     event,
    const char*      string,
    eCHAT_EVENT_TYPE type)
{
    eSTATUS status;
    int     printed_length;

    event->type = type;

    status = print_string_to_buffer((char*)event->data,
                                    string,
                                    sizeof(event->data),
                                    &printed_length);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    event->length = printed_length;
    return STATUS_SUCCESS;
}


eSTATUS chat_event_fill_origin(
    sCHAT_EVENT* event,
    const char*  name,
    CHAT_USER_ID id)
{
    eSTATUS status;

    event->origin.id = id;

    status = print_string_to_buffer(event->origin.name,
                                    name,
                                    sizeof(event->origin.name),
                                    NULL);
    return status;
}
