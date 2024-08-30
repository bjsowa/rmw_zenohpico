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
  RCUTILS_UNUSED(attachment);
  attachment_data->sequence_number = 0;
  attachment_data->source_timestamp = 0;
  memset(attachment_data->source_gid, 0, RMW_GID_STORAGE_SIZE);

  // TODO: Attachments in rmw_zenoh right now are only used to detect lost messages by checking if
  // sequence numbers associated with publisher GID are monotonically increasing. Do we need to
  // implement this in rmw_zenohpico?

  return RMW_RET_ERROR;
}