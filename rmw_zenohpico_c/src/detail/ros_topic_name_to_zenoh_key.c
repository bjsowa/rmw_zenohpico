#include "./ros_topic_name_to_zenoh_key.h"

#include "rmw/error_handling.h"

static const char *const zenoh_key_format_str = "%zu/%s/%s/%s";

char *ros_topic_name_to_zenoh_key(const size_t domain_id, const char *const topic_name,
                                  const char *const topic_type, const char *const type_hash,
                                  rcutils_allocator_t *allocator) {
  // Get the number of bytes to allocate
  int zenoh_key_size = rcutils_snprintf(NULL, 0, zenoh_key_format_str, domain_id, &topic_name[1],
                                        topic_type, type_hash);

  char *zenoh_key = (char *)allocator->allocate(zenoh_key_size + 1, allocator->state);
  if (zenoh_key == NULL) {
    RMW_SET_ERROR_MSG("failed to allocate memory for zenoh key");
    return NULL;
  }

  int ret = rcutils_snprintf(zenoh_key, zenoh_key_size + 1, zenoh_key_format_str, domain_id,
                             &topic_name[1], topic_type, type_hash);

  if (ret < 0 || ret > zenoh_key_size) {
    RMW_SET_ERROR_MSG("failed to create type name string");
    allocator->deallocate(zenoh_key, allocator->state);
    return NULL;
  }

  return zenoh_key;
}
