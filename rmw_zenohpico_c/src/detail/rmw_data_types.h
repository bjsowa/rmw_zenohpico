#ifndef RMW_ZENOHPICO_DETAIL__RMW_DATA_TYPES_H_
#define RMW_ZENOHPICO_DETAIL__RMW_DATA_TYPES_H_

#include "rmw/types.h"
#include "zenoh-pico.h"

struct rmw_context_impl_s {
  // An owned session.
  z_owned_session_t session;

  /// Shutdown flag.
  bool is_shutdown;
};

struct rmw_init_options_impl_s {
  // An owned config.
  z_owned_config_t config;
};

typedef struct rmw_zenohpico_node_s {
  uint8_t __dummy;
} rmw_zenohpico_node_t;

#endif