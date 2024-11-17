#include "./time.h"

#include "rmw/error_handling.h"
#include "zenoh-pico.h"

rmw_ret_t rmw_zp_get_current_timestamp(int64_t *timestamp) {
  _z_time_since_epoch time_since_epoch;
  if (_z_get_time_since_epoch(&time_since_epoch) < 0) {
    RMW_SET_ERROR_MSG("Zenoh-pico port does not support _z_get_time_since_epoch");
    return RMW_RET_ERROR;
  }

  *timestamp = _z_timestamp_ntp64_from_time(time_since_epoch.secs, time_since_epoch.nanos);

  return RMW_RET_OK;
}