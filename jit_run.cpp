#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"

using namespace llvm;
using namespace llvm::orc;

ExitOnError ExitOnErr;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        errs() << "Usage: " << argv[0] << " <ir_file.ll>\n";
        return 1;
    }
    // 初始化LLVM
    InitLLVM X(argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // 创建LLVM上下文
    LLVMContext Context;
    SMDiagnostic Err;

    // 从.ll文件加载LLVM IR模块
    std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
    if (!M) {
        errs() << "Error loading file: " << Err.getMessage() << "\n";
        return 1;
    }
    
    // 创建JIT实例
    auto J = ExitOnErr(LLJITBuilder().create());

    // 将模块添加到JIT
    ExitOnErr(J->addIRModule(ThreadSafeModule(std::move(M), std::make_unique<LLVMContext>())));

    // 查找并执行函数
    auto MainSymbol = ExitOnErr(J->lookup("main"));
    auto *MainFn = MainSymbol.toPtr<int()>();
    MainFn();
    return 0;
}