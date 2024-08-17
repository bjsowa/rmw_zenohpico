#ifndef RMW_ZENOHPICO_DETAIL__NODE_H_
#define RMW_ZENOHPICO_DETAIL__NODE_H_

#include <stdint.h>

#include "rmw/ret_types.h"

typedef struct rmw_zenohpico_node_s {
  uint8_t __dummy;
} rmw_zenohpico_node_t;

rmw_ret_t rmw_zenohpico_node_init(rmw_zenohpico_node_t* node);

rmw_ret_t rmw_zenohpico_node_fini(rmw_zenohpico_node_t* node);

#endif