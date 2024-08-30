#include "detail/attachment_helpers.h"
#include "detail/identifiers.h"
#include "detail/publisher.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/error_handling.h"
#include "rmw/ret_types.h"
#include "rmw/rmw.h"

rmw_ret_t rmw_publish(const rmw_publisher_t *publisher, const void *ros_message,
                      rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(allocation);

  RMW_CHECK_FOR_NULL_WITH_MSG(publisher, "publisher handle is null",
                              return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(publisher, publisher->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(ros_message, "ros message handle is null",
                              return RMW_RET_INVALID_ARGUMENT);

  rmw_zp_publisher_t *publisher_data = publisher->data;
  RMW_CHECK_FOR_NULL_WITH_MSG(publisher_data, "publisher_data is null",
                              return RMW_RET_INVALID_ARGUMENT);

  rcutils_allocator_t *allocator = &(publisher_data->context->options.allocator);

  // Serialize data.
  size_t serialized_size =
      rmw_zp_type_support_get_serialized_size(publisher_data->type_support, ros_message);

  // To store serialized message byte array.
  uint8_t *msg_bytes = allocator->allocate(serialized_size, allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(msg_bytes, "bytes for message is null", return RMW_RET_BAD_ALLOC);

  if (rmw_zp_type_support_serialize_ros_message(publisher_data->type_support, ros_message,
                                                msg_bytes, serialized_size) != RMW_RET_OK) {
    goto fail_serialize_ros_message;
  }

  // create attachment
  int64_t sequence_number = rmw_zp_publisher_get_next_sequence_number(publisher_data);

  zp_time_since_epoch time_since_epoch;
  if (zp_get_time_since_epoch(&time_since_epoch) < 0) {
    RMW_SET_ERROR_MSG("Zenoh-pico port does not support zp_get_time_since_epoch");
    goto fail_get_time_since_epoch;
  }

  int64_t source_timestamp =
      (int64_t)time_since_epoch.secs * 1000000000ll + (int64_t)time_since_epoch.nanos;

  rmw_zp_attachment_data_t attachment_data = {.sequence_number = sequence_number,
                                              .source_timestamp = source_timestamp};
  memcpy(attachment_data.source_gid, publisher_data->pub_gid, RMW_GID_STORAGE_SIZE);

  z_owned_bytes_t attachment;
  if (rmw_zp_attachment_data_serialize_to_zbytes(&attachment_data, &attachment) != RMW_RET_OK) {
    goto fail_serialize_attachment;
  }

  // The encoding is simply forwarded and is useful when key expressions in the
  // session use different encoding formats. In our case, all key expressions
  // will be encoded with CDR so it does not really matter.
  z_publisher_put_options_t options;
  z_publisher_put_options_default(&options);
  options.attachment = z_move(attachment);

  z_owned_bytes_t payload;
  z_bytes_from_static_buf(&payload, msg_bytes, serialized_size);

  if (z_publisher_put(z_loan(publisher_data->pub), z_move(payload), &options)) {
    RMW_SET_ERROR_MSG("unable to publish message");
    goto fail_publish_message;
  }

  allocator->deallocate(msg_bytes, allocator->state);

  return RMW_RET_OK;

fail_publish_message:
  z_drop(options.attachment);
fail_serialize_attachment:
fail_get_time_since_epoch:
fail_serialize_ros_message:
  allocator->deallocate(msg_bytes, allocator->state);
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_publish_loaned_message(const rmw_publisher_t *publisher, void *ros_message,
                                     rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publish_serialized_message(const rmw_publisher_t *publisher,
                                         const rmw_serialized_message_t *serialized_message,
                                         rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(serialized_message);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_ERROR;
}