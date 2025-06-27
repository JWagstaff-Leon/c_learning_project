#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "shared_ptr.h"

#include <stdint.h>


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef struct
{
    void*    pointee;
    uint64_t use_count;

    fSHARED_PTR_CLEANUP cleanup_cback;
} sSHARED_PTR;

// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
