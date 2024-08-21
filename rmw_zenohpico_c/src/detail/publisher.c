#include "./publisher.h"

rmw_ret_t rmw_zenohpico_publisher_init(rmw_zenohpico_publisher_t* publisher) {
  publisher->sequence_number = 1;

  if (z_mutex_init(&publisher->sequence_number_mutex) < 0) {
    RCUTILS_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zenohpico_publisher_fini(rmw_zenohpico_publisher_t* publisher) {
  if (z_drop(z_move(publisher->sequence_number_mutex)) < 0) {
    RCUTILS_SET_ERROR_MSG("Failed to drop zenohpico mutex");
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
