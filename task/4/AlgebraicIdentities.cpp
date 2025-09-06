#include "AlgebraicIdentities.hpp"

using namespace llvm;

PreservedAnalyses
AlgebraicIdentities::run(Module& mod, ModuleAnalysisManager& mam)
{
  int reductionCount = 0;

  // 遍历所有函数
  for (auto& func : mod) {
    // 遍历每个基本块
    for (auto& bb : func) {
      std::vector<Instruction*> toErase;
      // 遍历基本块的指令
      for (auto& inst : bb) {
        if (auto binOp = dyn_cast<BinaryOperator>(&inst)) {
          // 获取操作数
          Value* lhs = binOp->getOperand(0);
          Value* rhs = binOp->getOperand(1);

          // 尝试将操作数转换为常整数
          auto constLhs = dyn_cast<ConstantInt>(lhs);
          auto constRhs = dyn_cast<ConstantInt>(rhs);

          switch (binOp->getOpcode()) {
            case Instruction::Add:
              // x + 0 = x
              if (constRhs && constRhs->isZero()) {
                binOp->replaceAllUsesWith(lhs);
                toErase.push_back(binOp);
                ++reductionCount;
              } else if (constLhs && constLhs->isZero()) {
                binOp->replaceAllUsesWith(rhs);
                toErase.push_back(binOp);
                ++reductionCount;
              }
              break;

            case Instruction::Sub:
              // x - 0 = x
              if (constRhs && constRhs->isZero()) {
                binOp->replaceAllUsesWith(lhs);
                toErase.push_back(binOp);
                ++reductionCount;
              }
              break;

            case Instruction::Mul:
              // x * 1 = x
              if (constRhs && constRhs->equalsInt(1)) {
                binOp->replaceAllUsesWith(lhs);
                toErase.push_back(binOp);
                ++reductionCount;
              } else if (constLhs && constLhs->equalsInt(1)) {
                binOp->replaceAllUsesWith(rhs);
                toErase.push_back(binOp);
                ++reductionCount;
              }
              // x * 0 = 0
              else if (constRhs && constRhs->isZero()) {
                binOp->replaceAllUsesWith(constRhs);
                toErase.push_back(binOp);
                ++reductionCount;
              } else if (constLhs && constLhs->isZero()) {
                binOp->replaceAllUsesWith(constLhs);
                toErase.push_back(binOp);
                ++reductionCount;
              }
              break;

            case Instruction::UDiv:
            case Instruction::SDiv:
              // x / 1 = x
              if (constRhs && constRhs->equalsInt(1)) {
                binOp->replaceAllUsesWith(lhs);
                toErase.push_back(binOp);
                ++reductionCount;
              }
              break;

            case Instruction::URem:
            case Instruction::SRem:
              // x % 1 = 0
              if (constRhs && constRhs->equalsInt(1)) {
                binOp->replaceAllUsesWith(
                  ConstantInt::get(binOp->getType(), 0));
                toErase.push_back(binOp);
                ++reductionCount;
              }
              break;

            default:
              break;
          }
        }
      }
      // 删除被替换的指令
      for (auto* inst : toErase) {
        inst->eraseFromParent();
      }
    }
  }

  mOut << "AlgebraicIdentities running...\nTo eliminate " << reductionCount
       << " instructions\n";
  return PreservedAnalyses::all();
}
