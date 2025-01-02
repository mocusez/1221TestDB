#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Value.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "mlir/ExecutionEngine/OptUtils.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IRReader/IRReader.h"

#include "Passes.h"

bool EnablePass = true;
bool EnableInlinePass = true && EnablePass;
bool EnableLLVMPass = true && EnablePass;
bool DumpLLVMIR = true;
bool enableOpt = true;
bool runJIT = true;

int dumpLLVMIR(mlir::ModuleOp module,bool enableJIT=false) {
    mlir::registerBuiltinDialectTranslation(*module->getContext());
    mlir::registerLLVMDialectTranslation(*module->getContext());

    // Convert the module to LLVM IR in a new LLVM IR context.
    llvm::LLVMContext llvmContext;
    auto llvmModule = mlir::translateModuleToLLVMIR(module, llvmContext);
    if (!llvmModule) {
        llvm::errs() << "Failed to emit LLVM IR\n";
        return -1;
    }

    llvm::ExitOnError ExitOnErr;
    llvm::SMDiagnostic Err;

  // Initialize LLVM targets.
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();


  // Create target machine and configure the LLVM Module
    auto tmBuilderOrError = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (!tmBuilderOrError) {
        llvm::errs() << "Could not create JITTargetMachineBuilder\n";
        return -1;
    }

    auto tmOrError = tmBuilderOrError->createTargetMachine();
    if (!tmOrError) {
        llvm::errs() << "Could not create TargetMachine\n";
        return -1;
    }
    mlir::ExecutionEngine::setupTargetTripleAndDataLayout(llvmModule.get(),
                                                        tmOrError.get().get());

  /// Optionally run an optimization pipeline over the llvm module.
    auto optPipeline = mlir::makeOptimizingTransformer(3 , /*sizeLevel=*/0,
        /*targetMachine=*/nullptr);
    if (auto err = optPipeline(llvmModule.get())) {
    llvm::errs() << "Failed to optimize LLVM IR " << err << "\n";
    return -1;
    }

    if(enableJIT){
        auto J = ExitOnErr(llvm::orc::LLJITBuilder().create());
        
        std::unique_ptr<llvm::Module> M = parseIRFile("test_mvcc.ll", Err, llvmContext);
        if (!M) {
            llvm::errs() << "Error loading file: " << Err.getMessage() << "\n";
            return 1;
        }
        ExitOnErr(J->addIRModule(llvm::orc::ThreadSafeModule(std::move(M), std::make_unique<llvm::LLVMContext>())));
        ExitOnErr(J->addIRModule(llvm::orc::ThreadSafeModule(std::move(llvmModule), std::make_unique<llvm::LLVMContext>())));

        auto MainSymbol = ExitOnErr(J->lookup("main"));
        auto normalJIT = [&MainSymbol](){
            auto *main = MainSymbol.toPtr<int()>();
            main();
        };
        normalJIT();
    } else{
        llvm::outs() << *llvmModule << "\n";
    }
    return 0;
}

int main() {
    mlir::MLIRContext context;

    // Register dialects
    context.loadDialect<mlir::func::FuncDialect>();
    context.loadDialect<mlir::arith::ArithDialect>();
    context.loadDialect<mlir::LLVM::LLVMDialect>();

    mlir::OpBuilder builder(&context);
    mlir::OwningOpRef<mlir::ModuleOp> module = mlir::ModuleOp::create(builder.getUnknownLoc());

    // Create function returning i32
    auto i32Type = builder.getI32Type();
    auto runType = mlir::LLVM::LLVMFunctionType::get(i32Type, {}, false);

    auto runMlir = builder.create<mlir::LLVM::LLVMFuncOp>(
        builder.getUnknownLoc(),
        "test",
        runType
    );

    auto mainType = builder.getFunctionType({}, {i32Type});
    auto mainFunc = builder.create<mlir::func::FuncOp>(
        builder.getUnknownLoc(),
        "main",
        mainType
    );

    auto entryBlock = mainFunc.addEntryBlock();
    builder.setInsertionPointToStart(entryBlock);

    auto callResult = builder.create<mlir::LLVM::CallOp>(
        builder.getUnknownLoc(),
        i32Type,
        "test",
        mlir::ValueRange{}
    );

    auto retVal = builder.create<mlir::arith::ConstantOp>(
        builder.getUnknownLoc(),
        builder.getI32IntegerAttr(0)
    );

    builder.create<mlir::func::ReturnOp>(
      builder.getUnknownLoc(), 
      mlir::ValueRange{retVal});

    module->push_back(runMlir);
    module->push_back(mainFunc);

    if(EnablePass){
        mlir::PassManager pm(&context);
        pm.addPass(mlir::createCSEPass());
        pm.addPass(mlir::createCanonicalizerPass());

        if(EnableInlinePass){
            pm.addPass(mlir::createInlinerPass());
        }
        
        if(EnableLLVMPass) {
            pm.addPass(mlir::toy::createLowerToLLVMPass());
        }

        if (mlir::failed(pm.run(*module))) {
            llvm::errs() << "Failed to lower to LLVM.\n";
            return 1;
        }
    }

    if(EnableLLVMPass){
        if(runJIT) dumpLLVMIR(*module,true);
        else if(DumpLLVMIR) dumpLLVMIR(*module);
    } else {
        module->print(llvm::outs());
    }
    return 0;
}
