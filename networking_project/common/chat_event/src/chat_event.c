#include "chat_event.h"


eSTATUS chat_event_copy(
    sCHAT_EVENT*       restrict dst,
    const sCHAT_EVENT* restrict src)
{
    eSTATUS status;

    status = chat_event_fill_origin(dst, src->origin.name, src->origin.id);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }
    
    status = chat_event_fill_content(dst, src->type, src->data);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    return STATUS_SUCCESS;
}


eSTATUS chat_event_fill_content(
    sCHAT_EVENT*     restrict event,
    eCHAT_EVENT_TYPE          type,
    const char*      restrict string)
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
    sCHAT_EVENT* restrict event,
    const char*  restrict name,
    CHAT_USER_ID          id)
{
    eSTATUS status;

    event->origin.id = id;

    status = print_string_to_buffer(event->origin.name,
                                    name,
                                    sizeof(event->origin.name),
                                    NULL);
    return status;
}
