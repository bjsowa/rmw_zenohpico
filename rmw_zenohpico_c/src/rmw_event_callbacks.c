#include "rmw/rmw.h"

rmw_ret_t rmw_subscription_set_on_new_message_callback(rmw_subscription_t* subscription,
                                                       rmw_event_callback_t callback,
                                                       const void* user_data) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(callback);
  RCUTILS_UNUSED(user_data);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_service_set_on_new_request_callback(rmw_service_t* service,
                                                  rmw_event_callback_t callback,
                                                  const void* user_data) {
  RCUTILS_UNUSED(service);
  RCUTILS_UNUSED(callback);
  RCUTILS_UNUSED(user_data);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_client_set_on_new_response_callback(rmw_client_t* client,
                                                  rmw_event_callback_t callback,
                                                  const void* user_data) {
  RCUTILS_UNUSED(client);
  RCUTILS_UNUSED(callback);
  RCUTILS_UNUSED(user_data);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_event_set_callback(rmw_event_t* event, rmw_event_callback_t callback,
                                 const void* user_data) {
  RCUTILS_UNUSED(event);
  RCUTILS_UNUSED(callback);
  RCUTILS_UNUSED(user_data);
  return RMW_RET_UNSUPPORTED;
}