cmake_minimum_required(VERSION 3.16)
project(fdp C)

set(CMAKE_C_STANDARD 11)

set(SOURCES
    src/main.c
    src/fdp/cnf/tpg_model.c
    src/fdp/cnf/normal_scope.c
    src/fdp/cnf/normal_scope_values.c
    src/fdp/cube_set.c
    src/fdp/bdd/probability.c
    src/fault/drop_detected.c
    src/fdp/cnf/essential_assignment.c
    src/fdp/analysis.c
    src/fdp/worker_pool.c
    src/io/fault_csv.c
    src/io/cube_text.c
    src/fdp/context.c
    src/fault/fault_list.c
    src/fault/target_fault.c
    src/fdp/cnf/detection_circuit.c
    src/fdp/cnf/faulty_circuit.c
    src/fdp/cnf/good_circuit.c
    src/common/memory.c
    src/io/file.c
    src/fault/hash.c
    src/netlist/traversal.c
    src/io/report.c
    src/netlist/netlist.c
    src/netlist/expand_tdf.c
    src/cli/options.c
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

include(CTest)
if(BUILD_TESTING)
    add_executable(target_order_test
        tests/target_order_test.c
        src/fault/target_fault.c
    )
    target_compile_options(target_order_test PRIVATE -fcommon -UNDEBUG)
    target_include_directories(target_order_test PRIVATE ${COMMON_INCLUDES})
    add_test(NAME target_order COMMAND target_order_test)

    add_executable(fdp_output_test
        tests/fdp_output_test.c
        src/fdp/bdd/probability.c
        src/io/fault_csv.c
        src/io/cube_text.c
    )
    target_compile_options(fdp_output_test PRIVATE -fcommon -UNDEBUG)
    target_include_directories(fdp_output_test PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(fdp_output_test cudd gmp m)
    add_test(NAME fdp_output COMMAND fdp_output_test)

    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME fault_equivalence COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/fault_equivalence_check.py
            --binary $<TARGET_FILE:main_release> --debug $<TARGET_FILE:main_debug>)
        add_test(NAME options_paths COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/options_paths_check.py
            --binary $<TARGET_FILE:main_release> --debug $<TARGET_FILE:main_debug>)
        add_test(NAME cube_output COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/cube_output_check.py
            --binary $<TARGET_FILE:main_release> --debug $<TARGET_FILE:main_debug>)
        add_test(NAME fault_progress COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/progress_check.py
            --binary $<TARGET_FILE:main_release> --debug $<TARGET_FILE:main_debug>
            --output ${CMAKE_BINARY_DIR}/progress_checks)
        set_tests_properties(fault_progress PROPERTIES TIMEOUT 90)
        add_test(NAME batch_runner COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/runner_check.py
            --binary $<TARGET_FILE:main_release>)
        add_test(NAME tdf_model COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/tdf_check.py
            --binary $<TARGET_FILE:main_release> --debug $<TARGET_FILE:main_debug>
            --output ${CMAKE_BINARY_DIR}/tdf_checks)
    endif()
endif()
