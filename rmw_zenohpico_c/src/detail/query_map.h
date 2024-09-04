#ifndef RMW_ZENOHPICO_DETAIL__QUERY_QUEUE_H_
#define RMW_ZENOHPICO_DETAIL__QUERY_QUEUE_H_

#include <stdint.h>

#include "rcutils/allocator.h"
#include "rmw/rmw.h"
#include "zenoh-pico.h"

typedef struct {
  uint32_t* keys;
  const z_loaned_query_t** queries;
  size_t capacity;
} rmw_zp_query_map_t;

rmw_ret_t rmw_zp_query_map_init(rmw_zp_query_map_t* query_map, size_t capacity,
                                rcutils_allocator_t* allocator);

rmw_ret_t rmw_zp_query_map_fini(rmw_zp_query_map_t* query_map, rcutils_allocator_t* allocator);

rmw_ret_t rmw_zp_query_map_insert(rmw_zp_query_map_t* query_map, const z_loaned_query_t* query,
                                  int64_t sequence_number, const uint8_t* writer_guid);

rmw_ret_t rmw_zp_query_map_extract(rmw_zp_query_map_t* query_map, int64_t sequence_number,
                                   const uint8_t* writer_guid, const z_loaned_query_t** query);

#endif