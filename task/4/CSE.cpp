#include "CSE.hpp"
#include <vector>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

bool impossibleToCSE(unsigned op) {
    return op == Instruction::Br || op == Instruction::Ret || op == Instruction::Alloca
        || op == Instruction::PHI || op == Instruction::ICmp || op == Instruction::FCmp;
}

bool maybeSameGEP(GetElementPtrInst* gep1, GetElementPtrInst* gep2) {
    if (!gep1 || !gep2)
        return false;

    if (gep1->getNumIndices() != gep2->getNumIndices() ||
        gep1->getPointerOperand() != gep2->getPointerOperand())
        return false;

    for (auto i = gep1->idx_begin(), j = gep2->idx_begin(); i != gep1->idx_end(); ++i, ++j) {
        if (*i != *j) {
            if (auto const1 = dyn_cast<ConstantInt>(*i), const2 = dyn_cast<ConstantInt>(*j);
                !const1 || !const2 || const1->getValue() != const2->getValue()) {
                return false;
            }
        }
    }

    return true;
}

PreservedAnalyses CommonSubexpressionElimination::run(Module& M, ModuleAnalysisManager& MAM) {
    int instrCount = 0;
    for (Function& F : M) {
        if (F.isDeclaration())
            continue;

        for (BasicBlock& BB : F) {
            std::vector<Instruction*> toErase;
            for (auto I = BB.begin(); I != BB.end(); ++I) {
                Instruction* Ia = &*I;
                if (impossibleToCSE(Ia->getOpcode()))
                    continue;

                for (auto J = std::next(I); J != BB.end(); ++J) {
                    Instruction* Ib = &*J;
                    if (Ia->getOpcode() == Ib->getOpcode()) {
                        if (auto* BinOpA = dyn_cast<BinaryOperator>(Ia)) {
                            auto* BinOpB = dyn_cast<BinaryOperator>(Ib);
                            if (BinOpB && BinOpA->getOpcode() == BinOpB->getOpcode() &&
                                ((BinOpA->getOperand(0) == BinOpB->getOperand(0) &&
                                  BinOpA->getOperand(1) == BinOpB->getOperand(1)) ||
                                 (BinOpA->getOperand(0) == BinOpB->getOperand(1) &&
                                  BinOpA->getOperand(1) == BinOpB->getOperand(0)))) {
                                Ib->replaceAllUsesWith(Ia);
                                toErase.push_back(Ib);
                                ++instrCount;
                            }
                        } else if (auto* LoadA = dyn_cast<LoadInst>(Ia)) {
                            auto* LoadB = dyn_cast<LoadInst>(Ib);
                            if (LoadB &&
                                LoadA->getPointerOperand() == LoadB->getPointerOperand()) {
                                LoadB->replaceAllUsesWith(LoadA);
                                toErase.push_back(Ib);
                                ++instrCount;
                            }
                        } else if (auto* StoreA = dyn_cast<StoreInst>(Ia)) {
                            auto* StoreB = dyn_cast<StoreInst>(Ib);
                            if (StoreB &&
                                (StoreA->getPointerOperand() == StoreB->getPointerOperand() ||
                                 maybeSameGEP(
                                   dyn_cast<GetElementPtrInst>(StoreA->getPointerOperand()),
                                   dyn_cast<GetElementPtrInst>(
                                     StoreB->getPointerOperand())))) {
                                toErase.push_back(Ia);
                                ++instrCount;
                                break;
                            }
                        }
                    } else if (auto* LoadA = dyn_cast<LoadInst>(Ia)) {
                        auto* StoreB = dyn_cast<StoreInst>(Ib);
                        if (StoreB &&
                            LoadA->getPointerOperand() == StoreB->getPointerOperand()) {
                            LoadA->replaceAllUsesWith(StoreB->getValueOperand());
                            toErase.push_back(Ia);
                            ++instrCount;
                            break;
                        }
                    } else if (auto* StoreA = dyn_cast<StoreInst>(Ia)) {
                        auto* LoadB = dyn_cast<LoadInst>(Ib);
                        if (LoadB &&
                            StoreA->getPointerOperand() == LoadB->getPointerOperand()) {
                            LoadB->replaceAllUsesWith(StoreA->getValueOperand());
                            toErase.push_back(Ib);
                            ++instrCount;
                        }
                    }
                }
            }

            // Now erase the instructions safely after the iteration is complete
            for (auto* Inst : toErase) {
                Inst->eraseFromParent();
            }
        }
    }
    outs() << "[frontend opt]: CSE eliminated " << instrCount << " instructions\n";
    return PreservedAnalyses::all();
}
