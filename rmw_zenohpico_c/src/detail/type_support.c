#include "./type_support.h"

#include "./identifiers.h"
#include "rcutils/snprintf.h"
#include "rmw/error_handling.h"
#include "rmw/macros.h"
#include "zenoh-pico.h"

#define CDR_HEADER_SIZE 4

static const char *const type_format_str = "%s::dds_::%s_";
// const uint8_t cdr_header[] = {0, 1, 0, 0};

static const char *create_message_type_name(const message_type_support_callbacks_t *members,
                                            rcutils_allocator_t *allocator) {
  if (!members) {
    RMW_SET_ERROR_MSG("members handle is null");
    return NULL;
  }

  // Get the number of bytes to allocate
  int type_name_size = rcutils_snprintf(NULL, 0, type_format_str, members->message_namespace_,
                                        members->message_name_);

  char *type_name = (char *)allocator->allocate(type_name_size + 1, allocator->state);
  if (type_name == NULL) {
    RMW_SET_ERROR_MSG("failed to allocate memory for type name");
    return NULL;
  }

  int ret = rcutils_snprintf(type_name, type_name_size + 1, type_format_str,
                             members->message_namespace_, members->message_name_);

  if (ret < 0 || ret > type_name_size) {
    RMW_SET_ERROR_MSG("failed to create type name string");
    allocator->deallocate(type_name, allocator->state);
    return NULL;
  }

  return type_name;
}

static const char *create_service_type_name(const service_type_support_callbacks_t *members,
                                            rcutils_allocator_t *allocator) {
  if (!members) {
    RMW_SET_ERROR_MSG("members handle is null");
    return NULL;
  }

  // Get the number of bytes to allocate
  int type_name_size =
      rcutils_snprintf(NULL, 0, type_format_str, members->package_name_, members->service_name_);

  char *type_name = (char *)allocator->allocate(type_name_size + 1, allocator->state);
  if (type_name == NULL) {
    RMW_SET_ERROR_MSG("failed to allocate memory for type name");
    return NULL;
  }

  int ret = rcutils_snprintf(type_name, type_name_size + 1, type_format_str, members->package_name_,
                             members->service_name_);

  if (ret < 0 || ret > type_name_size) {
    RMW_SET_ERROR_MSG("failed to create type name string");
    allocator->deallocate(type_name, allocator->state);
    return NULL;
  }

  return type_name;
}

