cmake_minimum_required(VERSION 2.4)

project(httpd_server)

find_package(Threads)

add_executable(httpd_server httpd.c)

target_link_libraries(httpd_server Threads::Threads)
