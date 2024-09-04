#include "./attachment_helpers.h"

#include "rcutils/macros.h"
#include "rmw/error_handling.h"

typedef struct {
  const rmw_zp_attachment_data_t *data;
  int idx;
} attachment_context_t;

static bool create_attachment_iter(z_owned_bytes_t *kv_pair, void *context) {
  attachment_context_t *ctx = (attachment_context_t *)context;
  z_owned_bytes_t k, v;

  if (ctx->idx == 0) {
    z_bytes_serialize_from_str(&k, "sequence_number");
    z_bytes_serialize_from_int64(&v, ctx->data->sequence_number);
  } else if (ctx->idx == 1) {
    z_bytes_serialize_from_str(&k, "source_timestamp");
    z_bytes_serialize_from_int64(&v, ctx->data->source_timestamp);
  } else if (ctx->idx == 2) {
    z_bytes_serialize_from_str(&k, "source_gid");
    z_bytes_serialize_from_buf(&v, ctx->data->source_gid, RMW_GID_STORAGE_SIZE);
  } else {
    return false;
  }

  z_bytes_from_pair(kv_pair, z_move(k), z_move(v));
  ctx->idx += 1;
  return true;
}

rmw_ret_t rmw_zp_attachment_data_serialize_to_zbytes(
    const rmw_zp_attachment_data_t *attachment_data, z_owned_bytes_t *attachment) {
  attachment_context_t context = {.data = attachment_data, .idx = 0};

  if (z_bytes_from_iter(attachment, create_attachment_iter, (void *)&context) < 0) {
    RMW_SET_ERROR_MSG("failed to serialize bytes for attachment");
    return RMW_RET_ERROR;
  };

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_attachment_data_deserialize_from_zbytes(
    const z_loaned_bytes_t *attachment, rmw_zp_attachment_data_t *attachment_data) {
  if (z_bytes_is_empty(attachment)) {
    RMW_SET_ERROR_MSG("Received empty attachment");
    return RMW_RET_ERROR;
  }

  z_bytes_iterator_t iter = z_bytes_get_iterator(attachment);
  z_owned_bytes_t pair, key, val;
  z_owned_string_t key_string;
  z_owned_slice_t gid_slice;
  bool sequence_number_found = false;
  bool source_timestamp_found = false;
  bool source_gid_found = false;

  while (z_bytes_iterator_next(&iter, &pair)) {
    if (z_bytes_deserialize_into_pair(z_loan(pair), &key, &val) < 0) {
      RMW_SET_ERROR_MSG("Failed to deserialize pair into key and value");
      goto fail_deserialize_pair;
    }

    if (z_bytes_deserialize_into_string(z_loan(key), &key_string) < 0) {
      RMW_SET_ERROR_MSG("Failed to deserialize key into string");
      goto fail_deserialize_key;
    }

    const char *key_string_ptr = z_string_data(z_loan(key_string));
    size_t key_string_len = z_string_len(z_loan(key_string));

    if (!sequence_number_found && strncmp(key_string_ptr, "sequence_number", key_string_len) == 0) {
      if (z_bytes_deserialize_into_int64(z_loan(val), &attachment_data->sequence_number) < 0) {
        RMW_SET_ERROR_MSG("Failed to deserialize sequence_number attachment");
        goto fail_deserialize_value;
      }

      sequence_number_found = true;

    } else if (!source_timestamp_found &&
               strncmp(key_string_ptr, "source_timestamp", key_string_len) == 0) {
      if (z_bytes_deserialize_into_int64(z_loan(val), &attachment_data->source_timestamp) < 0) {
        RMW_SET_ERROR_MSG("Failed to deserialize source_timestamp attachment");
        goto fail_deserialize_value;
      }

      source_timestamp_found = true;

    } else if (!source_gid_found && strncmp(key_string_ptr, "source_gid", key_string_len) == 0) {
      if (z_bytes_deserialize_into_slice(z_loan(val), &gid_slice) < 0) {
        RMW_SET_ERROR_MSG("Failed to deserialize source_gid attachment");
        goto fail_deserialize_value;
      }

      if (z_slice_len(z_loan(gid_slice)) != RMW_GID_STORAGE_SIZE) {
        RMW_SET_ERROR_MSG("GID length mismatched.");
        goto fail_gid_length_mismatch;
      }

      memcpy(attachment_data->source_gid, z_slice_data(z_loan(gid_slice)),
             z_slice_len(z_loan(gid_slice)));

      source_gid_found = true;
      z_drop(z_move(gid_slice));
    }

    z_drop(z_move(key_string));
    z_drop(z_move(key));
    z_drop(z_move(val));
    z_drop(z_move(pair));
  }

  if (!sequence_number_found) {
    RMW_SET_ERROR_MSG("Attachment is missing sequence_number");
    return RMW_RET_ERROR;
  }

  if (!source_timestamp_found) {
    RMW_SET_ERROR_MSG("Attachment is missing source_timestamp");
    return RMW_RET_ERROR;
  }

  if (!source_gid_found) {
    RMW_SET_ERROR_MSG("Attachment is missing source_gid");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;

fail_gid_length_mismatch:
  z_drop(z_move(gid_slice));
fail_deserialize_value:
  z_drop(z_move(key_string));
fail_deserialize_key:
  z_drop(z_move(key));
  z_drop(z_move(val));
fail_deserialize_pair:
  z_drop(z_move(pair));
  return RMW_RET_ERROR;
}

void rmw_zp_attachment_data_clone(const rmw_zp_attachment_data_t *attachment_data_src,
                                  rmw_zp_attachment_data_t *attachment_data_dst) {
  attachment_data_dst->sequence_number = attachment_data_src->sequence_number;
  attachment_data_dst->source_timestamp = attachment_data_src->source_timestamp;
  memcpy(attachment_data_dst->source_gid, attachment_data_src->source_gid, RMW_GID_STORAGE_SIZE);
}
