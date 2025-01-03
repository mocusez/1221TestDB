// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "mlir/ExecutionEngine/OptUtils.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Target/LLVMIR/Export.h"

#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"

#include "Hello/HelloDialect.h"
#include "Hello/HelloPasses.h"
#include "Hello/HelloOps.h"

namespace cl = llvm::cl;
static cl::opt<std::string> inputFilename(cl::Positional,
                                          cl::desc("<input hello file>"),
                                          cl::init("-"),
                                          cl::value_desc("filename"));

int dumpLLVMIR(mlir::ModuleOp module) {
  mlir::registerBuiltinDialectTranslation(*module->getContext());
  mlir::registerLLVMDialectTranslation(*module->getContext());
  // Convert the module to LLVM IR in a new LLVM IR context.

  llvm::LLVMContext llvmContext;
  auto llvmModule = mlir::translateModuleToLLVMIR(module, llvmContext);
  if (!llvmModule) {
    llvm::errs() << "Failed to emit LLVM IR\n";
    return -1;
  }

  // Initialize LLVM targets.
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
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
  ;
  //  mlir::ExecutionEngine::setupTargetTriple(llvmModule.get());

  // Optionally run an optimization pipeline over the llvm module.
  auto optPipeline = mlir::makeOptimizingTransformer(0, 0, nullptr);
  if (auto err = optPipeline(llvmModule.get())) {
    llvm::errs() << "Failed to optimize LLVM IR " << err << "\n";
    return -1;
  }
  // llvm::outs() << *llvmModule << "\n";

  llvm::ExitOnError ExitOnErr;
  llvm::SMDiagnostic Err;

  auto J = ExitOnErr(llvm::orc::LLJITBuilder().create());
  std::string ErrMsg;
  if (llvm::sys::DynamicLibrary::LoadLibraryPermanently("./libtest_mvcc_lib.so", &ErrMsg)) {
      llvm::outs() << "Failed to load library: " + ErrMsg << "\n";
  }
  ExitOnErr(J->addIRModule(llvm::orc::ThreadSafeModule(std::move(llvmModule), std::make_unique<llvm::LLVMContext>())));

  auto MainSymbol = ExitOnErr(J->lookup("main"));
  auto normalJIT = [&MainSymbol](){
      auto *main = MainSymbol.toPtr<int()>();
      main();
  };
  normalJIT();

  return 0;
}

int loadMLIR(mlir::MLIRContext &context,
             mlir::OwningOpRef<mlir::ModuleOp> &module) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr =
      llvm::MemoryBuffer::getFileOrSTDIN(inputFilename);
  if (std::error_code ec = fileOrErr.getError()) {
    llvm::errs() << "Could not open input file: " << ec.message() << "\n";
    return -1;
  }

  llvm::SourceMgr sourceMgr;
  sourceMgr.AddNewSourceBuffer(std::move(*fileOrErr), llvm::SMLoc());
  module = mlir::parseSourceFile<mlir::ModuleOp>(sourceMgr, &context);
  if (!module) {
    llvm::errs() << "Error can't load file " << inputFilename << "\n";
    return 3;
  }
  return 0;
}

int loadAndProcessMLIR(mlir::MLIRContext &context,
                       mlir::OwningOpRef<mlir::ModuleOp> &module) {
  // if (int error = loadMLIR(context, module)) {
  //   return error;
  // }

  // Register passes to be applied in this compile process
  mlir::PassManager passManager(&context);
  if (mlir::failed(mlir::applyPassManagerCLOptions(passManager)))
    return 4;

  passManager.addPass(hello::createLowerToAffinePass());
  passManager.addPass(hello::createLowerToLLVMPass());

  if (mlir::failed(passManager.run(*module))) {
    return 4;
  }

  return 0;
}

int runJit(mlir::ModuleOp module) {
  // Initialize LLVM targets.
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  // Register the translation from MLIR to LLVM IR, which must happen before we
  // can JIT-compile.
  mlir::registerLLVMDialectTranslation(*module->getContext());

  // An optimization pipeline to use within the execution engine.
  auto optPipeline = mlir::makeOptimizingTransformer(0, /*sizeLevel=*/0,
                                                     /*targetMachine=*/nullptr);

  // Create an MLIR execution engine. The execution engine eagerly JIT-compiles
  // the module.
  mlir::ExecutionEngineOptions engineOptions;
  engineOptions.transformer = optPipeline;
  auto maybeEngine = mlir::ExecutionEngine::create(module, engineOptions);
  assert(maybeEngine && "failed to construct an execution engine");
  auto &engine = maybeEngine.get();

  // Invoke the JIT-compiled function.
  auto invocationResult = engine->invokePacked("main");
  if (invocationResult) {
    llvm::errs() << "JIT invocation failed\n";
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  mlir::registerMLIRContextCLOptions();
  mlir::registerPassManagerCLOptions();

  cl::ParseCommandLineOptions(argc, argv, "Hello compiler\n");
  mlir::MLIRContext context;
  

    // Register dialects
    context.getOrLoadDialect<hello::HelloDialect>();
    context.getOrLoadDialect<mlir::func::FuncDialect>(); 
    context.getOrLoadDialect<mlir::arith::ArithDialect>();
    context.getOrLoadDialect<mlir::LLVM::LLVMDialect>();

    mlir::OpBuilder builder(&context);
    mlir::OwningOpRef<mlir::ModuleOp> module = mlir::ModuleOp::create(builder.getUnknownLoc());

    // Create function returning i32
    auto i32Type = builder.getI32Type();
    mlir::Location loc = builder.getUnknownLoc();
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

    builder.create<hello::TestOp>(builder.getUnknownLoc());

    builder.create<mlir::arith::ConstantOp>(
        builder.getUnknownLoc(),
        builder.getI32IntegerAttr(0)
    );

    auto lhsAttr = builder.getI32IntegerAttr(50);
    auto rhsAttr = builder.getI32IntegerAttr(70);
    auto lhs = builder.create<mlir::arith::ConstantOp>(loc, lhsAttr);
    auto rhs = builder.create<mlir::arith::ConstantOp>(loc, rhsAttr);
    auto retVal = builder.create<hello::AddOp>(loc, i32Type,lhs, rhs);

    builder.create<mlir::func::ReturnOp>(
      builder.getUnknownLoc(), 
      mlir::ValueRange{retVal});

    module->push_back(runMlir);
    module->push_back(mainFunc);
  
  
  // module->print(llvm::outs());
  // llvm::outs() << "==================" <<"\n";
  if (int error = loadAndProcessMLIR(context, module)) {
    return error;
  }


  dumpLLVMIR(*module);
  // runJit(*module);
  // module->print(llvm::outs());
  return 0;
}
