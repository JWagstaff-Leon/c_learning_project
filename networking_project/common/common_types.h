#pragma once


#define BASE_ERROR_VALUE -1


typedef enum
{
    STATUS_SUCCESS = 0,

    STATUS_FAILURE        = BASE_ERROR_VALUE - 0,
    STATUS_INVALID_ARG    = BASE_ERROR_VALUE - 1,
    STATUS_ALLOC_FAILED   = BASE_ERROR_VALUE - 2,
    STATUS_DEALLOC_FAILED = BASE_ERROR_VALUE - 3,
    STATUS_NO_SPACE       = BASE_ERROR_VALUE - 4,
    STATUS_EMPTY          = BASE_ERROR_VALUE - 5,
    STATUS_INCOMPLETE     = BASE_ERROR_VALUE - 6,
    STATUS_CLOSED         = BASE_ERROR_VALUE - 7,
    STATUS_INVALID_STATE  = BASE_ERROR_VALUE - 8
} eSTATUS;


typedef void* (*fGENERIC_THREAD_ENTRY) (void*);


void* generic_allocator(
    size_t bytes);


void generic_deallocator(
    void* ptr);


eSTATUS generic_create_thread(
    fGENERIC_THREAD_ENTRY thread_entry,
    void* arg);
