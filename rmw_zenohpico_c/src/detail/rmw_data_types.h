#ifndef RMW_ZENOHPICO_DETAIL__RMW_DATA_TYPES_H_
#define RMW_ZENOHPICO_DETAIL__RMW_DATA_TYPES_H_

#include "rmw/types.h"
#include "zenoh-pico.h"

struct rmw_context_impl_s {
  // An owned session.
  z_owned_session_t session;

  /// Shutdown flag.
  bool is_shutdown;

  // Equivalent to rmw_dds_common::Context's guard condition
  /// Guard condition that should be triggered when the graph changes.
  rmw_guard_condition_t* graph_guard_condition;

  // A counter to assign a local id for every entity created in this session.
  size_t next_entity_id;
};

struct rmw_init_options_impl_s {
  // An owned config.
  z_owned_config_t config;
};

#endif