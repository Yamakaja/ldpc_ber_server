cmake_minimum_required(VERSION 3.12)
project(sdfec LANGUAGES C CXX)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -flto")
SET(CMAKE_C_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -flto")
SET(CMAKE_EXE_LINKER_FLAGS "-flto")

find_package(pybind11 REQUIRED)

pybind11_add_module(sdfec
    sdfec_usr_intf.c
    sdfec.cpp
    sdfec_bindings.cpp
    xsdfec_stats.cpp)

target_link_libraries(sdfec PRIVATE ssl)

execute_process(COMMAND python3 -c "import site; print(site.getsitepackages()[0])"
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES)

install(TARGETS sdfec DESTINATION ${PYTHON_SITE_PACKAGES})
