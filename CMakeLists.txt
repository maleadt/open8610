cmake_minimum_required (VERSION 2.6)
project (open8610)

add_library (linux8610 linux8610.h linux8610.c)

add_library (rw8610 rw8610.h rw8610.c)
target_link_libraries (rw8610 linux8610 m)

add_executable (dump8610 dump8610.c)
target_link_libraries (dump8610 rw8610)

add_executable (history8610 history8610.c)
target_link_libraries (history8610 rw8610)

add_executable (log8610 log8610.c)
target_link_libraries (log8610 rw8610)

add_executable (memreset8610 memreset8610.c)
target_link_libraries (memreset8610 rw8610)
