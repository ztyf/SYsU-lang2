#include "DSE.hpp"

using namespace llvm;

PreservedAnalyses
DeadstorageElimination::run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM)
{
  int removeCount = 0;

  for (llvm::Function& F : M) {
    for (llvm::BasicBlock& BB : F) {
      std::set<llvm::Instruction*> instrToErase;
      for (auto BI = BB.rbegin(); BI != BB.rend(); ++BI) {
        llvm::Instruction& I = *BI;
        if (llvm::isa<llvm::AllocaInst>(I)) {
          auto* alloc = llvm::cast<llvm::AllocaInst>(&I);
          if (alloc->use_empty())
            instrToErase.insert(&I);
        } else if (llvm::isa<llvm::StoreInst>(I)) {
          auto* store = llvm::cast<llvm::StoreInst>(&I);
          if (store->getPointerOperand()->hasNUsesOrMore(2) ||
              llvm::isa<llvm::GetElementPtrInst>(store->getPointerOperand()))
            continue;

          instrToErase.insert(&I);
          auto* toDeleteValueUse = store->getValueOperand();
          auto* toDeleteDesUse = store->getPointerOperand();

          std::deque<llvm::Value*> queue;
          if (auto* valInstr =
                llvm::dyn_cast<llvm::Instruction>(toDeleteValueUse))
            if (valInstr->hasOneUse())
              if (llvm::isa<llvm::CallInst>(valInstr))
                queue.push_back(toDeleteValueUse);
              else
                queue.push_back(toDeleteValueUse);

          while (!queue.empty()) {
            auto* val = queue.front();
            queue.pop_front();
            for (auto UI = val->use_begin(), UE = val->use_end(); UI != UE;) {
              llvm::Use& U = *UI;
              ++UI;
              auto* user = llvm::cast<llvm::Instruction>(U.getUser());
              if (user->hasNUsesOrMore(2))
                continue;
              queue.push_back(user);
              instrToErase.insert(user);
              U.set(nullptr);
            }
          }
        }
      }
      for (auto* instr : instrToErase) {
        instr->eraseFromParent();
        removeCount++;
      }
    }
  }

  mOut << "[frontend opt]: deadStorageElimination removed " << removeCount
       << " instructions.\n";
  return llvm::PreservedAnalyses::all();
}
