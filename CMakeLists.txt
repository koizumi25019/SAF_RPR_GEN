cmake_minimum_required(VERSION 3.16)
project(fdp C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(FDP_BUILD_APP "Build FDP executables (requires the clasp library)" ON)

set(SOURCES
    src/main.c
    src/fdp/create_TPG_model.c
    src/fdp/normal_scope.c
    src/fdp/normal_scope_values.c
    src/fdp/cube_set.c
    src/fdp/cudd_wrapper.c
    src/fdp/drop_dete_fault.c
    src/fdp/essential_assignment.c
    src/fdp/fault_detection_prob.c
    src/fdp/fault_pool.c
    src/fdp/fault_result.c
    src/fdp/init.c
    src/fdp/read.c
    src/fdp/target_fault.c
    src/fdp/cnf/detection_circuit.c
    src/fdp/cnf/faulty_circuit.c
    src/fdp/cnf/good_circuit.c
    src/lib/lib.c
    src/netlist/netlist.c
    src/netlist/expand_tdf.c
    src/opt/opt.c
    src/fdp/sbxid/sbxid.c
)

set(TOOLS_DIR ${CMAKE_SOURCE_DIR}/external)

set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/fdp
    ${CMAKE_SOURCE_DIR}/src/fdp/cnf
    ${CMAKE_SOURCE_DIR}/src/lib
    ${CMAKE_SOURCE_DIR}/src/netlist
    ${CMAKE_SOURCE_DIR}/src/opt
    ${TOOLS_DIR}/cadical/src
    ${TOOLS_DIR}/cudd
    ${TOOLS_DIR}/cudd/cudd
)

link_directories(
    ${TOOLS_DIR}/cadical/build
    ${TOOLS_DIR}/cudd/cudd/.libs
)

if(FDP_BUILD_APP)
find_package(Clasp CONFIG QUIET)
if(NOT Clasp_FOUND OR NOT TARGET libclasp)
    message(FATAL_ERROR
        "SB-XID requires the clasp C++ library. See src/fdp/sbxid/README.md. "
        "Set -DCMAKE_PREFIX_PATH=<clasp-install> after installing clasp, or "
        "use -DFDP_BUILD_APP=OFF to build clasp-free tests only.")
endif()
find_package(Threads REQUIRED)
add_library(sbxid_clasp STATIC src/fdp/sbxid/clasp_bridge.cpp)
target_link_libraries(sbxid_clasp PRIVATE libclasp Threads::Threads)

# Debugビルド
add_executable(main_debug ${SOURCES})
target_compile_options(main_debug PRIVATE -g -O0 -DDEBUG -fcommon)

target_include_directories(main_debug PRIVATE ${COMMON_INCLUDES})
target_link_libraries(main_debug cadical cudd gmp m stdc++ sbxid_clasp)

# Releaseビルド
add_executable(main_release ${SOURCES})
target_compile_options(main_release PRIVATE -O3 -DNDEBUG -fcommon)
target_include_directories(main_release PRIVATE ${COMMON_INCLUDES})
target_link_libraries(main_release cadical cudd gmp m stdc++ sbxid_clasp)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(target_order_test
        tests/target_order_test.c
        src/fdp/target_fault.c
    )
    target_compile_options(target_order_test PRIVATE -fcommon -UNDEBUG)
    target_include_directories(target_order_test PRIVATE ${COMMON_INCLUDES})
    add_test(NAME target_order COMMAND target_order_test)

    add_executable(fdp_output_test
        tests/fdp_output_test.c
        src/fdp/cudd_wrapper.c
        src/fdp/fault_result.c
    )
    target_compile_options(fdp_output_test PRIVATE -fcommon -UNDEBUG)
    target_include_directories(fdp_output_test PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(fdp_output_test cudd gmp m)
    add_test(NAME fdp_output COMMAND fdp_output_test)

    # The solver double belongs only to this test, never to an FDP executable.
    add_executable(sbxid_core_test tests/sbxid_core_test.c src/fdp/sbxid/sbxid.c)
    target_compile_options(sbxid_core_test PRIVATE -fcommon -UNDEBUG -Wall -Wextra)
    target_include_directories(sbxid_core_test PRIVATE ${COMMON_INCLUDES})
    add_test(NAME sbxid_core COMMAND sbxid_core_test)

    add_executable(sbxid_fault_scope_test tests/sbxid_fault_scope_test.c src/fdp/sbxid/sbxid.c)
    target_compile_options(sbxid_fault_scope_test PRIVATE -fcommon -UNDEBUG -Wall -Wextra)
    target_include_directories(sbxid_fault_scope_test PRIVATE ${COMMON_INCLUDES})
    add_test(NAME sbxid_fault_scope COMMAND sbxid_fault_scope_test)

    add_executable(sbxid_pi_objective_test tests/sbxid_pi_objective_test.c src/fdp/sbxid/sbxid.c)
    target_compile_options(sbxid_pi_objective_test PRIVATE -fcommon -UNDEBUG -Wall -Wextra)
    target_include_directories(sbxid_pi_objective_test PRIVATE ${COMMON_INCLUDES})
    add_test(NAME sbxid_pi_objective COMMAND sbxid_pi_objective_test)

    add_executable(normal_scope_pattern_test tests/normal_scope_pattern_test.c
        src/fdp/normal_scope.c src/fdp/normal_scope_values.c)
    target_compile_options(normal_scope_pattern_test PRIVATE -fcommon -UNDEBUG)
    target_include_directories(normal_scope_pattern_test PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(normal_scope_pattern_test cadical stdc++ m)
    add_test(NAME normal_scope_pattern COMMAND normal_scope_pattern_test)

    if(FDP_BUILD_APP)
        add_executable(sbxid_pi_objective_benchmark EXCLUDE_FROM_ALL
            tests/sbxid_pi_objective_benchmark.c src/fdp/sbxid/sbxid.c
            src/fdp/normal_scope.c src/netlist/netlist.c)
        target_compile_options(sbxid_pi_objective_benchmark PRIVATE -O3 -fcommon -UNDEBUG)
        target_include_directories(sbxid_pi_objective_benchmark PRIVATE ${COMMON_INCLUDES})
        target_link_libraries(sbxid_pi_objective_benchmark PRIVATE sbxid_clasp)

        add_executable(clasp_bridge_test tests/clasp_bridge_test.cpp)
        target_compile_options(clasp_bridge_test PRIVATE -UNDEBUG)
        target_include_directories(clasp_bridge_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
        target_link_libraries(clasp_bridge_test PRIVATE sbxid_clasp)
        add_test(NAME clasp_bridge COMMAND clasp_bridge_test)
    endif()

    if(FDP_BUILD_APP)
        find_package(Python3 COMPONENTS Interpreter)
        if(Python3_Interpreter_FOUND)
            add_test(NAME sbxid_pi_objective_integration COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/tests/sbxid_pi_objective_check.py
                --binary $<TARGET_FILE:main_release> --debug $<TARGET_FILE:main_debug>
                --output ${CMAKE_BINARY_DIR}/sbxid_pi_objective_checks)
            add_test(NAME tdf_disabled COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/tests/tdf_disabled_check.py
                --binary $<TARGET_FILE:main_release> --debug $<TARGET_FILE:main_debug>
                --output ${CMAKE_BINARY_DIR}/tdf_disabled_checks)
        endif()
    endif()
endif()
