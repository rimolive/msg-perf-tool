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
#ifndef SENDER_H
#define SENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/wait.h>

#include "contrib/options.h"
#include "contrib/logger.h"
#include "contrib/log.h"

#include "vmsl.h"
#include "message_sender.h"
#include "process_utils.h"

#if defined(__STOMP_SUPPORT__)
 #include "stomp-wrapper.h"
#endif // __STOMP_SUPPORT__

#if defined(__AMQP_SUPPORT__)
 #include "proton-wrapper.h"
#endif // __AMQP_SUPPORT__



#ifdef __cplusplus
}
#endif

#endif /* SENDER_H */
