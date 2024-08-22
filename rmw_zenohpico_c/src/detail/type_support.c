#include "./type_support.h"

#include "./identifiers.h"
#include "rcutils/snprintf.h"
#include "rmw/error_handling.h"
#include "zenoh-pico.h"

#define CDR_ENCAPSULATION_SIZE 4

static const char *const type_format_str = "%s::dds_::%s_";
const uint8_t cdr_encapsulation[] = {0, 1, 0, 0};

static const char *create_type_name(const message_type_support_callbacks_t *members,
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

rmw_ret_t rmw_zenohpico_type_support_init(rmw_zenohpico_type_support_t *type_support,
                                          const rosidl_message_type_support_t *message_type_support,
                                          rcutils_allocator_t *allocator) {
  // TODO: Check type support identifier
  type_support->callbacks = message_type_support->data;

  type_support->type_name = create_type_name(type_support->callbacks, allocator);
  if (type_support->type_name == NULL) {
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zenohpico_type_support_fini(rmw_zenohpico_type_support_t *type_support,
                                          rcutils_allocator_t *allocator) {
  if (type_support->type_name != NULL) {
    allocator->deallocate((char *)type_support->type_name, allocator->state);
  }

  return RMW_RET_OK;
}

size_t rmw_zenohpico_type_support_get_serialized_size(rmw_zenohpico_type_support_t *type_support,
                                                      const void *ros_message) {
  return CDR_ENCAPSULATION_SIZE + type_support->callbacks->get_serialized_size(ros_message);
}

rmw_ret_t rmw_zenohpico_type_support_serialize_ros_message(
    rmw_zenohpico_type_support_t *type_support, const void *ros_message, uint8_t *buf,
    size_t buf_size) {
  ucdrBuffer ub;

  memcpy(buf, cdr_encapsulation, CDR_ENCAPSULATION_SIZE);

  ucdr_init_buffer(&ub, &buf[CDR_ENCAPSULATION_SIZE], buf_size - CDR_ENCAPSULATION_SIZE);
  if (!type_support->callbacks->cdr_serialize(ros_message, &ub)) {
    RMW_SET_ERROR_MSG("failed to create type name string");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zenohpico_type_support_deserialize_ros_message(
    rmw_zenohpico_type_support_t *type_support, const uint8_t *buf, void *ros_message) {
  return RMW_RET_ERROR;
}

const rosidl_message_type_support_t *find_message_type_support(
    const rosidl_message_type_support_t *type_supports) {
  const rosidl_message_type_support_t *type_support =
      get_message_typesupport_handle(type_supports, RMW_ZENOHPICO_C_TYPESUPPORT_C);
  if (!type_support) {
    rcutils_error_string_t error_string = rcutils_get_error_string();
    rcutils_reset_error();
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Type support not from this implementation. Got:\n"
        "    %s\n"
        "while fetching it",
        error_string.str);
    return NULL;
  }

  return type_support;
}

const rosidl_service_type_support_t *find_service_type_support(
    const rosidl_service_type_support_t *type_supports) {
  const rosidl_service_type_support_t *type_support =
      get_service_typesupport_handle(type_supports, RMW_ZENOHPICO_C_TYPESUPPORT_C);
  if (!type_support) {
    rcutils_error_string_t error_string = rcutils_get_error_string();
    rcutils_reset_error();
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Type support not from this implementation. Got:\n"
        "    %s\n"
        "while fetching it",
        error_string.str);
    return NULL;
  }

  return type_support;
}
