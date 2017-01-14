/**
 Copyright 2015 Otavio Rodolfo Piske

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
#include "options.h"

static options_t *options = NULL;

options_t *options_new() {
	options_t *ret = (options_t *) calloc(1, sizeof(options_t));

	if (!ret) {
		fprintf(stderr, "Not enough memory to allocate for options object\n");

		return NULL;
	}
	bzero(ret->url, sizeof(ret->url));
	bzero(ret->logdir, sizeof(ret->logdir));
	ret->daemon = false;

	return ret;
}

void options_destroy(options_t **obj) {
	free(*obj);
	*obj = NULL;
}

void options_set_defaults(options_t *ret) {
	strncpy(ret->url, "amqp://localhost:5672/test.performance.queue", sizeof(ret->url));

	strcpy(ret->logdir, ".");

	ret->parallel_count = 2;
	ret->count = 0;
	ret->log_level = INFO;
	ret->message_size = 32;
	ret->duration = gru_duration_from_seconds(0);
	ret->daemon = false;
	ret->throttle = 0;
}

void set_options_object(options_t *obj) {
	if (options == NULL) {
		options = obj;
	}
}

const options_t *get_options_object(void) { return options; }