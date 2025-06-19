#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <ncurses.h>

#include "common_types.h"
#include "message_queue.h"


// Constants -------------------------------------------------------------------

#define CLIENT_UI_MESSAGE_QUEUE_SIZE 32

// The intersection of isprint and isspace, excepting '\n'
#define input_case 0x09: \
              case 0x0B: \
              case 0x0C: \
              case 0x0D: \
              case 0x20: \
              case 0x21: \
              case 0x22: \
              case 0x23: \
              case 0x24: \
              case 0x25: \
              case 0x26: \
              case 0x27: \
              case 0x28: \
              case 0x29: \
              case 0x2A: \
              case 0x2B: \
              case 0x2C: \
              case 0x2D: \
              case 0x2E: \
              case 0x2F: \
              case 0x30: \
              case 0x31: \
              case 0x32: \
              case 0x33: \
              case 0x34: \
              case 0x35: \
              case 0x36: \
              case 0x37: \
              case 0x38: \
              case 0x39: \
              case 0x3A: \
              case 0x3B: \
              case 0x3C: \
              case 0x3D: \
              case 0x3E: \
              case 0x3F: \
              case 0x40: \
              case 0x41: \
              case 0x42: \
              case 0x43: \
              case 0x44: \
              case 0x45: \
              case 0x46: \
              case 0x47: \
              case 0x48: \
              case 0x49: \
              case 0x4A: \
              case 0x4B: \
              case 0x4C: \
              case 0x4D: \
              case 0x4E: \
              case 0x4F: \
              case 0x50: \
              case 0x51: \
              case 0x52: \
              case 0x53: \
              case 0x54: \
              case 0x55: \
              case 0x56: \
              case 0x57: \
              case 0x58: \
              case 0x59: \
              case 0x5A: \
              case 0x5B: \
              case 0x5C: \
              case 0x5D: \
              case 0x5E: \
              case 0x5F: \
              case 0x60: \
              case 0x61: \
              case 0x62: \
              case 0x63: \
              case 0x64: \
              case 0x65: \
              case 0x66: \
              case 0x67: \
              case 0x68: \
              case 0x69: \
              case 0x6A: \
              case 0x6B: \
              case 0x6C: \
              case 0x6D: \
              case 0x6E: \
              case 0x6F: \
              case 0x70: \
              case 0x71: \
              case 0x72: \
              case 0x73: \
              case 0x74: \
              case 0x75: \
              case 0x76: \
              case 0x77: \
              case 0x78: \
              case 0x79: \
              case 0x7A: \
              case 0x7B: \
              case 0x7C: \
              case 0x7D: \
              case 0x7E

#define backspace_case 127: \
                  case KEY_BACKSPACE

#define ctrl_c_case ('c' & 0x1f)


// Types -----------------------------------------------------------------------

typedef enum
{
    CLIENT_UI_STATE_OPEN,
    
    CLIENT_UI_STATE_CLOSED
} eCLIENT_UI_STATE;


typedef struct
{
    fCLIENT_UI_CBACK user_cback;
    void*            user_arg;

    MESSAGE_QUEUE    message_queue;
    eCLIENT_UI_STATE state;

    THREAD_ID input_thread;

    WINDOW *input_window, *messages_window;
} sCLIENT_UI_CBLK;


// Functions -------------------------------------------------------------------

void* client_ui_thread_entry(
    void* arg);


void* client_ui_input_thread_entry(
    void* arg);


#ifdef __cplusplus
}
#endif
