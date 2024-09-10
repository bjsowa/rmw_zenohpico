cmake_minimum_required(VERSION 3.8)
project(zenohpico_vendor)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_vendor_package REQUIRED)

find_package(zenohpico QUIET)

ament_vendor(zenohpico_vendor
  VCS_URL "https://github.com/eclipse-zenoh/zenoh-pico.git"
  VCS_VERSION "9e926b5b517336d2a78d54b9515f420241fc7dc1"
  SATISFIED ${zenohpico_FOUND}
)

ament_package()