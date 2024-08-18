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