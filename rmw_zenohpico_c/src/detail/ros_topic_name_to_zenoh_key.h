#ifndef RMW_ZENOHPICO_DETAIL__ROS_TOPIC_NAME_TO_ZENOH_KEY_H_
#define RMW_ZENOHPICO_DETAIL__ROS_TOPIC_NAME_TO_ZENOH_KEY_H_

#include "rcutils/allocator.h"

//==============================================================================
// A function that generates a key expression for message transport of the
// format <ros_domain_id>/<topic_name>/<topic_type>/<topic_hash> In particular,
// Zenoh keys cannot start or end with a /, so this function will strip them out.
const char *ros_topic_name_to_zenoh_key(const size_t domain_id, const char *const topic_name,
                                        const char *const topic_type, const char *const type_hash,
                                        rcutils_allocator_t *allocator);

#endif