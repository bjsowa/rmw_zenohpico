#include "./type_support.h"

#include "rcutils/snprintf.h"
#include "rmw/error_handling.h"

static const char *const type_format_str = "%s::dds_::%s_";

static const char *create_type_name(const message_type_support_callbacks_t *members,
                                    rcutils_allocator_t *allocator) {
  if (!members) {
    RMW_SET_ERROR_MSG("members handle is null");
    return NULL;
  }

  // Get the number of bytes to allocate
  size_t type_name_size = rcutils_snprintf(NULL, 0, type_format_str, members->message_namespace_,
                                           members->message_name_);

  char *type_name = (char *)allocator->allocate(type_name_size + 1, allocator->state);
  if (type_name == NULL) {
    RMW_SET_ERROR_MSG("failed to allocate memory for type name");
    return NULL;
  }

  rcutils_snprintf(type_name, type_name_size + 1, type_format_str, members->message_namespace_,
                   members->message_name_);

  return type_name;
}

rmw_ret_t rmw_zenohpico_type_support_init(rmw_zenohpico_type_support_t *type_support,
                                          rosidl_message_type_support_t *message_type_support,
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
    allocator->deallocate(type_support->type_name, allocator->state);
  }

  return RMW_RET_OK;
}
