cmake_minimum_required(VERSION 3.8)
project(rmw_zenohpico_c)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(microcdr REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_typesupport_microxrcedds_c REQUIRED)
find_package(zenohpico_vendor REQUIRED)
find_package(zenohpico REQUIRED)

set(SRCS
  src/detail/guard_condition.c
  src/detail/identifier.c
  src/detail/wait_set.c
  src/rmw_guard_condition.c
  src/rmw_init_options.c
  src/rmw_init.c
  src/rmw_node.c
)

add_library(${PROJECT_NAME}
  ${SRCS}
)

target_link_libraries(${PROJECT_NAME}
  microcdr
  rmw::rmw
  rosidl_runtime_c::rosidl_runtime_c
  rosidl_typesupport_microxrcedds_c::rosidl_typesupport_microxrcedds_c
  zenohpico::zenohpico
)

configure_rmw_library(${PROJECT_NAME})

register_rmw_implementation("c:rosidl_typesupport_c:rosidl_typesupport_microxrcedds_c")

install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  microcdr
  rmw
  rosidl_runtime_c
  rosidl_typesupport_microxrcedds_c
  zenohpico_vendor
  zenohpico
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
