#include "chat_event.h"

#include "chat_user.h"
#include "common_types.h"


eSTATUS chat_event_copy(
    sCHAT_EVENT*       restrict dst,
    const sCHAT_EVENT* restrict src)
{
    eSTATUS status;
    status = chat_event_populate(dst, src->type, src->origin, src->data);
    return status;
}


eSTATUS chat_event_populate(
    sCHAT_EVENT*     restrict event,
    eCHAT_EVENT_TYPE          type,
    CHAT_USER_ID              origin,
    const char*      restrict string)
{
    eSTATUS status;
    int     printed_length;

    event->type   = type;
    event->origin = origin;

    status = print_string_to_buffer((char*)event->data,
                                    string,
                                    sizeof(event->data),
                                    &event->length);
    return status;
}
