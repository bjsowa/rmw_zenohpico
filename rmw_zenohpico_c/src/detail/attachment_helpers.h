#ifndef RMW_ZENOHPICO_DETAIL__ATTACHMENT_HELPERS_H_
#define RMW_ZENOHPICO_DETAIL__ATTACHMENT_HELPERS_H_

#include <stdint.h>

#include "rmw/types.h"
#include "zenoh-pico.h"

typedef struct {
  int64_t sequence_number;
  int64_t source_timestamp;
  uint8_t source_gid[RMW_GID_STORAGE_SIZE];
} rmw_zp_attachment_data_t;

rmw_ret_t rmw_zp_attachment_data_serialize_to_zbytes(
    const rmw_zp_attachment_data_t* attachment_data, z_owned_bytes_t* attachment);

rmw_ret_t rmw_zp_attachment_data_deserialize_from_zbytes(const z_loaned_bytes_t* attachment,
                                                         rmw_zp_attachment_data_t* attachment_data);

void rmw_zp_attachment_data_clone(const rmw_zp_attachment_data_t* attachment_data_src,
                                  rmw_zp_attachment_data_t* attachment_data_dst);

#endif