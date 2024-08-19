#include "rmw/rmw.h"

rmw_ret_t rmw_publish(const rmw_publisher_t *publisher, const void *ros_message,
                      rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_publish_loaned_message(const rmw_publisher_t *publisher, void *ros_message,
                                     rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publish_serialized_message(const rmw_publisher_t *publisher,
                                         const rmw_serialized_message_t *serialized_message,
                                         rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(serialized_message);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_ERROR;
}