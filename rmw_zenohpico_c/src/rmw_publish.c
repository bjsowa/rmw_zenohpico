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
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(ros_message, "ros message handle is null",
                              return RMW_RET_INVALID_ARGUMENT);

  rmw_zenohpico_publisher_t *publisher_data = publisher->data;
  RMW_CHECK_FOR_NULL_WITH_MSG(publisher_data, "publisher_data is null",
                              return RMW_RET_INVALID_ARGUMENT);

  rcutils_allocator_t *allocator = &(publisher_data->context->options.allocator);

  // Serialize data.
  size_t serialized_size =
      rmw_zenohpico_type_support_get_serialized_size(publisher_data->type_support, ros_message);

  // To store serialized message byte array.
  uint8_t *msg_bytes = allocator->allocate(serialized_size, allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(msg_bytes, "bytes for message is null", return RMW_RET_BAD_ALLOC);

  if (rmw_zenohpico_type_support_serialize_ros_message(publisher_data->type_support, ros_message,
                                                       msg_bytes, serialized_size) != RMW_RET_OK) {
    goto fail_serialize_ros_message;
  }

  // TODO: create attachment

  // The encoding is simply forwarded and is useful when key expressions in the
  // session use different encoding formats. In our case, all key expressions
  // will be encoded with CDR so it does not really matter.
  z_publisher_put_options_t options;
  z_publisher_put_options_default(&options);
  // options.attachment = z_move(attachment);

  z_owned_bytes_t payload;
  z_bytes_from_static_buf(&payload, msg_bytes, serialized_size);

  if (z_publisher_put(z_loan(publisher_data->pub), z_move(payload), &options)) {
    RMW_SET_ERROR_MSG("unable to publish message");
    return RMW_RET_ERROR;
  }

  allocator->deallocate(msg_bytes, allocator->state);

  return RMW_RET_OK;

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