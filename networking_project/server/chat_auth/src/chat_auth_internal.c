#include "chat_auth.h"
#include "chat_auth_internal.h"


eSTATUS chat_auth_read_database_file(
    sCHAT_AUTH_CBLK* master_cblk_ptr,
    const char*      path)
{
    // TODO this
}


eSTATUS chat_auth_save_database_file(
    sCHAT_AUTH_CBLK* master_cblk_ptr,
    const char*      path)
{
    if (NULL == path)
    {
        return STATUS_SUCCESS;
    }
    
    // TODO this
}
