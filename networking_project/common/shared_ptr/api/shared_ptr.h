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


#define shared_ptr_create(size, cleanup) shared_ptr_create_real (size, cleanup, __FILE__, __func__, __LINE__)
#define shared_ptr_share(ptr) shared_ptr_share_real (ptr, __FILE__, __func__, __LINE__)
#define shared_ptr_release(ptr) shared_ptr_release_real(ptr, __FILE__, __func__, __LINE__)

// Functions -------------------------------------------------------------------

SHARED_PTR shared_ptr_create_real(
    size_t              memory_size,
    fSHARED_PTR_CLEANUP cleanup_cback,
    const char* file,
    const char* func,
    unsigned int line);


SHARED_PTR shared_ptr_share_real(
    SHARED_PTR shared_ptr,
    const char* file,
    const char* func,
    unsigned int line);


void shared_ptr_release_real(
    SHARED_PTR shared_ptr,
    const char* file,
    const char* func,
    unsigned int line);


#ifdef __cplusplus
}
#endif
