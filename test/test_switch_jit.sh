#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Common variables
COMPILER="clang++-18"
STD_VERSION="-std=c++17"
INCLUDE_PATH="-I../include"
OUTPUT_DIR="build"

# Error handling
set -e

# Clean function
clean() {
    echo -e "${GREEN}Cleaning build directory...${NC}"
    rm -rf ${OUTPUT_DIR}
    mkdir -p ${OUTPUT_DIR}
}

# Build function
build() {
    local source=$1
    local output=$2
    echo -e "${GREEN}Building ${source}...${NC}"
    ${COMPILER} -S ${STD_VERSION} -emit-llvm -o ${OUTPUT_DIR}/${output} ${source} ${INCLUDE_PATH}
}

test(){
    cd build
    ../../build/jitSwitch
}

# Main execution
main() {
    clean
    build "test_mvcc.cpp" "test_mvcc.ll"
    build "test_opl.cpp" "test_opl.ll"
    echo -e "${GREEN}Build completed successfully!${NC}"
    test
}

# Execute main function
main
