#include "chat_event.h"

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
