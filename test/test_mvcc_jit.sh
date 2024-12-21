# clang++-18 -S -std=c++17 -emit-llvm -o test_mvcc.ll test_mvcc.cpp -I../include
# lli-18 test_mvcc.ll
../build/test_mvcc test_mvcc.ll