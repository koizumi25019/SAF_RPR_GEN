cmake_minimum_required(VERSION 3.16)
project(SAF_RPR_GEN C)

set(CMAKE_C_STANDARD 11)

set(SOURCES
    main.c
    # asg/
    asg/createSGmodel.c
    asg/cudd_wrapper.c
    asg/drop_dete_fault.c
    asg/fault_detection_prob.c
    asg/gmp_wrapper.c
    asg/init.c
    asg/read.c
    asg/target_fault.c
    # asg/cnf/
    asg/cnf/detection_circuit.c
    asg/cnf/faulty_circuit.c
    asg/cnf/good_circuit.c
    asg/cnf/make_blocking_clause.c
    # lib/
    lib/lib.c
    # netlist/
    netlist/netlist.c
    # opt/
    opt/opt.c
)

# Debugビルド
add_executable(main_debug ${SOURCES})
target_compile_options(main_debug PRIVATE -g -O0)
target_include_directories(main_debug PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/asg
    ${CMAKE_SOURCE_DIR}/asg/cnf
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/netlist
    ${CMAKE_SOURCE_DIR}/opt
    /usr/local/include
)
target_link_libraries(main_debug cudd gmp m cadical stdc++)

# Releaseビルド
add_executable(main_release ${SOURCES})
target_compile_options(main_release PRIVATE -O2)
target_include_directories(main_release PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/asg
    ${CMAKE_SOURCE_DIR}/asg/cnf
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/netlist
    ${CMAKE_SOURCE_DIR}/opt
    /usr/local/include
)
target_link_libraries(main_release cudd gmp m cadical stdc++)