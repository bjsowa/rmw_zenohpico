#include "./subscription.h"

rmw_ret_t rmw_zenohpico_subscription_init(rmw_zenohpico_subscription_t* subscription) {
  return RMW_RET_OK;
}

rmw_ret_t rmw_zenohpico_subscription_fini(rmw_zenohpico_subscription_t* subscription) {
  return RMW_RET_OK;
}

void rmw_zenohpico_sub_data_handler(const z_loaned_sample_t * sample, void * sub_data) {
  
}