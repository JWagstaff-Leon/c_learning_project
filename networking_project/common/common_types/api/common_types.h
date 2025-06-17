#pragma once

#include <endian.h>
#include <pthread.h>
#include <stddef.h>

#define BASE_STATUS_VALUE -1

#define mSTRINGIFY(x) _m_stringify(x)
#define _m_stringify(x) #x

#define htonll htobe64
#define ntohll be64toh


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


typedef enum
{
    THREAD_KILL_DEFERRED,
    THREAD_KILL_INSTANT
} eTHREAD_KILL_MODE;


typedef void* (*fGENERIC_THREAD_ENTRY) (void*);
typedef pthread_t THREAD_ID;

// TODO refactor for these
// typedef pthread_mutex_t   GENERIC_MUTEX;
// typedef pthread_condvar_t GENERIC_CONDAVR;


void* generic_allocator(
    size_t bytes);


void generic_deallocator(
    void* ptr);


eSTATUS generic_create_thread(
    fGENERIC_THREAD_ENTRY thread_entry,
    void*                 arg,
    THREAD_ID*            out_new_thread);


eSTATUS generic_thread_set_kill_mode(
    eTHREAD_KILL_MODE mode);


eSTATUS generic_kill_thread(
    THREAD_ID thread_id);


eSTATUS print_string_to_buffer(
    char*       restrict buffer,
    const char* restrict string,
    size_t               buffer_size,
    int*        restrict out_printed_length);
