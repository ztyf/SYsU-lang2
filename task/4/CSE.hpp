#pragma once

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
#include <vector>

class CommonSubexpressionElimination
  : public llvm::PassInfoMixin<CommonSubexpressionElimination>
{
public:
  explicit CommonSubexpressionElimination(llvm::raw_ostream& out)
    : mOut(out)
  {
  }

  llvm::PreservedAnalyses run(llvm::Module& M,
                              llvm::ModuleAnalysisManager& MAM);

private:
  llvm::raw_ostream& mOut;
};
