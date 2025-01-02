cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(QueryEnginePushModel VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")



# 添加头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 创建可执行文件
add_executable(test_mvcc test/test_mvcc_module.cpp test/test_mvcc_run.cpp)
add_library(test_mvcc_lib SHARED test/test_mvcc_module.cpp)

add_executable(test_simd src/simd.cpp test/test_simd.cpp)
add_executable(test_opl test/test_opl.cpp)

set(LLVM_DIR /usr/lib/llvm-18/lib/cmake/llvm/)
find_package(zstd REQUIRED)
find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(llvm_libs
    core
    support
    irreader
    executionengine
    interpreter
    mcjit
    native
    orcjit
)
add_executable(jit jit_run.cpp)
target_link_libraries(jit ${llvm_libs})

add_executable(jitSwitch test/jit_switch_run.cpp)
target_link_libraries(jitSwitch ${llvm_libs})

set(MLIR_DIR /usr/lib/llvm-18/lib/cmake/mlir/)
find_package(MLIR REQUIRED CONFIG)
set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
include(TableGen)
include(AddLLVM)
include(AddMLIR)
get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)

add_executable(mlir_run mlir_run.cpp src/LowerToLLVM.cpp)
target_link_libraries(mlir_run
  PRIVATE
  ${dialect_libs}
  ${conversion_libs}
    MLIRSupport
    MLIRIR
    MLIRFuncDialect
    MLIRArithDialect
    MLIRVectorDialect
    MLIRLLVMDialect
    MLIRPass
    MLIRTransforms

    MLIRBuiltinToLLVMIRTranslation
    MLIRExecutionEngine
    )
