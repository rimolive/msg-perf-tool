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
#include <network/gru_uri.h>

#include "proton-wrapper.h"
#include "proton-context.h"
#include "vmsl.h"

const int window = 10;
const char *url = NULL;

static inline bool failed(pn_messenger_t *messenger) {
	if (pn_messenger_errno(messenger)) {
		return true;
	}

	return false;
}

static inline proton_ctxt_t *proton_ctxt_cast(msg_ctxt_t *ctxt) {
	return (proton_ctxt_t *) ctxt->api_context;
}

static void proton_set_send_options(pn_messenger_t *messenger, msg_opt_t opt){
	if (opt.qos == MSG_QOS_AT_MOST_ONCE) {
		/**
		 * From the documentation:
		 *
		 * Sender presettles (aka at-most-once): "... in this configuration the sender
		 * settles (i.e. forgets about) the delivery before it even reaches the receiver,
		 * and if anything should happen to the delivery in-flight, there is no way to
		 * recover, hence the "at most once" semantics ..."
		 */
		mpt_trace("Using At most once");
		pn_messenger_set_snd_settle_mode(messenger, PN_SND_SETTLED);
	}
	else {
		logger_t logger = gru_logger_get();

		logger(WARNING, "Using an unsupported QOS mode");

		pn_messenger_set_outgoing_window(messenger, 1);
		pn_messenger_set_snd_settle_mode(messenger, PN_SND_UNSETTLED);
	}
}

static void proton_set_recv_options(pn_messenger_t *messenger, msg_opt_t opt){
	logger_t logger = gru_logger_get();

	if (opt.qos == MSG_QOS_AT_LEAST_ONCE) {
		logger(INFO, "Setting QOS to At least once");
		pn_messenger_set_rcv_settle_mode(messenger, PN_RCV_FIRST);
	}
	else {
		if (opt.qos == MSG_QOS_EXACTLY_ONCE) {
			logger(INFO, "Setting QOS to exactly once");
			pn_messenger_set_rcv_settle_mode(messenger, PN_RCV_SECOND);
		}
	}
}

msg_ctxt_t *proton_init(stat_io_t *stat_io, msg_opt_t opt, void *data, gru_status_t *status) {
	logger_t logger = gru_logger_get();

	logger(DEBUG, "Initializing proton wrapper");

	msg_ctxt_t *msg_ctxt = msg_ctxt_init(stat_io, status);
	if (!msg_ctxt) {
		return NULL;
	}

	proton_ctxt_t *proton_ctxt = proton_context_init();

	if (!proton_ctxt) {
		logger(FATAL, "Unable to initialize the proton context");

		goto err_exit;
	}

	pn_messenger_t *messenger = pn_messenger(NULL);

	logger(DEBUG, "Initializing the proton messenger");
	int err = pn_messenger_start(messenger);
	if (err) {
		logger(FATAL, "Unable to start the proton messenger");

		proton_context_destroy(&proton_ctxt);
		goto err_exit;
	}

	if (opt.direction == MSG_DIRECTION_SENDER) {
		proton_set_send_options(messenger, opt);
	}
	else {
		proton_set_recv_options(messenger, opt);
	}

	const options_t *options = get_options_object();

	url = gru_uri_simple_format(&options->uri, status);
	if (status->code != GRU_SUCCESS) {
		goto err_exit;
	}

	proton_ctxt->messenger = messenger;
	msg_ctxt->api_context = proton_ctxt;
	msg_ctxt->msg_opts = opt;

	return msg_ctxt;

	err_exit:
	msg_ctxt_destroy(&msg_ctxt);
	return NULL;
}

void proton_stop(msg_ctxt_t *ctxt, gru_status_t *status) {
	proton_ctxt_t *proton_ctxt = proton_ctxt_cast(ctxt);

	pn_messenger_stop(proton_ctxt->messenger);
}

void proton_destroy(msg_ctxt_t *ctxt, gru_status_t *status) {
	proton_ctxt_t *proton_ctxt = proton_ctxt_cast(ctxt);

	pn_messenger_free(proton_ctxt->messenger);
	proton_context_destroy(&proton_ctxt);

	msg_ctxt_destroy(&ctxt);
}

static void proton_check_status(pn_messenger_t *messenger, pn_tracker_t tracker) {
	logger_t logger = gru_logger_get();

	pn_status_t status = pn_messenger_status(messenger, tracker);

	logger(TRACE, "Checking message status");
	switch (status) {
		case PN_STATUS_UNKNOWN: {
			logger(TRACE, "Message status unknown");
			break;
		}
		case PN_STATUS_PENDING: {
			logger(TRACE, "Message status pending");
			break;
		}
		case PN_STATUS_ACCEPTED: {
			logger(TRACE, "Message status accepted");
			break;
		}
		case PN_STATUS_REJECTED: {
			logger(TRACE, "Message status rejected");
			break;
		}
		case PN_STATUS_RELEASED: {
			logger(TRACE, "Message status released");
			break;
		}
		case PN_STATUS_MODIFIED: {
			logger(TRACE, "Message status modified");
			break;
		}
		case PN_STATUS_ABORTED: {
			logger(TRACE, "Message status aborted");
			break;
		}
		case PN_STATUS_SETTLED: {
			logger(TRACE, "Message status settled");
			break;
		}
		default: {
			logger(TRACE, "Message status invalid");
			break;
		}
	}
}

