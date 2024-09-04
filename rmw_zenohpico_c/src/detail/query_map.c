#include "./query_map.h"

#include "rmw/error_handling.h"

static uint32_t get_query_id_hash(const int64_t sequence_number, const uint8_t* const writer_guid) {
  const uint32_t fnv_offset_basis = 0x811C9DC5UL;
  const uint32_t fnv_prime = 0x01000193UL;

  uint32_t hash = fnv_offset_basis;

  const uint8_t* const seq_ptr = (uint8_t*)&sequence_number;
  for (size_t i = 0; i < sizeof(sequence_number); i++) {
    hash ^= (uint32_t)seq_ptr[i];
    hash *= fnv_prime;
  }

  for (size_t i = 0; i < RMW_GID_STORAGE_SIZE; i++) {
    hash ^= (uint32_t)writer_guid[i];
    hash *= fnv_prime;
  }

  return hash;
}

static int get_index(rmw_zp_query_map_t* query_map, uint32_t key) {
  for (size_t i = 0; i < query_map->capacity; i++) {
    if (query_map->keys[i] == key) {
      return i;
    }
  }
  return -1;
}

rmw_ret_t rmw_zp_query_map_init(rmw_zp_query_map_t* query_map, size_t capacity,
                                rcutils_allocator_t* allocator) {
  query_map->capacity = capacity;

  query_map->keys = allocator->zero_allocate(capacity, sizeof(uint32_t), allocator->state);

  if (query_map->keys == NULL) {
    RMW_SET_ERROR_MSG("Failed to allocate query map keys");
    return RMW_RET_ERROR;
  }

  query_map->queries = allocator->allocate(capacity * sizeof(z_loaned_query_t*), allocator->state);

  if (query_map->keys == NULL) {
    RMW_SET_ERROR_MSG("Failed to allocate query map queries");
    allocator->deallocate(query_map->keys, allocator->state);
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_query_map_fini(rmw_zp_query_map_t* query_map, rcutils_allocator_t* allocator) {
  if (query_map->keys != NULL) {
    allocator->deallocate(query_map->keys, allocator->state);
  }

  if (query_map->queries != NULL) {
    allocator->deallocate(query_map->queries, allocator->state);
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_query_map_insert(rmw_zp_query_map_t* query_map, const z_loaned_query_t* query,
                                  int64_t sequence_number, const uint8_t* writer_guid) {
  const uint32_t hash = get_query_id_hash(sequence_number, writer_guid);

  if (hash == 0) {
    RMW_SET_ERROR_MSG("Computed query id hash of value 0 is not allowed");
    return RMW_RET_ERROR;
  }

  int key_index = get_index(query_map, hash);
  if (key_index != -1) {
    RMW_SET_ERROR_MSG("Query id hash conflict. Is the client incrementing sequence ids?");
    return RMW_RET_ERROR;
  }

  for (size_t i = 0; i < query_map->capacity; i++) {
    if (query_map->keys[i] == 0) {
      query_map->keys[i] = hash;
      query_map->queries[i] = query;
      return RMW_RET_OK;
    }
  }

  RMW_SET_ERROR_MSG("Query map full");
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_zp_query_map_extract(rmw_zp_query_map_t* query_map, int64_t sequence_number,
                                   const uint8_t* writer_guid, const z_loaned_query_t** query) {
  const uint32_t hash = get_query_id_hash(sequence_number, writer_guid);

  int key_index = get_index(query_map, hash);
  if (key_index == -1) {
    RMW_SET_ERROR_MSG("Could not find query in the query map");
    return RMW_RET_ERROR;
  }

  query_map->keys[key_index] = 0;

  *query = query_map->queries[key_index];

  return RMW_RET_OK;
}