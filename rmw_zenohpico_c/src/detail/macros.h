#ifndef RMW_ZENOHPICO_DETAIL__MACROS_H_
#define RMW_ZENOHPICO_DETAIL__MACROS_H_

#include "rcutils/macros.h"
#include "rmw/ret_types.h"

#define RMW_UNUSED(statement)        \
  {                                  \
    rmw_ret_t tmp_ret = (statement); \
    RCUTILS_UNUSED(tmp_ret);         \
  }

#endif