static void proton_commit(pn_messenger_t *messenger, gru_status_t *status) {
	pn_tracker_t tracker = pn_messenger_outgoing_tracker(messenger);

	mpt_trace("Committing the message delivery");

#if defined(MPT_DEBUG) && MPT_DEBUG >=1
	proton_check_status(messenger, tracker);
#endif
	pn_messenger_settle(messenger, tracker, 0);

#if defined(MPT_DEBUG) && MPT_DEBUG >=1
	proton_check_status(messenger, tracker);
#endif
}

static pn_timestamp_t proton_now(gru_status_t *status) {
	struct timeval now;

	if (gettimeofday(&now, NULL)) {
		gru_status_strerror(status, GRU_FAILURE, errno);

		return -1;
	}

	return ((pn_timestamp_t) now.tv_sec) * 1000 + (now.tv_usec / 1000);
}

static void proton_set_message_properties(msg_ctxt_t *ctxt, pn_message_t *message,
										  gru_status_t *status) {

	mpt_trace("Setting message address to %s", url);
	pn_message_set_address(message, url);

	// OPT_TODO: must be a configuration
	pn_message_set_durable(message, false);
	pn_message_set_ttl(message, 50000);

	pn_message_set_creation_time(message, proton_now(status));
}

static void proton_set_message_data(
	pn_message_t *message, msg_content_loader content_loader) {
	static bool cached = false;
	static msg_content_data_t msg_content;

	mpt_trace("Formatting message body");

	pn_data_t *body = pn_message_body(message);
	if (!cached) {
		content_loader(&msg_content);
		cached = true;
	}

	pn_data_put_string(body, pn_bytes(msg_content.capacity, msg_content.data));
}

static vmsl_stat_t proton_do_send(pn_messenger_t *messenger, pn_message_t *message,
						   gru_status_t *status) {
	mpt_trace("Putting message");
	pn_messenger_put(messenger, message);
	if (failed(messenger)) {
		pn_error_t *error = pn_messenger_error(messenger);

		const char *protonErrorText = pn_error_text(error);
		gru_status_set(status, GRU_FAILURE, protonErrorText);

		return VMSL_ERROR;
	}

	pn_messenger_send(messenger, -1);
	if (failed(messenger)) {
		pn_error_t *error = pn_messenger_error(messenger);

		const char *protonErrorText = pn_error_text(error);
		gru_status_set(status, GRU_FAILURE, protonErrorText);
		return VMSL_ERROR;
	}

	return VMSL_SUCCESS;
}

vmsl_stat_t proton_send(msg_ctxt_t *ctxt, msg_content_loader content_loader, gru_status_t *status) {
	vmsl_stat_t ret = {0};

	mpt_trace("Creating message object");
	pn_message_t *message = pn_message();


	proton_set_message_properties(ctxt, message, status);
	proton_set_message_data(message, content_loader);

	proton_ctxt_t *proton_ctxt = proton_ctxt_cast(ctxt);

	ret = proton_do_send(proton_ctxt->messenger, message, status);
	if (vmsl_stat_error(ret)) {
		return ret;
	}

	if (unlikely(ctxt->msg_opts.qos != MSG_QOS_AT_MOST_ONCE)) {
		proton_commit(proton_ctxt->messenger, status);
	}

	pn_message_free(message);
	return VMSL_SUCCESS;
}

static void proton_accept(pn_messenger_t *messenger) {
	pn_tracker_t tracker = pn_messenger_incoming_tracker(messenger);

	mpt_trace("Accepting the message delivery");

#if defined(MPT_DEBUG) && MPT_DEBUG >=1
	proton_check_status(messenger, tracker);
#endif
	pn_messenger_accept(messenger, tracker, PN_CUMULATIVE);
	pn_messenger_settle(messenger, tracker, PN_CUMULATIVE);

#if defined(MPT_DEBUG) && MPT_DEBUG >=1
	proton_check_status(messenger, tracker);
#endif
}

static void proton_reject(pn_messenger_t *messenger) {
	pn_tracker_t tracker = pn_messenger_incoming_tracker(messenger);

	mpt_trace("Accepting the message delivery");

#if defined(MPT_DEBUG) && MPT_DEBUG >=1
	proton_check_status(messenger, tracker);
#endif
	pn_messenger_reject(messenger, tracker, PN_CUMULATIVE);
	pn_messenger_settle(messenger, tracker, PN_CUMULATIVE);

#if defined(MPT_DEBUG) && MPT_DEBUG >=1
	proton_check_status(messenger, tracker);
#endif
}

static void proton_set_incoming_messenger_properties(pn_messenger_t *messenger) {
	pn_messenger_set_incoming_window(messenger, window);
}

