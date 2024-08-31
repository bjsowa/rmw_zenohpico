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
  const rosidl_type_hash_t *type_hash;
  const message_type_support_callbacks_t *callbacks;
} rmw_zp_message_type_support_t;

typedef struct {
  const char *type_name;
  const rosidl_type_hash_t *type_hash;
  const message_type_support_callbacks_t *request_callbacks;
  const message_type_support_callbacks_t *response_callbacks;
} rmw_zp_service_type_support_t;

rmw_ret_t rmw_zp_find_message_type_support(
    const rosidl_message_type_support_t *type_supports,
    rosidl_message_type_support_t const **message_type_support);

rmw_ret_t rmw_zp_find_service_type_support(
    const rosidl_service_type_support_t *type_supports,
    rosidl_service_type_support_t const **service_type_support);

// Init
rmw_ret_t rmw_zp_message_type_support_init(
    rmw_zp_message_type_support_t *type_support,
    const rosidl_message_type_support_t *message_type_supports, rcutils_allocator_t *allocator);

rmw_ret_t rmw_zp_service_type_support_init(
    rmw_zp_service_type_support_t *type_support,
    const rosidl_service_type_support_t *service_type_supports, rcutils_allocator_t *allocator);

// Fini
rmw_ret_t rmw_zp_message_type_support_fini(rmw_zp_message_type_support_t *type_support,
                                           rcutils_allocator_t *allocator);

rmw_ret_t rmw_zp_service_type_support_fini(rmw_zp_service_type_support_t *type_support,
                                           rcutils_allocator_t *allocator);

// Get serialized size
size_t rmw_zp_message_type_support_get_serialized_size(rmw_zp_message_type_support_t *type_support,
                                                       const void *ros_message);

size_t rmw_zp_service_type_support_get_request_serialized_size(
    rmw_zp_service_type_support_t *type_support, const void *ros_request);

size_t rmw_zp_service_type_support_get_response_serialized_size(
    rmw_zp_service_type_support_t *type_support, const void *ros_response);

// Serialize
rmw_ret_t rmw_zp_message_type_support_serialize(rmw_zp_message_type_support_t *type_support,
                                                const void *ros_message, uint8_t *buf,
                                                size_t buf_size);

rmw_ret_t rmw_zp_service_type_support_serialize_request(rmw_zp_service_type_support_t *type_support,
                                                        const void *ros_request, uint8_t *buf,
                                                        size_t buf_size);

rmw_ret_t rmw_zp_service_type_support_serialize_response(
    rmw_zp_service_type_support_t *type_support, const void *ros_response, uint8_t *buf,
    size_t buf_size);

// Deserialize
rmw_ret_t rmw_zp_message_type_support_deserialize(rmw_zp_message_type_support_t *type_support,
                                                  const uint8_t *buf, size_t buf_size,
                                                  void *ros_message);

rmw_ret_t rmw_zp_service_type_support_deserialize_request(
    rmw_zp_service_type_support_t *type_support, const uint8_t *buf, size_t buf_size,
    void *ros_request);

rmw_ret_t rmw_zp_service_type_support_deserialize_response(
    rmw_zp_service_type_support_t *type_support, const uint8_t *buf, size_t buf_size,
    void *ros_response);

#endif