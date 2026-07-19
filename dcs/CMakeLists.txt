cmake_minimum_required (VERSION 2.8)
project (cea2045)
set(LIBCEA2045_MAJOR_VERSION 1)
set(LIBCEA2045_MINOR_VERSION 0)
set(LIBCEA2045_MICRO_VERSION 0)
set(LIBCEA2045_VERSION ${LIBCEA2045_MAJOR_VERSION}.${LIBCEA2045_MINOR_VERSION}.${LIBCEA2045_MICRO_VERSION})

# options off-by-default that you can enable
option(WERROR "Set all warnings to errors" OFF)
option(STATICLIBS "Builds static versions of all installed libraries" OFF)
option(CONTROLLER "Build the CTA-2045 controller application" ON)

if(STATICLIBS)
  set(LIB_TYPE STATIC)
else()
  set(LIB_TYPE SHARED)
endif()

# assume gcc/clang otherwise
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

# different release and debug flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

if (WERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS})

# include paths for all the local libraries
include_directories(./cea2045)
include_directories(./third_party)

# ---- libcea2045 ----
file(GLOB_RECURSE cea2045_SRC ./cea2045/cea2045/*.cpp)
add_library(cea2045 ${LIB_TYPE} ${cea2045_SRC})
#set(INSTALL_ARGS FILES_MATCHING PATTERN "*.h" PATTERN ".deps" EXCLUDE PATTERN ".libs" EXCLUDE)
install(TARGETS cea2045 DESTINATION lib)
set(INSTALL_ARGS FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ./cea2045/cea2045 DESTINATION include ${INSTALL_ARGS})

if (CONTROLLER)
  file(GLOB_RECURSE controller_SRC ./controller/*.cpp)
  add_executable(cta2045_controller ${controller_SRC})
  target_link_libraries(cta2045_controller LINK_PUBLIC cea2045 pthread)
endif()

