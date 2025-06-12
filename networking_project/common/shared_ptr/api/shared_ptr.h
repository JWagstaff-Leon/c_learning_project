#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stddef.h>


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef void* SHARED_PTR;


typedef struct
{
    void* const pointee;
} SHARED_POINTEE;


#define SP_POINTEE(sp) (((SHARED_POINTEE*)sp)->pointee)
#define SP_POINTEE_AS(sp, type) ((type*)SP_POINTEE(sp))
#define SP_PROPERTY(sp, type, prop) (SP_POINTEE_AS(sp, type)->prop)


typedef void (*fSHARED_PTR_CLEANUP)(
    void* pointee);


// Functions -------------------------------------------------------------------

SHARED_PTR shared_ptr_create(
    size_t              memory_size,
    fSHARED_PTR_CLEANUP cleanup_cback);


SHARED_PTR shared_ptr_share(
    SHARED_PTR shared_ptr);


void shared_ptr_release(
    SHARED_PTR shared_ptr);


#ifdef __cplusplus
}
#endif
