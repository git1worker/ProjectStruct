cmake_minimum_required(VERSION 3.5)

set(APP_NAME mpl1c)

project(${APP_NAME} LANGUAGES CXX)

#set(Boost_USE_STATIC_LIBS     OFF)
#set(Boost_USE_STATIC_RUNTIME  OFF)
#find_package(Boost COMPONENTS nowide REQUIRED)

add_executable(${APP_NAME}
  src/mpl1c.cpp
)

target_compile_features(${APP_NAME}
  PRIVATE cxx_std_20
)

target_compile_options(${PROJECT_NAME} PRIVATE 
  -Wall 
  -Wextra 
  -Wconversion 
  -Wsign-conversion
)

target_link_libraries(${APP_NAME}
#  PRIVATE Boost::nowide
  PRIVATE pilink::pilink
)