vmsl_stat_t proton_subscribe(msg_ctxt_t *ctxt, void *data, gru_status_t *status) {
	logger_t logger = gru_logger_get();
	proton_ctxt_t *proton_ctxt = proton_ctxt_cast(ctxt);

	logger(INFO, "Subscribing to endpoint address at %s", url);
	pn_messenger_subscribe(proton_ctxt->messenger, url);
	if (failed(proton_ctxt->messenger)) {
		pn_error_t *error = pn_messenger_error(proton_ctxt->messenger);

		const char *protonErrorText = pn_error_text(error);
		gru_status_set(status, GRU_FAILURE, protonErrorText);

		return VMSL_ERROR;
	}

	proton_set_incoming_messenger_properties(proton_ctxt->messenger);
	return VMSL_SUCCESS;
}

static int proton_receive_local(pn_messenger_t *messenger, gru_status_t *status)
{
	int limit = window * 10;
	mpt_trace("Receiving at most %i messages", limit);
	pn_messenger_recv(messenger, 1024);
	if (failed(messenger)) {
		pn_error_t *error = pn_messenger_error(messenger);

		const char *protonErrorText = pn_error_text(error);

		gru_status_set(status, GRU_FAILURE, protonErrorText);

		return -1;
	}

	return 0;
}

static int proton_do_receive(
	pn_messenger_t *messenger, pn_message_t *message, msg_content_data_t *content) {

	pn_messenger_get(messenger, message);
	if (failed(messenger)) {
		logger_t logger = gru_logger_get();

		pn_error_t *error = pn_messenger_error(messenger);

		const char *protonErrorText = pn_error_text(error);
		logger(ERROR, protonErrorText);

		return 1;
	}

	pn_data_t *body = pn_message_body(message);

	content->size = content->capacity;
	pn_data_format(body, content->data, &content->size);
	if (failed(messenger)) {
		logger_t logger = gru_logger_get();
		pn_error_t *error = pn_messenger_error(messenger);

		const char *protonErrorText = pn_error_text(error);
		logger(ERROR, protonErrorText);

		return 1;
	}

	mpt_trace("Received data (%d bytes): %s", content->size, content->data);
	return 0;
}

static gru_timestamp_t proton_timestamp_to_mpt_timestamp_t(pn_timestamp_t timestamp) {
	gru_timestamp_t ret = {0};

	double ts = ((double) timestamp / 1000);
	double integral;

	ret.tv_usec = modf(ts, &integral) * 1000000;
	ret.tv_sec = integral;

	logger_t logger = gru_logger_get();

	logger(TRACE, "Returning: %lu / %lu / %f", ret.tv_sec, ret.tv_usec, integral);

	return ret;
}

vmsl_stat_t proton_receive(msg_ctxt_t *ctxt, msg_content_data_t *content, gru_status_t *status) {
	proton_ctxt_t *proton_ctxt = proton_ctxt_cast(ctxt);

	if (proton_receive_local(proton_ctxt->messenger, status) < 0) {
		return VMSL_ERROR;
	}

	int nmsgs = 0;
	int last = 0;
	int cur = 0;
	pn_message_t *message = pn_message();
	while (nmsgs = pn_messenger_incoming(proton_ctxt->messenger)) {
		cur++;
		int ret = proton_do_receive(proton_ctxt->messenger, message, content);

		if (ret == 0) {
			pn_timestamp_t proton_ts = pn_message_get_creation_time(message);

			if (proton_ts > 0) {
				gru_timestamp_t created = proton_timestamp_to_mpt_timestamp_t(proton_ts);

				pn_timestamp_t ts = proton_now(status);

				if (likely(ts > 0)) {
					gru_timestamp_t now = proton_timestamp_to_mpt_timestamp_t(ts);

					statistics_latency(ctxt->stat_io, created, now);
					content->count++;
				}
				else {
					logger_t logger = gru_logger_get();

					logger(ERROR,
						 "Discarding message due to unable to compute current time: %s",
						 status->message);
					content->errors++;
				}
			}
			else {
				content->errors++;
			}

			if ((last + window) == cur) {
				mpt_trace("Acknowledging message %i of %i (%i / %i)", cur, nmsgs,
					 content->count,
					 content->errors);

				proton_accept(proton_ctxt->messenger);
				last = cur;
			}
			else {
				mpt_trace("Buffering message %i of %i for acknowledge (%i / %i)", cur,
					 nmsgs, content->count, content->errors);
			}
		}
	}

	if (cur > last) {
		uint64_t delta = nmsgs - last;
		content->count = content->count - delta;
			mpt_trace("Possible delta for acknowledge: %i (%i / %i)", delta,
				 nmsgs, last);
		proton_accept(proton_ctxt->messenger);
	}

	pn_message_free(message);
	return VMSL_SUCCESS;
}


bool proton_vmsl_assign(vmsl_t *vmsl) {
	logger_t logger = gru_logger_get();


	logger(INFO, "Initializing AMQP protocol");

	vmsl->init = proton_init;
	vmsl->receive = proton_receive;
	vmsl->subscribe = proton_subscribe;
	vmsl->send = proton_send;
	vmsl->stop = proton_stop;
	vmsl->destroy = proton_destroy;

	return true;
}