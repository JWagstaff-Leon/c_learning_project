#include "shared_ptr.h"
#include "shared_ptr_internal.h"

#include "common_types.h"


#include <stdio.h>
SHARED_PTR shared_ptr_create_real(
    size_t              memory_size,
    fSHARED_PTR_CLEANUP cleanup_cback,
    const char* file,
    const char* func,
    unsigned int line)
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

    fprintf(stderr, "### %s\n    %s:%u - %s\n    Created ptr %p -> %p (%lux)\n", __func__, file, line, func, new_shared_ptr, new_shared_ptr->pointee, new_shared_ptr->use_count);
    return new_shared_ptr;
}


SHARED_PTR shared_ptr_share_real(
    SHARED_PTR shared_ptr,
    const char* file,
    const char* func,
    unsigned int line)
{
fprintf(stderr, "### %s\n    %s:%u - %s\n    ptr = %p -> %p (%lux)\n", __func__, file, line, func, (sSHARED_PTR*)shared_ptr, ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->pointee : NULL), ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->use_count : 0));
    if (NULL != shared_ptr)
    {
        ((sSHARED_PTR*)shared_ptr)->use_count += 1;
    }
fprintf(stderr, "    exiting with ptr = %p -> %p (%lux)\n", (sSHARED_PTR*)shared_ptr, ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->pointee : NULL), ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->use_count : 0));

    return shared_ptr;
}


void shared_ptr_release_real(
    SHARED_PTR shared_ptr,
    const char* file,
    const char* func,
    unsigned int line)
{
fprintf(stderr, "### %s\n    %s:%u - %s\n    ptr = %p -> %p (%lux)\n", __func__, file, line, func, (sSHARED_PTR*)shared_ptr, ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->pointee : NULL), ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->use_count : 0));
    sSHARED_PTR* ptr;
    if (NULL != (ptr = (sSHARED_PTR*)shared_ptr))
    {
        ptr->use_count -= 1;

        if (0 == ptr->use_count)
        {
            if (NULL != ptr->cleanup_cback)
            {
fprintf(stderr, "    cleanup cback...\n");
                ptr->cleanup_cback(ptr->pointee);
            }

            generic_deallocator(ptr->pointee);
            generic_deallocator(ptr);
        }
    }
fprintf(stderr, "    exiting with ptr = %p -> %p (%lux)\n", (sSHARED_PTR*)shared_ptr, ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->pointee : NULL), ((shared_ptr != NULL) ? ((sSHARED_PTR*)shared_ptr)->use_count : 0));
}
