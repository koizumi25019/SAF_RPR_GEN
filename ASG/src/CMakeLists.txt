cmake_minimum_required(VERSION 3.16)
project(SAF_RPR_GEN C)

set(CMAKE_C_STANDARD 11)

set(SOURCES
    main.c
    asg/create_TPG_model.c
    asg/cudd_wrapper.c
    asg/drop_dete_fault.c
    asg/essential_assignment.c
    asg/fault_detection_prob.c
    asg/gmp_wrapper.c
    asg/init.c
    asg/read.c
    asg/target_fault.c
    asg/cnf/detection_circuit.c
    asg/cnf/faulty_circuit.c
    asg/cnf/good_circuit.c
    lib/lib.c
    netlist/netlist.c
    opt/opt.c
    XID/xid_adapter.c
    XID/XID.c
    XID/xid_fsim.c
    XID/xid_forward_imp.c
    XID/xid_backward_imp.c
    XID/xid_backward_imp_limited.c
)

set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/asg
    ${CMAKE_SOURCE_DIR}/asg/cnf
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/netlist
    ${CMAKE_SOURCE_DIR}/opt
    ${CMAKE_SOURCE_DIR}/XID
    $ENV{HOME}/local/include
    /usr/local/include
)

link_directories(
    $ENV{HOME}/local/lib
    /usr/local/lib
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