#include "chat_auth.h"
#include "chat_auth_internal.h"

void* chat_auth_thread_entry(
    void* arg)
{
    sCHAT_AUTH_CBLK* master_cblk_ptr;
    master_cblk_ptr = (sCHAT_AUTH_CBLK*)arg;

    while (CHAT_AUTH_STATE_CLOSED != master_cblk_ptr->state)
    {
        
    }
}
