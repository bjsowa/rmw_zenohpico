#include "./time.h"

#include "rmw/error_handling.h"
#include "zenoh-pico.h"

rmw_ret_t rmw_zp_get_current_timestamp(int64_t *timestamp) {
  zp_time_since_epoch time_since_epoch;
  if (zp_get_time_since_epoch(&time_since_epoch) < 0) {
    RMW_SET_ERROR_MSG("Zenoh-pico port does not support zp_get_time_since_epoch");
    return RMW_RET_ERROR;
  }

  *timestamp = (int64_t)time_since_epoch.secs * 1000000000ll + (int64_t)time_since_epoch.nanos;

  return RMW_RET_OK;
}