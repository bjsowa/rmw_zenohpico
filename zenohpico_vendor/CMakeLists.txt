cmake_minimum_required(VERSION 3.8)
project(zenohpico_vendor)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_vendor_package REQUIRED)

ament_vendor(zenohpico_vendor
  VCS_URL "https://github.com/eclipse-zenoh/zenoh-pico.git"
  VCS_VERSION "e837b672f3412f361873ba4b780a2d01360465f6"
)

ament_package()