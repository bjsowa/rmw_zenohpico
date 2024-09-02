#include "./service.h"

#include "./qos.h"

rmw_ret_t rmw_zp_service_init(rmw_zp_service_t* service, const rmw_qos_profile_t* qos_profile,
                              rcutils_allocator_t* allocator) {
  service->adapted_qos_profile = *qos_profile;

  if (rmw_zp_adapt_qos_profile(&service->adapted_qos_profile) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_init(&service->query_queue, service->adapted_qos_profile.depth,
                                allocator) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (z_mutex_init(&service->query_queue_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    goto fail_init_query_queue_mutex;
  }

  return RMW_RET_OK;

fail_init_query_queue_mutex:
  rmw_zp_message_queue_fini(&service->query_queue, allocator);
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_zp_service_fini(rmw_zp_service_t* service, rcutils_allocator_t* allocator) {
  rmw_ret_t ret = RMW_RET_OK;

  if (z_drop(z_move(service->query_queue_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    ret = RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_fini(&service->query_queue, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  return ret;
}
