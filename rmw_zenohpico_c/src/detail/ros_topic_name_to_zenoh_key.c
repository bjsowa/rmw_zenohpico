#include "./ros_topic_name_to_zenoh_key.h"

const char *ros_topic_name_to_zenoh_key(const size_t domain_id, const char *const topic_name,
                                        const char *const topic_type, const char *const type_hash,
                                        rcutils_allocator_t *allocator) {
  // keyexpr_str = std::to_string(domain_id);
  // keyexpr_str += "/";
  // keyexpr_str += strip_slashes(topic_name);
  // keyexpr_str += "/";
  // keyexpr_str += topic_type;
  // keyexpr_str += "/";
  // keyexpr_str += type_hash;

  // TODO(yuyuan): use z_view_keyexpr_t?
//   z_owned_keyexpr_t keyexpr;
  // z_keyexpr_from_str(&keyexpr, keyexpr_str.c_str());
//   return keyexpr;
}
