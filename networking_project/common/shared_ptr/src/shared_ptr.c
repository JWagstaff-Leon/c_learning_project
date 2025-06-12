#include "shared_ptr.h"
#include "shared_ptr_internal.h"

#include "common_types.h"


SHARED_PTR shared_ptr_create(
    size_t              memory_size,
    fSHARED_PTR_CLEANUP cleanup_cback)
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

    return new_shared_ptr;
}


SHARED_PTR shared_ptr_share(
    SHARED_PTR shared_ptr)
{
    if (NULL != shared_ptr)
    {
        ((sSHARED_PTR*)shared_ptr)->use_count += 1;
    }

    return shared_ptr;
}


void shared_ptr_release(
    SHARED_PTR shared_ptr)
{
    sSHARED_PTR* ptr;
    if (NULL != (ptr = (sSHARED_PTR*)shared_ptr))
    {
        ptr->use_count -= 1;

        if (0 == ptr->use_count)
        {
            if (NULL != ptr->cleanup_cback)
            {
                ptr->cleanup_cback(ptr->pointee);
            }

            generic_deallocator(ptr->pointee);
            generic_deallocator(ptr);
        }
    }
}
