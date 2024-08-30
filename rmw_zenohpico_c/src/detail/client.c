#include "./client.h"

#include "./qos.h"
#include "rmw/error_handling.h"

rmw_ret_t rmw_zp_client_init(rmw_zp_client_t* client, const rmw_qos_profile_t* qos_profile) {
  client->sequence_number = 1;
  client->adapted_qos_profile = *qos_profile;

  if (rmw_zp_adapt_qos_profile(&client->adapted_qos_profile) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (z_mutex_init(&client->sequence_number_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_client_fini(rmw_zp_client_t* client) {
  if (z_drop(z_move(client->sequence_number_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}