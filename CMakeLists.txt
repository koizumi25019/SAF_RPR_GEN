cmake_minimum_required(VERSION 3.16)
project(fdp C)

set(CMAKE_C_STANDARD 11)

set(SOURCES
    src/main.c
    src/fdp/create_TPG_model.c
    src/fdp/cube_set.c
    src/fdp/cudd_wrapper.c
    src/fdp/drop_dete_fault.c
    src/fdp/essential_assignment.c
    src/fdp/fault_detection_prob.c
    src/fdp/gt_verify.c
    src/fdp/pcount.c
    src/fdp/aig_dump.c
    src/fdp/cube_trend.c
    src/fdp/experiment.c
    src/fdp/cnf_dump.c
    src/fdp/gmp_wrapper.c
    src/fdp/init.c
    src/fdp/read.c
    src/fdp/target_fault.c
    src/fdp/cnf/detection_circuit.c
    src/fdp/cnf/faulty_circuit.c
    src/fdp/cnf/good_circuit.c
    src/lib/lib.c
    src/netlist/netlist.c
    src/opt/opt.c
    src/fdp/xid/xid_adapter.c
    src/fdp/xid/XID.c
    src/fdp/xid/xid_fsim.c
    src/fdp/xid/xid_forward_imp.c
    src/fdp/xid/xid_backward_imp.c
    src/fdp/xid/xid_backward_imp_limited.c
)

set(TOOLS_DIR ${CMAKE_SOURCE_DIR}/external)

set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/fdp
    ${CMAKE_SOURCE_DIR}/src/fdp/cnf
    ${CMAKE_SOURCE_DIR}/src/lib
    ${CMAKE_SOURCE_DIR}/src/netlist
    ${CMAKE_SOURCE_DIR}/src/opt
    ${CMAKE_SOURCE_DIR}/src/fdp/xid
    ${TOOLS_DIR}/cadical/src
    ${TOOLS_DIR}/cudd
    ${TOOLS_DIR}/cudd/cudd
)

link_directories(
    ${TOOLS_DIR}/cadical/build
    ${TOOLS_DIR}/cudd/cudd/.libs
)

# Debugビルド
add_executable(main_debug ${SOURCES})
target_compile_options(main_debug PRIVATE -g -O0 -DDEBUG -fcommon)

target_include_directories(main_debug PRIVATE ${COMMON_INCLUDES})
target_link_libraries(main_debug cadical cudd gmp m stdc++)

# Releaseビルド
add_executable(main_release ${SOURCES})
target_compile_options(main_release PRIVATE -O3 -DNDEBUG -fcommon)
target_include_directories(main_release PRIVATE ${COMMON_INCLUDES})
target_link_libraries(main_release cadical cudd gmp m stdc++)