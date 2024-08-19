#ifndef RMW_ZENOHPICO_DETAIL__ROS_TOPIC_NAME_TO_ZENOH_KEY_H_
#define RMW_ZENOHPICO_DETAIL__ROS_TOPIC_NAME_TO_ZENOH_KEY_H_

#include "rcutils/allocator.h"

//==============================================================================
// A function that generates a key expression for message transport of the
// format <ros_domain_id>/<topic_name>/<topic_type>/<topic_hash>.
// We assume topic name is a valid fully qualified topic name.
char *ros_topic_name_to_zenoh_key(const size_t domain_id, const char *const topic_name,
                                  const char *const topic_type, const char *const type_hash,
                                  rcutils_allocator_t *allocator);

#endif