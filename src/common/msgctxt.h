/**
 Copyright 2016 Otavio Rodolfo Piske
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */
#ifndef MSGCTX_H
#define MSGCTX_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdlib.h>
#include <stdint.h>

#include "statistics.h"
#include <log/gru_logger.h>
    
typedef struct msg_content_data_t_ {
    uint64_t count;
    size_t capacity;
    size_t size;
    void *data;
} msg_content_data_t;

typedef void(*msg_content_loader)(msg_content_data_t *content_data);

typedef struct msg_ctxt_t_ {
    void *api_context;
    stat_io_t *stat_io;
} msg_ctxt_t;

msg_ctxt_t *msg_ctxt_init(stat_io_t *stat_io);
void msg_ctxt_destroy(msg_ctxt_t **ctxt);


#ifdef __cplusplus
}
#endif

#endif /* MSGCTX_H */

