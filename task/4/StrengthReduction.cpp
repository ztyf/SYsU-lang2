#include "StrengthReduction.hpp"

using namespace llvm;

PreservedAnalyses
StrengthReduction::run(Module& mod, ModuleAnalysisManager& mam)
{
  int reductionCount = 0;

  // 遍历所有函数
  for (auto& func : mod) {
    // 遍历每个基本块
    for (auto& bb : func) {
      std::vector<Instruction*> toErase;
      // 遍历基本块的指令
      for (auto& inst : bb) {
        if (auto TheInst = dyn_cast<BinaryOperator>(&inst)) {
          // 获取操作数
          Value* lhs = TheInst->getOperand(0);
          Value* rhs = TheInst->getOperand(1);
          auto constRhs = dyn_cast<ConstantInt>(rhs);

          // 判断右操作数是否是2的幂
          bool isPower2 = false;
          if (constRhs) {
            int64_t n = constRhs->getSExtValue();
            if (n > 0 && (n & (n - 1)) == 0)
              isPower2 = true;
          }

          if (isPower2) {
            if (TheInst->getOpcode() == Instruction::Mul) {
              // 替换乘法指令的使用
              TheInst->replaceAllUsesWith(BinaryOperator::CreateShl(
                lhs,
                ConstantInt::get(lhs->getType(),
                                 Log2_64(constRhs->getZExtValue())),
                "shl.reduction",
                TheInst));
              toErase.push_back(TheInst);
              ++reductionCount;
            } else if (TheInst->getOpcode() == Instruction::UDiv) {
              // 替换无符号除法指令的使用
              TheInst->replaceAllUsesWith(BinaryOperator::CreateLShr(
                lhs,
                ConstantInt::get(lhs->getType(),
                                 Log2_64(constRhs->getZExtValue())),
                "lshr.reduction",
                TheInst));
              toErase.push_back(TheInst);
              ++reductionCount;
            } else if (TheInst->getOpcode() == Instruction::URem) {
              // 替换取余指令的使用
              IRBuilder<> builder(TheInst);
              Value* div = builder.CreateLShr(
                lhs,
                ConstantInt::get(lhs->getType(),
                                 Log2_64(constRhs->getZExtValue())),
                "div.reduction");
              Value* mul = builder.CreateMul(div, constRhs, "mul.reduction");
              Value* sub = builder.CreateSub(lhs, mul, "sub.reduction");
              TheInst->replaceAllUsesWith(sub);
              toErase.push_back(TheInst);
              ++reductionCount;
            }
          }
        }
      }
      // 删除被替换的指令
      for (auto* inst : toErase) {
        inst->eraseFromParent();
      }
    }
  }

  mOut << "StrengthReduction running...\nTo eliminate " << reductionCount
       << " instructions\n";
  return PreservedAnalyses::all();
}
