#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef void* TIMER;

// Functions -------------------------------------------------------------------

eSTATUS timer_init(
    void);


eSTATUS timer_create(
    void);


eSTATUS timer_start(
    void);


eSTATUS timer_restart(
    void);


eSTATUS timer_stop(
    void);


eSTATUS timer_delete(
    void);


#ifdef __cplusplus
}
#endif
