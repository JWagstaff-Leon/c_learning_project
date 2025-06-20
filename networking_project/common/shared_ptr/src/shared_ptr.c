#include "shared_ptr.h"
#include "shared_ptr_internal.h"

#include "common_types.h"


#ifdef SHARED_PTR_DEBUG
#include <stdio.h>
#endif

#ifdef SHARED_PTR_DEBUG
SHARED_PTR shared_ptr_create_real(
    size_t              memory_size,
    fSHARED_PTR_CLEANUP cleanup_cback,
    const char* file,
    const char* func,
    unsigned int line)
#else    
SHARED_PTR shared_ptr_create(
    size_t              memory_size,
    fSHARED_PTR_CLEANUP cleanup_cback)
#endif
{
    sSHARED_PTR* new_shared_ptr = generic_allocator(sizeof(sSHARED_PTR));
    if (NULL == new_shared_ptr)
    {
        return NULL;
    }

    new_shared_ptr->pointee = generic_allocator(memory_size);
    if (NULL == new_shared_ptr->pointee)
    {
        generic_deallocator(new_shared_ptr);
        return NULL;
    }

    new_shared_ptr->use_count     = 1;
    new_shared_ptr->cleanup_cback = cleanup_cback;

#ifdef SHARED_PTR_DEBUG
    fprintf(stderr, "### %s\n    %s:%u - %s\n    Created ptr %p -> %p (%lux)\n", __func__, file, line, func, new_shared_ptr, new_shared_ptr->pointee, new_shared_ptr->use_count);
#endif
    return new_shared_ptr;
}

#ifdef SHARED_PTR_DEBUG
SHARED_PTR shared_ptr_share_real(
    SHARED_PTR shared_ptr,
    const char* file,
    const char* func,
    unsigned int line)
#else
SHARED_PTR shared_ptr_share(
    SHARED_PTR shared_ptr)
#endif
{
#ifdef SHARED_PTR_DEBUG
    fprintf(stderr, "### %s\n    %s:%u - %s\n    ptr = %p -> %p (%lux)\n", __func__, file, line, func, (sSHARED_PTR*)shared_ptr, ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->pointee : NULL), ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->use_count : 0));
#endif

    if (NULL != shared_ptr)
    {
        ((sSHARED_PTR*)shared_ptr)->use_count += 1;
    }

#ifdef SHARED_PTR_DEBUG
    fprintf(stderr, "    exiting with ptr = %p -> %p (%lux)\n", (sSHARED_PTR*)shared_ptr, ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->pointee : NULL), ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->use_count : 0));
#endif

    return shared_ptr;
}


#ifdef SHARED_PTR_DEBUG
void shared_ptr_release_real(
    SHARED_PTR shared_ptr,
    const char* file,
    const char* func,
    unsigned int line)
#else
void shared_ptr_release(
    SHARED_PTR shared_ptr)
#endif
{
    sSHARED_PTR* ptr;

#ifdef SHARED_PTR_DEBUG
    fprintf(stderr, "### %s\n    %s:%u - %s\n    ptr = %p -> %p (%lux)\n", __func__, file, line, func, (sSHARED_PTR*)shared_ptr, ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->pointee : NULL), ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->use_count : 0));
#endif

    if (NULL != (ptr = (sSHARED_PTR*)shared_ptr))
    {
        ptr->use_count -= 1;

#ifdef SHARED_PTR_DEBUG
        fprintf(stderr, "    exiting with ptr = %p -> %p (%lux)\n", (sSHARED_PTR*)shared_ptr, ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->pointee : NULL), ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->use_count : 0));
#endif

        if (0 == ptr->use_count)
        {
            if (NULL != ptr->cleanup_cback)
            {
#ifdef SHARED_PTR_DEBUG
                fprintf(stderr, "    cleanup cback...\n");
#endif
                ptr->cleanup_cback(ptr->pointee);
            }

            generic_deallocator(ptr->pointee);
            generic_deallocator(ptr);
        }
    }
}
