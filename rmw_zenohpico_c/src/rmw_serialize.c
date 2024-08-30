#include "detail/type_support.h"
#include "rmw/rmw.h"

rmw_ret_t rmw_get_serialized_message_size(const rosidl_message_type_support_t *type_support,
                                          const rosidl_runtime_c__Sequence__bound *message_bounds,
                                          size_t *size) {
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(message_bounds);
  RCUTILS_UNUSED(size);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_serialize(const void *ros_message, const rosidl_message_type_support_t *type_supports,
                        rmw_serialized_message_t *serialized_message) {
  const rosidl_message_type_support_t *message_type_support;
  if (rmw_zp_find_message_type_support(type_supports, &message_type_support) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  rmw_zp_message_type_support_t type_support;
  type_support.callbacks = message_type_support->data;

  size_t serialized_size = rmw_zp_type_support_get_serialized_size(&type_support, ros_message);

  if (serialized_message->buffer_capacity < serialized_size) {
    if (rmw_serialized_message_resize(serialized_message, serialized_size) != RMW_RET_OK) {
      return RMW_RET_ERROR;
    }
  }

  serialized_message->buffer_length = serialized_size;
  serialized_message->buffer_capacity = serialized_size;

  if (rmw_zp_message_type_support_serialize(&type_support, ros_message, serialized_message->buffer,
                                            serialized_size) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_deserialize(const rmw_serialized_message_t *serialized_message,
                          const rosidl_message_type_support_t *type_supports, void *ros_message) {
  const rosidl_message_type_support_t *message_type_support;
  if (rmw_zp_find_message_type_support(type_supports, &message_type_support) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  rmw_zp_message_type_support_t type_support;
  type_support.callbacks = message_type_support->data;

  if (rmw_zp_message_type_support_deserialize(&type_support, serialized_message->buffer,
                                              serialized_message->buffer_length,
                                              ros_message) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}