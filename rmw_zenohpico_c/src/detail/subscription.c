#include "./subscription.h"

#include "./attachment_helpers.h"
#include "zenoh-pico.h"

rmw_ret_t rmw_zenohpico_subscription_init(rmw_zenohpico_subscription_t* subscription) {
  return RMW_RET_OK;
}

rmw_ret_t rmw_zenohpico_subscription_fini(rmw_zenohpico_subscription_t* subscription) {
  return RMW_RET_OK;
}

void rmw_zenohpico_sub_data_handler(const z_loaned_sample_t* sample, void* data) {
  z_view_string_t keystr;
  z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);

  rmw_zenohpico_subscription_t* sub_data = data;
  if (sub_data == NULL) {
    // TODO: report error
    return;
  }

  rmw_zenohpico_attachment_data_t attachment_data;

  const z_loaned_bytes_t* attachment = z_sample_attachment(sample);
  if (!_z_bytes_check(attachment)) {
    // TODO: report error
    // Set fallback attachment data
    attachment_data.sequence_number = 0;
    attachment_data.source_timestamp = 0;
    memset(attachment_data.source_gid, 0, RMW_GID_STORAGE_SIZE);
  } else if (rmw_zenohpico_attachment_data_deserialize_from_zbytes(attachment, &attachment_data) !=
             RMW_RET_OK) {
    // Deserialize method will set default values for attachment data it fails to deserialize
    // TODO: report error
  }

  const z_loaned_bytes_t* payload = z_sample_payload(sample);

  // TODO: avoid dynamic allocation
  z_owned_slice_t slice;
  z_bytes_deserialize_into_slice(payload, &slice);

  if (rmw_zenohpico_subscription_add_new_message(sub_data, &attachment_data, z_move(slice)) !=
      RMW_RET_OK) {
    // TODO: report error
  }
}

rmw_ret_t rmw_zenohpico_subscription_add_new_message(
    rmw_zenohpico_subscription_t* subscription,
    const rmw_zenohpico_attachment_data_t* attachment_data, z_moved_slice_t payload) {
  return RMW_RET_UNSUPPORTED;
}