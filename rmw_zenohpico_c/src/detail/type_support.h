#ifndef RMW_ZENOHPICO_DETAIL__TYPE_SUPPORT_H_
#define RMW_ZENOHPICO_DETAIL__TYPE_SUPPORT_H_

#include <stdint.h>

#include "rcutils/allocator.h"
#include "rmw/ret_types.h"
#include "rosidl_typesupport_microxrcedds_c/message_type_support.h"
#include "rosidl_typesupport_microxrcedds_c/service_type_support.h"
#include "ucdr/microcdr.h"

typedef struct {
  const char *type_name;
  const message_type_support_callbacks_t *callbacks;
} rmw_zp_type_support_t;

rmw_ret_t rmw_zp_type_support_init(rmw_zp_type_support_t *type_support,
                                   const rosidl_message_type_support_t *message_type_support,
                                   rcutils_allocator_t *allocator);

rmw_ret_t rmw_zp_type_support_fini(rmw_zp_type_support_t *type_support,
                                   rcutils_allocator_t *allocator);

size_t rmw_zp_type_support_get_serialized_size(rmw_zp_type_support_t *type_support,
                                               const void *ros_message);

rmw_ret_t rmw_zp_type_support_serialize_ros_message(rmw_zp_type_support_t *type_support,
                                                    const void *ros_message, uint8_t *buf,
                                                    size_t buf_size);

rmw_ret_t rmw_zp_type_support_deserialize_ros_message(rmw_zp_type_support_t *type_support,
                                                      const uint8_t *buf, size_t buf_size,
                                                      void *ros_message);

rmw_ret_t rmw_zp_find_message_type_support(
    const rosidl_message_type_support_t *type_supports,
    rosidl_message_type_support_t const **message_type_support);

rmw_ret_t rmw_zp_find_service_type_support(
    const rosidl_service_type_support_t *type_supports,
    rosidl_service_type_support_t const **service_type_support);

#endif