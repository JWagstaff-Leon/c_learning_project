#pragma once


#define BASE_STATUS_VALUE -1

#define mSTRINGIFY(x) _m_stringify(x)
#define _m_stringify(x) #x


typedef enum
{
    STATUS_SUCCESS = 0,

    STATUS_FAILURE        = BASE_STATUS_VALUE - 0,
    STATUS_INVALID_ARG    = BASE_STATUS_VALUE - 1,
    STATUS_ALLOC_FAILED   = BASE_STATUS_VALUE - 2,
    STATUS_DEALLOC_FAILED = BASE_STATUS_VALUE - 3,
    STATUS_NO_SPACE       = BASE_STATUS_VALUE - 4,
    STATUS_EMPTY          = BASE_STATUS_VALUE - 5,
    STATUS_INCOMPLETE     = BASE_STATUS_VALUE - 6,
    STATUS_CLOSED         = BASE_STATUS_VALUE - 7,
    STATUS_INVALID_STATE  = BASE_STATUS_VALUE - 8,
    STATUS_NOT_FOUND      = BASE_STATUS_VALUE - 9
} eSTATUS;


typedef enum
{
    PIPE_END_READ  = 0,
    PIPE_END_WRITE = 1
} ePIPE_END;


typedef void* (*fGENERIC_THREAD_ENTRY) (void*);


void* generic_allocator(
    size_t bytes);


void generic_deallocator(
    void* ptr);


eSTATUS generic_create_thread(
    fGENERIC_THREAD_ENTRY thread_entry,
    void* arg);


eSTATUS print_string_to_buffer(
    char*       restrict buffer,
    const char* restrict string,
    size_t               buffer_size,
    int*        restrict out_printed_length);
