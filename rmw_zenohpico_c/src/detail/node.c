#include "./node.h"

#include "rcutils/macros.h"

rmw_ret_t rmw_zp_node_init(rmw_zp_node_t* node) {
  RCUTILS_UNUSED(node);
  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_node_fini(rmw_zp_node_t* node) {
  RCUTILS_UNUSED(node);
  return RMW_RET_OK;
}
