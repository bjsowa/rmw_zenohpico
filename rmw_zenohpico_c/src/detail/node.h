#ifndef RMW_ZENOHPICO_DETAIL__NODE_H_
#define RMW_ZENOHPICO_DETAIL__NODE_H_

#include <stdint.h>

#include "rmw/ret_types.h"

typedef struct {
  uint8_t __dummy;
} rmw_zp_node_t;

rmw_ret_t rmw_zp_node_init(rmw_zp_node_t* node);

rmw_ret_t rmw_zp_node_fini(rmw_zp_node_t* node);

#endif