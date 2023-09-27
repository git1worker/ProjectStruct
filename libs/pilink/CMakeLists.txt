cmake_minimum_required(VERSION 3.5)

set(LIBRARY_NAME pilink)

project(${LIBRARY_NAME} LANGUAGES CXX)

# TODO: may be move backeds to separate CMake projects?

# LIBUSB BACKEND

find_package(libusb QUIET)
if (NOT libusb_FOUND)
  find_package(PkgConfig)
  if (PkgConfig_FOUND)
    pkg_check_modules(libusb-1.0 REQUIRED IMPORTED_TARGET libusb-1.0)
    add_library(libusb::libusb ALIAS PkgConfig::libusb-1.0)
  endif (PkgConfig_FOUND)
endif (NOT libusb_FOUND)

find_package(Boost 1.83.0 COMPONENTS url REQUIRED)

set(LIBRARY_LIBUSB_BACKEND_HEADERS
  src/transport/usb/libusb/device.hpp
  src/transport/usb/libusb/error.hpp
  src/transport/usb/libusb/enumerate.hpp
)

set(LIBRARY_LIBUSB_BACKEND_SOURCES
  src/transport/usb/libusb/device.cpp
  src/transport/usb/libusb/error.cpp
  src/transport/usb/libusb/enumerate.cpp
)

set(LIBRARY_LIBUSB_BACKEND_DEPS
  PRIVATE libusb::libusb
  PRIVATE Boost::url
)
#

#WINUSB BACKEND
  # TODO:
#

set(LIBRARY_HEADERS
  include/${LIBRARY_NAME}/pilink.hpp
)

set(LIBRARY_SOURCES
  src/pilink.cpp
)

add_library(${LIBRARY_NAME}
  ${LIBRARY_HEADERS}
  ${LIBRARY_SOURCES}

  ${LIBRARY_LIBUSB_BACKEND_HEADERS}
  ${LIBRARY_LIBUSB_BACKEND_SOURCES}
)
add_library("${LIBRARY_NAME}::${LIBRARY_NAME}" ALIAS ${LIBRARY_NAME})

if((CMAKE_C_COMPILER_ID MATCHES "GNU") OR (CMAKE_C_COMPILER_ID MATCHES "Clang"))
  target_compile_options(${LIBRARY_NAME}
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:
    -Wall
    -Wextra
    -Wpedantic
    -Wcast-align
    -Wcast-qual
    -Wconversion
    -Wctor-dtor-privacy
    -Wenum-compare
    -Wfloat-equal
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo>
  )
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${LIBRARY_NAME} PRIVATE $<$<COMPILE_LANGUAGE:C>:/W4>)
endif()

target_compile_features(${LIBRARY_NAME}
  PRIVATE cxx_std_17
)

target_include_directories(${LIBRARY_NAME}
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}>
  PRIVATE $<INSTALL_INTERFACE:include/${LIBRARY_NAME}>

  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>
)

target_link_libraries(${LIBRARY_NAME}
  ${LIBRARY_LIBUSB_BACKEND_DEPS}
)

install(TARGETS ${LIBRARY_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
