#include "detail/subscription.h"
#include "rmw/rmw.h"

rmw_ret_t rmw_take(const rmw_subscription_t* subscription, void* ros_message, bool* taken,
                   rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_with_info(const rmw_subscription_t* subscription, void* ros_message, bool* taken,
                             rmw_message_info_t* message_info,
                             rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(message_info);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_sequence(const rmw_subscription_t* subscription, size_t count,
                            rmw_message_sequence_t* message_sequence,
                            rmw_message_info_sequence_t* message_info_sequence, size_t* taken,
                            rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(count);
  RCUTILS_UNUSED(message_sequence);
  RCUTILS_UNUSED(message_info_sequence);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_serialized_message(const rmw_subscription_t* subscription,
                                      rmw_serialized_message_t* serialized_message, bool* taken,
                                      rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(serialized_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_serialized_message_with_info(const rmw_subscription_t* subscription,
                                                rmw_serialized_message_t* serialized_message,
                                                bool* taken, rmw_message_info_t* message_info,
                                                rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(serialized_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(message_info);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_loaned_message(const rmw_subscription_t* subscription, void** loaned_message,
                                  bool* taken, rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_loaned_message_with_info(const rmw_subscription_t* subscription,
                                            void** loaned_message, bool* taken,
                                            rmw_message_info_t* message_info,
                                            rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(message_info);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_return_loaned_message_from_subscription(const rmw_subscription_t* subscription,
                                                      void* loaned_message) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);
  return RMW_RET_UNSUPPORTED;
}
