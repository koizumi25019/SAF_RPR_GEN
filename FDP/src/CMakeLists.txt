cmake_minimum_required(VERSION 3.16)
project(SAF_RPR_GEN C)

set(CMAKE_C_STANDARD 11)

set(SOURCES
    main.c
    fdp/create_TPG_model.c
    fdp/cudd_wrapper.c
    fdp/drop_dete_fault.c
    fdp/essential_assignment.c
    fdp/fault_detection_prob.c
    fdp/gmp_wrapper.c
    fdp/init.c
    fdp/read.c
    fdp/target_fault.c
    fdp/cnf/detection_circuit.c
    fdp/cnf/faulty_circuit.c
    fdp/cnf/good_circuit.c
    lib/lib.c
    netlist/netlist.c
    opt/opt.c
    fdp/xid/xid_adapter.c
    fdp/xid/XID.c
    fdp/xid/xid_fsim.c
    fdp/xid/xid_forward_imp.c
    fdp/xid/xid_backward_imp.c
    fdp/xid/xid_backward_imp_limited.c
)

set(TOOLS_DIR ${CMAKE_SOURCE_DIR}/../tools)

set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/fdp
    ${CMAKE_SOURCE_DIR}/fdp/cnf
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/netlist
    ${CMAKE_SOURCE_DIR}/opt
    ${CMAKE_SOURCE_DIR}/fdp/xid
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