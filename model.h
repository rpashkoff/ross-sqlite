#ifndef __MODEL_H__
#define __MODEL_H__

#include "ross.h"
#include "sqlite3.h"
#include "log.h"

#define MEM_POOL_SIZE (512 * 1024 * 1024)


typedef enum model_message_e model_message_e;  
enum model_message_e {
    EVENT_INC =  1,
    EVENT_DEC,
};


typedef struct model_message_t model_message_t;
struct model_message_t {
    tw_lpid sender;
    model_message_e type;
};


typedef struct model_state_t model_state_t; 
struct model_state_t {
    void * db;
    char mem_pool[MEM_POOL_SIZE];
};


#endif