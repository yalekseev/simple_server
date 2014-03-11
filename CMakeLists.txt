cmake_minimum_required (VERSION 2.6)
project (simple_server)
add_executable (simple_server io.c handler.c server.c)
