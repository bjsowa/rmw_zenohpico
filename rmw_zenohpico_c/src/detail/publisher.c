#include "./publisher.h"

#include "./qos.h"
#include "rmw/error_handling.h"

rmw_ret_t rmw_zenohpico_publisher_init(rmw_zenohpico_publisher_t* publisher,
                                       const rmw_qos_profile_t* qos_profile) {
  publisher->sequence_number = 1;
  publisher->adapted_qos_profile = *qos_profile;

  if (rmw_zenohpico_adapt_qos_profile(&publisher->adapted_qos_profile) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (z_mutex_init(&publisher->sequence_number_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zenohpico_publisher_fini(rmw_zenohpico_publisher_t* publisher) {
  if (z_drop(z_move(publisher->sequence_number_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

size_t rmw_zenohpico_publisher_get_next_sequence_number(rmw_zenohpico_publisher_t* publisher) {
  z_mutex_lock(z_loan_mut(publisher->sequence_number_mutex));
  size_t seq = publisher->sequence_number++;
  z_mutex_unlock(z_loan_mut(publisher->sequence_number_mutex));
  return seq;
}
