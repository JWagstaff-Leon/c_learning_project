#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stddef.h>


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef void* SHARED_PTR;


typedef void (*fSHARED_PTR_CLEANUP)(
    void* pointee);


// Functions -------------------------------------------------------------------

SHARED_PTR shared_ptr_create(
    size_t              memory_size,
    fSHARED_PTR_CLEANUP cleanup_cback);


void* shared_ptr_get_pointee(
    SHARED_PTR shared_ptr);

SHARED_PTR shared_ptr_share(
    SHARED_PTR shared_ptr);


void shared_ptr_release(
    SHARED_PTR shared_ptr);


#ifdef __cplusplus
}
#endif
