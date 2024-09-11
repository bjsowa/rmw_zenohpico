cmake_minimum_required(VERSION 3.8)
project(zenohpico_vendor)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_vendor_package REQUIRED)

find_package(zenohpico QUIET)

ament_vendor(zenohpico_vendor
  VCS_URL "https://github.com/eclipse-zenoh/zenoh-pico.git"
  VCS_VERSION "8a5f1bb38d3aafb1a94609d1f221d8357d40d9e7"
  SATISFIED ${zenohpico_FOUND}
)

ament_package()