cmake_minimum_required (VERSION 2.6)
project (simple_server)
find_package(Threads)
add_executable (simple_server io.c tasks_common.c tasks_proc.c tasks_thread.c server.c)
target_link_libraries(simple_server pthread)
