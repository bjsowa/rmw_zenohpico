#include "./qos.h"

#include "rmw/error_handling.h"

// Define defaults for various QoS settings.
#define RMW_ZENOHPICO_DEFAULT_HISTORY RMW_QOS_POLICY_HISTORY_KEEP_LAST;
// If the depth field in the qos profile is set to 0, the RMW implementation
// has the liberty to assign a default depth. The zenoh transport protocol
// is configured with 256 channels so theoretically, this would be the maximum
// depth we can set before blocking transport. A high depth would increase the
// memory footprint of processes as more messages are stored in memory while a
// very low depth might unintentionally drop messages leading to a poor
// out-of-the-box experience for new users. For now we set the depth to 42,
// a popular "magic number". See https://en.wikipedia.org/wiki/42_(number).
#define RMW_ZENOHPICO_DEFAULT_HISTORY_DEPTH             42;
#define RMW_ZENOHPICO_DEFAULT_RELIABILITY               RMW_QOS_POLICY_RELIABILITY_RELIABLE;
#define RMW_ZENOHPICO_DEFAULT_DURABILITY                RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
#define RMW_ZENOHPICO_DEFAULT_DEADLINE                  RMW_DURATION_INFINITE;
#define RMW_ZENOHPICO_DEFAULT_LIFESPAN                  RMW_DURATION_INFINITE;
#define RMW_ZENOHPICO_DEFAULT_LIVELINESS                RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
#define RMW_ZENOHPICO_DEFAULT_LIVELINESS_LEASE_DURATION RMW_DURATION_INFINITE;

rmw_ret_t rmw_zp_adapt_qos_profile(rmw_qos_profile_t *qos_profile) {
  switch (qos_profile->history) {
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
    case RMW_QOS_POLICY_HISTORY_UNKNOWN:
      qos_profile->history = RMW_ZENOHPICO_DEFAULT_HISTORY;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      RMW_SET_ERROR_MSG("rmw_zenohpico does not support QoS history policy: Keep all");
      return RMW_RET_ERROR;
    default:
      break;
  }

  if (qos_profile->depth == 0) {
    qos_profile->depth = RMW_ZENOHPICO_DEFAULT_HISTORY_DEPTH;
  }

  switch (qos_profile->reliability) {
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
    case RMW_QOS_POLICY_RELIABILITY_UNKNOWN:
      qos_profile->reliability = RMW_ZENOHPICO_DEFAULT_RELIABILITY;
    default:
      break;
  }

  switch (qos_profile->durability) {
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
    case RMW_QOS_POLICY_DURABILITY_UNKNOWN:
      qos_profile->durability = RMW_ZENOHPICO_DEFAULT_DURABILITY;
    default:
      break;
  }

  if (rmw_time_equal(qos_profile->deadline, (rmw_time_t)RMW_QOS_DEADLINE_DEFAULT)) {
    qos_profile->deadline = (rmw_time_t)RMW_ZENOHPICO_DEFAULT_DEADLINE;
  }

  if (rmw_time_equal(qos_profile->lifespan, (rmw_time_t)RMW_QOS_LIFESPAN_DEFAULT)) {
    qos_profile->lifespan = (rmw_time_t)RMW_ZENOHPICO_DEFAULT_LIFESPAN;
  }

  if (rmw_time_equal(qos_profile->liveliness_lease_duration,
                     (rmw_time_t)RMW_QOS_LIVELINESS_LEASE_DURATION_DEFAULT)) {
    qos_profile->liveliness_lease_duration =
        (rmw_time_t)RMW_ZENOHPICO_DEFAULT_LIVELINESS_LEASE_DURATION;
  }

  switch (qos_profile->liveliness) {
    case RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT:
    case RMW_QOS_POLICY_LIVELINESS_UNKNOWN:
      qos_profile->liveliness = RMW_ZENOHPICO_DEFAULT_LIVELINESS;
    default:
      break;
  }

  return RMW_RET_OK;
}
