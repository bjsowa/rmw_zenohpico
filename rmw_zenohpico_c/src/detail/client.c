#include "./client.h"

#include "./qos.h"
#include "rmw/error_handling.h"

rmw_ret_t rmw_zp_client_init(rmw_zp_client_t* client, const rmw_qos_profile_t* qos_profile,
                             rcutils_allocator_t* allocator) {
  client->sequence_number = 1;
  client->adapted_qos_profile = *qos_profile;
  client->is_shutdown = false;
  client->num_in_flight = 0;

  if (rmw_zp_adapt_qos_profile(&client->adapted_qos_profile) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (z_mutex_init(&client->sequence_number_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    return RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_init(&client->reply_queue, client->adapted_qos_profile.depth,
                                allocator) != RMW_RET_OK) {
    goto fail_init_reply_queue;
  }

  if (z_mutex_init(&client->reply_queue_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    goto fail_init_reply_queue_mutex;
  }

  if (z_mutex_init(&client->in_flight_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    goto fail_init_in_flight_mutex;
  }

  return RMW_RET_OK;

fail_init_in_flight_mutex:
  z_drop(z_move(client->reply_queue_mutex));
fail_init_reply_queue_mutex:
  rmw_zp_message_queue_fini(&client->reply_queue, allocator);
fail_init_reply_queue:
  z_drop(z_move(client->sequence_number_mutex));
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_zp_client_fini(rmw_zp_client_t* client, rcutils_allocator_t* allocator) {
  rmw_ret_t ret = RMW_RET_OK;

  if (z_drop(z_move(client->in_flight_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    ret = RMW_RET_ERROR;
  }

  if (z_drop(z_move(client->reply_queue_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    ret = RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_fini(&client->reply_queue, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  if (z_drop(z_move(client->sequence_number_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    ret = RMW_RET_ERROR;
  }

  return ret;
}

size_t rmw_zp_client_get_next_sequence_number(rmw_zp_client_t* client) {
  z_mutex_lock(z_loan_mut(client->sequence_number_mutex));
  size_t seq = client->sequence_number++;
  z_mutex_unlock(z_loan_mut(client->sequence_number_mutex));
  return seq;
}

void rmw_zp_client_increment_queries_in_flight(rmw_zp_client_t* client) {
  z_mutex_lock(z_loan_mut(client->in_flight_mutex));
  client->num_in_flight++;
  z_mutex_unlock(z_loan_mut(client->in_flight_mutex));
}

void rmw_zp_client_decrement_queries_in_flight(rmw_zp_client_t* client, bool* queries_in_flight) {
  z_mutex_lock(z_loan_mut(client->in_flight_mutex));
  *queries_in_flight = --client->num_in_flight > 0;
  z_mutex_unlock(z_loan_mut(client->in_flight_mutex));
}

void rmw_zp_client_data_handler(const z_loaned_reply_t* reply, void* data) {
  rmw_zp_client_t* client_data = data;
  if (client_data == NULL) {
    // TODO: report error
    return;
  }

  // See the comment about the "num_in_flight" class variable in the
  // rmw_client_data_t class for why we need to do this.
  if (client_data->is_shutdown) {
    return;
  }

  if (!z_reply_is_ok(reply)) {
    // TODO(bjsowa): Report error
    return;
  }

  const z_loaned_sample_t* sample = z_reply_ok(reply);

  const z_loaned_bytes_t* attachment = z_sample_attachment(sample);
  if (!_z_bytes_check(attachment)) {
    // TODO(bjsowa): report error
    return;
  }

  const z_loaned_bytes_t* payload = z_sample_payload(sample);

  if (rmw_zp_client_add_new_reply(client_data, attachment, payload) != RMW_RET_OK) {
    // TODO(bjsowa): report error
  }
}

void rmw_zp_client_data_dropper(void* data) {
  rmw_zp_client_t* client_data = data;
  if (client_data == NULL) {
    // TODO: report error
    return;
  }

  // See the comment about the "num_in_flight" class variable in the
  // rmw_client_data_t class for why we need to do this.
  bool queries_in_flight = false;
  rmw_zp_client_decrement_queries_in_flight(client_data, &queries_in_flight);

  if (client_data->is_shutdown) {
    if (!queries_in_flight) {
      rmw_zp_client_fini(client_data, &client_data->context->options.allocator);
      client_data->context->options.allocator.deallocate(
          client_data, client_data->context->options.allocator.state);
    }
  }
}

rmw_ret_t rmw_zp_client_add_new_reply(rmw_zp_client_t* client, const z_loaned_bytes_t* attachment,
                                      const z_loaned_bytes_t* payload) {
  z_mutex_lock(z_loan_mut(client->reply_queue_mutex));

  if (client->reply_queue.size >= client->adapted_qos_profile.depth) {
    // TODO: Log warning if reply is discarded due to hitting the queue depth

    // Adapted QoS has depth guaranteed to be >= 1
    if (rmw_zp_message_queue_pop_front(&client->reply_queue, NULL) != RMW_RET_OK) {
      z_mutex_unlock(z_loan_mut(client->reply_queue_mutex));
      return RMW_RET_ERROR;
    }
  }

  if (rmw_zp_message_queue_push_back(&client->reply_queue, attachment, payload, NULL) !=
      RMW_RET_OK) {
    z_mutex_unlock(z_loan_mut(client->reply_queue_mutex));
    return RMW_RET_ERROR;
  }

  z_mutex_unlock(z_loan_mut(client->reply_queue_mutex));

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_client_pop_next_reply(rmw_zp_client_t* client, rmw_zp_message_t* reply_data) {
  z_mutex_lock(z_loan_mut(client->reply_queue_mutex));

  if (client->reply_queue.size == 0) {
    RMW_SET_ERROR_MSG("Trying to pop message from empty message queue");
    z_mutex_unlock(z_loan_mut(client->reply_queue_mutex));
    return RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_pop_front(&client->reply_queue, reply_data) != RMW_RET_OK) {
    z_mutex_unlock(z_loan_mut(client->reply_queue_mutex));
    return RMW_RET_ERROR;
  }

  z_mutex_unlock(z_loan_mut(client->reply_queue_mutex));

  return RMW_RET_OK;
}