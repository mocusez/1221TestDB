#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Row.h"

using namespace llvm;
using namespace llvm::orc;

ExitOnError ExitOnErr;

struct ModuleHandle {
    ResourceTrackerSP Tracker;
    std::unique_ptr<LLVMContext> Context;
    std::string Name;
};

Expected<ModuleHandle> loadModule(std::unique_ptr<LLJIT> &J, const std::string &IRFile) {
    auto Context = std::make_unique<LLVMContext>();
    SMDiagnostic Err;
    
    auto M = parseIRFile(IRFile, Err, *Context);
    if (!M) {
        return createStringError(inconvertibleErrorCode(),
            "Error loading file: " + Err.getMessage().str());
    }
    
    auto RT = J->getMainJITDylib().createResourceTracker();
    if (auto Err = J->addIRModule(RT, 
        ThreadSafeModule(std::move(M), std::move(Context)))) {
        return std::move(Err);
    }
    
    return ModuleHandle{RT, std::make_unique<LLVMContext>(), IRFile};
}

Expected<int> runModule(std::unique_ptr<LLJIT> &J, const ModuleHandle &Handle) {
    auto MainSymbol = J->lookup("jitMain");
    if (!MainSymbol)
        return MainSymbol.takeError();

    std::vector<Row> data = {
        {1, 10},
        {2, 20},
        {3, 30}
    };
        
    auto *MainFn = MainSymbol->toPtr<int(std::vector<Row>)>();
    return MainFn(data);
}

Error cleanupModule(ModuleHandle &Handle) {
    if (auto Err = Handle.Tracker->remove())
        return Err;
    Handle.Context.reset();
    return Error::success();
}

int main(int argc, char *argv[]) {
    // 初始化LLVM
    InitLLVM X(argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();  // Add ASM parser initialization
    
    // Set up target machine
    auto JTMB = ExitOnErr(JITTargetMachineBuilder::detectHost());
    auto TM = ExitOnErr(JTMB.createTargetMachine());

    // 创建LLVM上下文
    LLVMContext Context;
    SMDiagnostic Err;

    std::optional<ModuleHandle> Handle = std::nullopt;
    auto J = ExitOnErr(LLJITBuilder().create());

    auto loadIR = [&Handle,&J](const std::string& filename){
        if (!(Handle.has_value())){
            Handle.emplace(ExitOnErr(loadModule(J, filename)));
            outs() << filename << " Load Complete!\n";
        } else {
            outs() << "Wrong Load!\n";
        }
    };
    auto clean = [&Handle](bool isInProcess=true){
        if (Handle.has_value()){
            ExitOnErr(cleanupModule(*Handle));
            Handle.reset();
            outs() << "Module is Clean!\n";
        } else if(isInProcess) {
            outs() << "Wrong Clean!\n";
        }
    };

    std::string cmd;
    outs() << "JIT> ";
    while (std::getline(std::cin, cmd)) {
        if (cmd == "exit") break;
        if (cmd == "loada") loadIR("test_mvcc.ll");
        if (cmd == "loadb") loadIR("test_opl.ll");
        if (cmd == "run"){
            if (Handle.has_value()){
                auto Result = runModule(J, *Handle);
                outs() << "Result: " << *Result << "\n";
            } else {
                outs() << "Wrong run! \n";
            }
        }
        if (cmd == "clean") clean();
        outs() << "JIT> ";
    }

    clean(false);
    outs() << "Good Bye!\n";
    return 0;
}