rmw_ret_t rmw_zp_find_message_type_support(
    const rosidl_message_type_support_t *type_supports,
    rosidl_message_type_support_t const **message_type_support) {
  *message_type_support =
      get_message_typesupport_handle(type_supports, RMW_ZENOHPICO_C_TYPESUPPORT_C);
  if (!message_type_support) {
    rcutils_error_string_t error_string = rcutils_get_error_string();
    rcutils_reset_error();
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Type support not from this implementation. Got:\n"
        "    %s\n"
        "while fetching it",
        error_string.str);
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_find_service_type_support(
    const rosidl_service_type_support_t *type_supports,
    rosidl_service_type_support_t const **service_type_support) {
  *service_type_support =
      get_service_typesupport_handle(type_supports, RMW_ZENOHPICO_C_TYPESUPPORT_C);
  if (!service_type_support) {
    rcutils_error_string_t error_string = rcutils_get_error_string();
    rcutils_reset_error();
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Type support not from this implementation. Got:\n"
        "    %s\n"
        "while fetching it",
        error_string.str);
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_message_type_support_init(
    rmw_zp_message_type_support_t *type_support,
    const rosidl_message_type_support_t *message_type_supports, rcutils_allocator_t *allocator) {
  const rosidl_message_type_support_t *message_type_support;
  if (rmw_zp_find_message_type_support(message_type_supports, &message_type_support) !=
      RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  type_support->callbacks = message_type_support->data;

  type_support->type_name = create_message_type_name(type_support->callbacks, allocator);
  if (type_support->type_name == NULL) {
    return RMW_RET_ERROR;
  }

  type_support->type_hash = message_type_support->get_type_hash_func(message_type_support);

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_message_type_support_fini(rmw_zp_message_type_support_t *type_support,
                                           rcutils_allocator_t *allocator) {
  if (type_support->type_name != NULL) {
    allocator->deallocate((char *)type_support->type_name, allocator->state);
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_service_type_support_init(
    rmw_zp_service_type_support_t *type_support,
    const rosidl_service_type_support_t *service_type_supports, rcutils_allocator_t *allocator) {
  const rosidl_service_type_support_t *service_type_support;
  if (rmw_zp_find_service_type_support(service_type_supports, &service_type_support) !=
      RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  const service_type_support_callbacks_t *service_members = service_type_support->data;

  type_support->request_callbacks = service_members->request_members_()->data;
  type_support->response_callbacks = service_members->response_members_()->data;

  type_support->type_name = create_service_type_name(service_members, allocator);
  if (type_support->type_name == NULL) {
    return RMW_RET_ERROR;
  }

  type_support->type_hash = service_type_support->get_type_hash_func(service_type_support);

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_service_type_support_fini(rmw_zp_service_type_support_t *type_support,
                                           rcutils_allocator_t *allocator) {
  if (type_support->type_name != NULL) {
    allocator->deallocate((char *)type_support->type_name, allocator->state);
  }

  return RMW_RET_OK;
}

size_t rmw_zp_type_support_get_serialized_size(rmw_zp_message_type_support_t *type_support,
                                               const void *ros_message) {
  return CDR_HEADER_SIZE + type_support->callbacks->get_serialized_size(ros_message);
}

rmw_ret_t rmw_zp_message_type_support_serialize(rmw_zp_message_type_support_t *type_support,
                                                const void *ros_message, uint8_t *buf,
                                                size_t buf_size) {
  if (buf_size < CDR_HEADER_SIZE) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Cannot serialize the message into buffer of size: %zu. Must have space for a 4-byte "
        "header",
        buf_size);
    return RMW_RET_ERROR;
  }

  memset(buf, 0, CDR_HEADER_SIZE);
  if (UCDR_MACHINE_ENDIANNESS == UCDR_LITTLE_ENDIANNESS) {
    buf[1] |= 0x1;
  }
  // TODO(bjsowa): What do the rest of the bits in the CDR header mean? Do we care?

  ucdrBuffer ub;
  ucdr_init_buffer_origin_offset(&ub, buf, buf_size, CDR_HEADER_SIZE, CDR_HEADER_SIZE);
  if (!type_support->callbacks->cdr_serialize(ros_message, &ub)) {
    RMW_SET_ERROR_MSG("Type support failed to serialize the message");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_message_type_support_deserialize(rmw_zp_message_type_support_t *type_support,
                                                  const uint8_t *buf, size_t buf_size,
                                                  void *ros_message) {
  if (buf_size < CDR_HEADER_SIZE) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Cannot deserialize buffer of size: %zu. Must contain at least a 4-byte header", buf_size);
    return RMW_RET_ERROR;
  }

  ucdrEndianness endianness;
  if (buf[1] & 0x1) {
    endianness = UCDR_LITTLE_ENDIANNESS;
  } else {
    endianness = UCDR_BIG_ENDIANNESS;
  }
  // TODO(bjsowa): What do the rest of the bits in the CDR header mean? Do we care?

  ucdrBuffer ub;
  ucdr_init_buffer_origin_offset_endian(&ub, (uint8_t *)buf, buf_size, CDR_HEADER_SIZE,
                                        CDR_HEADER_SIZE, endianness);
  if (!type_support->callbacks->cdr_deserialize(&ub, ros_message)) {
    RMW_SET_ERROR_MSG("Type support failed to deserialize the message");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}
