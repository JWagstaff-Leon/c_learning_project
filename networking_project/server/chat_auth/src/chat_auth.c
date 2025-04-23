#include "chat_auth.h"
#include "chat_auth_internal.h"
#include "chat_auth_fsm.h"

#include <string.h>

#include "chat_user.h"
#include "common_types.h"

static sCHAT_AUTH_CBLK k_master_cblk;
static bool            init_flag = false;


eSTATUS chat_auth_open(
    void)
{
    eSTATUS            status;
    sCHAT_AUTH_MESSAGE message;
    
    if (!init_flag)
    {
        memset(&k_master_cblk, 0, sizeof(k_master_cblk));

        k_master_cblk.state = CHAT_AUTH_STATE_INIT;
        
        status = message_queue_create(&k_master_cblk.message_queue,
                                      CHAT_AUTH_MESSAGE_QUEUE_SIZE,
                                      sizeof(sCHAT_AUTH_MESSAGE));
        if (STATUS_SUCCESS != status)
        {
            return status;
        }
        
        status = generic_create_thread(chat_auth_thread_entry, &k_master_cblk);
        if (STATUS_SUCCESS != status)
        {
            return status;
        }

        init_flag = true;
    }

    message.type = CHAT_AUTH_MESSAGE_OPEN;
    status = message_queue_put(k_master_cblk.message_queue,
                               &message,
                               sizeof(message));
    return status;
}


eSTATUS chat_auth_submit_credentials(
    void*                  auth_object,
    sCHAT_USER_CREDENTIALS credentials)
{

}


eSTATUS chat_auth_shutdown(
    void)
{

}
