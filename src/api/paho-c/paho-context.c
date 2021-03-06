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
#include "paho-context.h"

paho_ctxt_t *paho_context_init() {
    paho_ctxt_t *ret = malloc(sizeof(paho_ctxt_t));

    if (!ret) {
        return NULL;
    }

    return ret;
}

void paho_context_destroy(paho_ctxt_t **ctxt) {
    free(*ctxt);
    *ctxt = NULL;
}