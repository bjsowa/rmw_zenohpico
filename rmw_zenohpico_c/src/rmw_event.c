#include "rcutils/macros.h"
#include "rmw/event.h"
#include "rmw/rmw.h"

rmw_ret_t rmw_publisher_event_init(rmw_event_t* rmw_event, const rmw_publisher_t* publisher,
                                   rmw_event_type_t event_type) {
  RCUTILS_UNUSED(rmw_event);
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(event_type);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_event_init(rmw_event_t* rmw_event,
                                      const rmw_subscription_t* subscription,
                                      rmw_event_type_t event_type) {
  RCUTILS_UNUSED(rmw_event);
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(event_type);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_event_set_callback(rmw_event_t* event, rmw_event_callback_t callback,
                                 const void* user_data) {
  RCUTILS_UNUSED(event);
  RCUTILS_UNUSED(callback);
  RCUTILS_UNUSED(user_data);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_event(const rmw_event_t* event_handle, void* event_info, bool* taken) {
  RCUTILS_UNUSED(event_handle);
  RCUTILS_UNUSED(event_info);
  RCUTILS_UNUSED(taken);
  return RMW_RET_UNSUPPORTED;
}