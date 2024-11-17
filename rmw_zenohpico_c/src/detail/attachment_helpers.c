#include "./attachment_helpers.h"

#include "./macros.h"
#include "rcutils/macros.h"
#include "rmw/error_handling.h"

rmw_ret_t rmw_zp_attachment_data_serialize_to_zbytes(
    const rmw_zp_attachment_data_t *attachment_data, z_owned_bytes_t *attachment) {
  ze_owned_serializer_t serializer;
  ze_serializer_empty(&serializer);

  if (ze_serializer_serialize_str(z_loan_mut(serializer), "sequence_number") < 0) {
    RMW_SET_ERROR_MSG("Failed to serialize sequence_number key");
    goto fail;
  }

  if (ze_serializer_serialize_int64(z_loan_mut(serializer), attachment_data->sequence_number) < 0) {
    RMW_SET_ERROR_MSG("Failed to serialize sequence_number value");
    goto fail;
  }

  if (ze_serializer_serialize_str(z_loan_mut(serializer), "source_timestamp") < 0) {
    RMW_SET_ERROR_MSG("Failed to serialize source_timestamp key");
    goto fail;
  }

  if (ze_serializer_serialize_int64(z_loan_mut(serializer), attachment_data->source_timestamp) <
      0) {
    RMW_SET_ERROR_MSG("Failed to serialize source_timestamp value");
    goto fail;
  }

  if (ze_serializer_serialize_str(z_loan_mut(serializer), "source_gid") < 0) {
    RMW_SET_ERROR_MSG("Failed to serialize source_gid key");
    goto fail;
  }

  if (ze_serializer_serialize_buf(z_loan_mut(serializer), attachment_data->source_gid,
                                  RMW_GID_STORAGE_SIZE) < 0) {
    RMW_SET_ERROR_MSG("Failed to serialize source_gid value");
    goto fail;
  }

  ze_serializer_finish(z_move(serializer), attachment);

  return RMW_RET_OK;

fail:
  z_drop(z_move(serializer));
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_zp_attachment_data_deserialize_from_zbytes(
    const z_loaned_bytes_t *attachment, rmw_zp_attachment_data_t *attachment_data) {
  if (z_bytes_is_empty(attachment)) {
    RMW_SET_ERROR_MSG("Received empty attachment");
    return RMW_RET_ERROR;
  }

  ze_deserializer_t deserializer = ze_deserializer_from_bytes(attachment);
  z_owned_string_t key;

  // Sequence number
  if (ze_deserializer_deserialize_string(&deserializer, &key) < 0) {
    RMW_SET_ERROR_MSG("Failed to deserialize key from attachment");
    return RMW_RET_ERROR;
  }

  if (strncmp(z_string_data(z_loan(key)), "sequence_number", z_string_len(z_loan(key))) != 0) {
    RMW_SET_ERROR_MSG("sequence_number is not found in the attachment.");
    z_drop(z_move(key));
    return RMW_RET_ERROR;
  }
  z_drop(z_move(key));
  if (ze_deserializer_deserialize_int64(&deserializer, &attachment_data->sequence_number)) {
    RMW_SET_ERROR_MSG("Failed to deserialize the sequence_number.");
    return RMW_RET_ERROR;
  }

  // Source timestamp
  if (ze_deserializer_deserialize_string(&deserializer, &key) < 0) {
    RMW_SET_ERROR_MSG("Failed to deserialize key from attachment");
    return RMW_RET_ERROR;
  }

  if (strncmp(z_string_data(z_loan(key)), "source_timestamp", z_string_len(z_loan(key))) != 0) {
    RMW_SET_ERROR_MSG("source_timestamp is not found in the attachment.");
    z_drop(z_move(key));
    return RMW_RET_ERROR;
  }
  z_drop(z_move(key));
  if (ze_deserializer_deserialize_int64(&deserializer, &attachment_data->source_timestamp)) {
    RMW_SET_ERROR_MSG("Failed to deserialize the source_timestamp.");
    return RMW_RET_ERROR;
  }

  // Source GID
  if (ze_deserializer_deserialize_string(&deserializer, &key) < 0) {
    RMW_SET_ERROR_MSG("Failed to deserialize key from attachment");
    return RMW_RET_ERROR;
  }

  if (strncmp(z_string_data(z_loan(key)), "source_gid", z_string_len(z_loan(key))) != 0) {
    RMW_SET_ERROR_MSG("source_gid is not found in the attachment.");
    z_drop(z_move(key));
    return RMW_RET_ERROR;
  }
  z_drop(z_move(key));

  z_owned_slice_t slice;
  if (ze_deserializer_deserialize_slice(&deserializer, &slice)) {
    RMW_SET_ERROR_MSG("Failed to deserialize the source_gid.");
    return RMW_RET_ERROR;
  }
  if (z_slice_len(z_loan(slice)) != RMW_GID_STORAGE_SIZE) {
    RMW_SET_ERROR_MSG("The length of source_gid mismatched.");
    z_drop(z_move(slice));
    return RMW_RET_ERROR;
  }
  memcpy(attachment_data->source_gid, z_slice_data(z_loan(slice)), z_slice_len(z_loan(slice)));
  z_drop(z_move(slice));

  return RMW_RET_OK;
}

void rmw_zp_attachment_data_clone(const rmw_zp_attachment_data_t *attachment_data_src,
                                  rmw_zp_attachment_data_t *attachment_data_dst) {
  attachment_data_dst->sequence_number = attachment_data_src->sequence_number;
  attachment_data_dst->source_timestamp = attachment_data_src->source_timestamp;
  memcpy(attachment_data_dst->source_gid, attachment_data_src->source_gid, RMW_GID_STORAGE_SIZE);
}
