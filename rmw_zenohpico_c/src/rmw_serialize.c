#include "rmw/rmw.h"

rmw_ret_t rmw_get_serialized_message_size(const rosidl_message_type_support_t *type_support,
                                          const rosidl_runtime_c__Sequence__bound *message_bounds,
                                          size_t *size) {
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(message_bounds);
  RCUTILS_UNUSED(size);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_serialize(const void *ros_message, const rosidl_message_type_support_t *type_support,
                        rmw_serialized_message_t *serialized_message) {
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(serialized_message);
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_deserialize(const rmw_serialized_message_t *serialized_message,
                          const rosidl_message_type_support_t *type_support, void *ros_message) {
  RCUTILS_UNUSED(serialized_message);
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(ros_message);
  return RMW_RET_ERROR;
}