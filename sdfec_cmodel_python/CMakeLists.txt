cmake_minimum_required(VERSION 3.12)
project(sdfec_cmodel LANGUAGES C CXX)

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -flto")
SET(CMAKE_C_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -flto")
SET(CMAKE_EXE_LINKER_FLAGS "-flto")
set(SDFEC_CMODEL_DIR "/opt/Xilinx/sdfec_cmodel/" CACHE FILEPATH "The location of the sdfec cmodel files.")

find_package(pybind11 REQUIRED)

pybind11_add_module(sdfec_cmodel
    sdfec_cmodel.cpp
    sdfec_cmodel_bindings.cpp
    sdfec_cmodel_shim.cpp
    xoroshiro128plus.c
    xoroshiro128plus_matrices.c
    ldpc_ber_tester.cpp
    boxmuller.cpp
    boxmuller_rom.cpp)

target_link_directories(sdfec_cmodel PRIVATE "${SDFEC_CMODEL_DIR}")
target_include_directories(sdfec_cmodel PRIVATE "${SDFEC_CMODEL_DIR}")

target_link_libraries(sdfec_cmodel PRIVATE fmt dl m)
# target_link_libraries(sdfec_cmodel PRIVATE Ip_sd_fec_v1_1_bitacc_cmodel fmt)

target_compile_definitions(sdfec_cmodel PRIVATE "-DSDFEC_CMODEL_DIR=\"${SDFEC_CMODEL_DIR}\"")

execute_process(COMMAND python3 -c "import site; print(site.getsitepackages()[0])"
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES)

install(TARGETS sdfec_cmodel DESTINATION ${PYTHON_SITE_PACKAGES})


###############################################################
# Second shared library using dlopen to control dynamic linkage
###############################################################
pybind11_add_module(sdfec_cmodel_exp
    sdfec_cmodel_exp_bindings.cpp
    sdfec_cmodel_exp.cpp)

target_link_directories(sdfec_cmodel_exp PRIVATE "${SDFEC_CMODEL_DIR}")
target_include_directories(sdfec_cmodel_exp PRIVATE "${SDFEC_CMODEL_DIR}")

target_link_libraries(sdfec_cmodel_exp PRIVATE dl fmt m)

target_compile_definitions(sdfec_cmodel_exp PRIVATE "-DSDFEC_CMODEL_DIR=\"${SDFEC_CMODEL_DIR}\"")

# install(TARGETS sdfec_cmodel_exp DESTINATION ${PYTHON_SITE_PACKAGES})
