#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <string>
#include <vector>

using namespace llvm;
using namespace llvm::orc;

using MathFunc = int(*)();

int main(int argc, char *argv[]) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    auto JIT =  LLJITBuilder().create();

    std::string ErrMsg;
    if (sys::DynamicLibrary::LoadLibraryPermanently("./libtest_mvcc_lib.so", &ErrMsg)) {
        outs() << "Failed to load library: " + ErrMsg << "\n";
    }

    // 添加动态库到搜索路径
    (**JIT).getMainJITDylib().addGenerator(
        cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
            (**JIT).getDataLayout().getGlobalPrefix())));

    // 查找并执行函数
    auto AddSymbol = JITEvaluatedSymbol((**JIT).lookup("test")->getValue(), JITSymbolFlags::Exported);
    auto Add = (MathFunc)(AddSymbol.getAddress());

    Add();

    return 0;
}