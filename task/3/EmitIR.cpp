#include "EmitIR.hpp"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <vector>

#define self (*this)
llvm::BasicBlock* mCurrentLoopEnd;
llvm::BasicBlock* mCurrentLoopCond;
using namespace asg;

EmitIR::EmitIR(Obj::Mgr& mgr, llvm::LLVMContext& ctx, llvm::StringRef mid)
  : mMgr(mgr)                                         // 对象管理器
  , mMod(mid, ctx)                                    // 创建一个LLVM模块
  , mCtx(ctx)                                         // LLVM上下文
  , mIntTy(llvm::Type::getInt32Ty(ctx))               // 整型
  , mCurIrb(std::make_unique<llvm::IRBuilder<>>(ctx)) // 当前IR构建器
  , mCtorTy(
      llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), false)) // 函数类型
  // , mCurrentLoopEnd(nullptr)
  // , mCurrentLoopCond(nullptr)
  , mCurFunc(nullptr) // 当前函数
{
}

llvm::Module&
EmitIR::operator()(asg::TranslationUnit* tu) // inital function
{
  for (auto&& i : tu->decls)
    self(i);
  return mMod;
}

//==============================================================================
// 类型
//==============================================================================

llvm::Type*
EmitIR::operator()(const Type* type)
{
  // mCtx  mMod
  auto& irb = *mCurIrb;
  if (type->texp == nullptr) {
    switch (type->spec) {
      case Type::Spec::kInt:
        return llvm::Type::getInt32Ty(mCtx);
      // TODO: 在此添加对更多基础类型的处理
      case Type::Spec::kVoid:
        return llvm::Type::getVoidTy(mCtx);
      case Type::Spec::kChar:
        return llvm::Type::getInt8Ty(mCtx);
      default:
        ABORT();
    }
  }

  Type subt;
  subt.spec = type->spec;
  subt.qual = type->qual;
  subt.texp = type->texp->sub;

  // TODO: 在此添加对指针类型、数组类型和函数类型的处理
  if (auto p = type->texp->dcst<FunctionType>()) {
    std::vector<llvm::Type*> pty;
    for (auto&& i : p->params) {
      pty.push_back(self(i));
    }
    return llvm::FunctionType::get(self(&subt), pty, false);
  }

  if (auto p = type->texp->dcst<ArrayType>()) {
    return llvm::ArrayType::get(self(&subt), p->len);
  }

  if (auto p = type->texp->dcst<PointerType>()) {
    return llvm::PointerType::get(llvm::Type::getInt32Ty(mCtx), 0);
  }
  ABORT();
}

//==============================================================================
// 表达式
//==============================================================================

llvm::Value*
EmitIR::operator()(Expr* obj)
{
  // TODO: 在此添加对更多表达式处理的跳转
  if (auto p = obj->dcst<IntegerLiteral>())
    return self(p);

  if (auto p = obj->dcst<DeclRefExpr>())
    return self(p);

  if (auto p = obj->dcst<ImplicitCastExpr>())
    return self(p);

  if (auto p = obj->dcst<BinaryExpr>())
    return self(p);

  if (auto p = obj->dcst<UnaryExpr>())
    return self(p);

  if (auto p = obj->dcst<ParenExpr>())
    return self(p);

  if (auto p = obj->dcst<CallExpr>())
    return self(p);
  // if (auto p = obj->dcst<InitListExpr>())
  //   return self(p);

  // if (auto p = obj->dcst<ImplicitInitExpr>())
  //   return self(p);

  ABORT();
}

llvm::Constant*
EmitIR::operator()(IntegerLiteral* obj)
{
  return llvm::ConstantInt::get(self(obj->type), obj->val);
}

// // TODO: 在此添加对更多表达式类型的处理

llvm::Value*
EmitIR::operator()(DeclRefExpr* obj)
{
  // 在LLVM IR层面，左值体现为返回指向值的指针
  // 在ImplicitCastExpr::kLValueToRValue中发射load指令从而变成右值
  llvm::Value* ret = nullptr;
  auto& irb = *mCurIrb;
  ret = mMod.getGlobalVariable(obj->decl->name);
  return reinterpret_cast<llvm::Value*>(obj->decl->any);
}

llvm::Value*
EmitIR::operator()(ImplicitCastExpr* obj)
{
  auto sub = self(obj->sub); // 子表达式

  auto& irb = *mCurIrb;
  switch (obj->kind) { // 类型转换
    case ImplicitCastExpr::
      kLValueToRValue: { // 左值转右值(这是第一种，后面还要加其他的case)
      auto ty = self(obj->sub->type);
      auto loadVal = irb.CreateLoad(ty, sub);
      return loadVal;
    }
    case ImplicitCastExpr::kArrayToPointerDecay: { // 数组转指针衰变
      return sub;
    }

    default:
      ABORT();
  }
}

llvm::Value*
EmitIR::operator()(BinaryExpr* obj)
{
  llvm::Value *lftVal, *rhtVal;

  lftVal = self(obj->lft);
  auto& irb = *mCurIrb;
  rhtVal = self(obj->rht);

  switch (obj->op) {
    case BinaryExpr::kAdd:
      return irb.CreateAdd(lftVal, rhtVal);
    case BinaryExpr::kSub:
      return irb.CreateSub(lftVal, rhtVal);
    case BinaryExpr::kMul:
      return irb.CreateMul(lftVal, rhtVal);
    case BinaryExpr::kDiv:
      return irb.CreateSDiv(lftVal, rhtVal);
    case BinaryExpr::kMod:
      return irb.CreateSRem(lftVal, rhtVal);
    case BinaryExpr::kGt:
      return irb.CreateICmpSGT(lftVal, rhtVal);
    case BinaryExpr::kGe:
      return irb.CreateICmpSGE(lftVal, rhtVal);
    case BinaryExpr::kLt:
      return irb.CreateICmpSLT(lftVal, rhtVal);
    case BinaryExpr::kLe:
      return irb.CreateICmpSLE(lftVal, rhtVal);
    case BinaryExpr::kEq:
      return irb.CreateICmpEQ(lftVal, rhtVal);
    case BinaryExpr::kNe:
      return irb.CreateICmpNE(lftVal, rhtVal);
    case BinaryExpr::kAssign: {
      if (auto p = obj->lft->dcst<DeclRefExpr>()) {
        irb.CreateStore(rhtVal, lftVal);
        return rhtVal;
      }
    }
    case BinaryExpr::kIndex: {
      return irb.CreateInBoundsGEP(self(obj->type), lftVal, rhtVal);
    }
    case BinaryExpr::kINVALID: {
      ABORT();
    }
    case BinaryExpr::kComma: {
      return rhtVal;
    }
    case BinaryExpr::kAnd: {
      llvm::Function* func = irb.GetInsertBlock()->getParent();
      llvm::BasicBlock* lhsTrueBlock =
        llvm::BasicBlock::Create(mCtx, "land.rhs", func);
      llvm::BasicBlock* landEndBlock =
        llvm::BasicBlock::Create(mCtx, "land.end", func);

      llvm::Value* lftCond = irb.CreateICmpNE(lftVal, llvm::Constant::getNullValue(lftVal->getType()));
      irb.CreateCondBr(lftCond, lhsTrueBlock, landEndBlock);

      irb.SetInsertPoint(lhsTrueBlock);
      
      llvm::Value* rhtCond = irb.CreateICmpNE(rhtVal, llvm::Constant::getNullValue(rhtVal->getType()));
      irb.CreateBr(landEndBlock);

      irb.SetInsertPoint(landEndBlock);
      auto phi = irb.CreateAnd(lftCond, rhtCond, "land.merge");

      return phi;
    }
    case BinaryExpr::kOr: {
      llvm::Function* func = irb.GetInsertBlock()->getParent();
      llvm::BasicBlock* lhsFalseBlock =
        llvm::BasicBlock::Create(mCtx, "lor.rhs", func);
      llvm::BasicBlock* lorEndBlock =
        llvm::BasicBlock::Create(mCtx, "lor.end", func);

      llvm::Value* lftCond = irb.CreateICmpNE(lftVal, llvm::Constant::getNullValue(lftVal->getType()));
      irb.CreateCondBr(lftCond, lorEndBlock, lhsFalseBlock);

      irb.SetInsertPoint(lhsFalseBlock);
      llvm::Value* rhtCond = irb.CreateICmpNE(rhtVal, llvm::Constant::getNullValue(rhtVal->getType()));
      irb.CreateBr(lorEndBlock);

      irb.SetInsertPoint(lorEndBlock);
      auto phi = irb.CreateOr(lftCond, rhtCond, "lor.merge");

      return phi;
    }
    default:
      ABORT();
  }
}

llvm::Value*
EmitIR::operator()(UnaryExpr* obj) 
{
  auto& irb = *mCurIrb;
  llvm::Value* subVal = self(obj->sub);

  switch (obj->op) {
    case UnaryExpr::kNeg:
      return irb.CreateNeg(subVal);
    case UnaryExpr::kPos:
      return subVal;
    case UnaryExpr::kNot:{
      return mCurIrb->CreateICmpEQ(subVal, llvm::Constant::getNullValue(subVal->getType()));
    }
    default:
      ABORT();
  }

}


llvm::Value*
EmitIR::operator()(ParenExpr* obj)
{
  return self(obj->sub);
}

// struct CallExpr : Expr
// {
//   Expr* head{ nullptr };
//   std::vector<Expr*> args;

// private:
//   void __mark__(Mark mark) override;
// };
// if (auto p = type->texp->dcst<FunctionType>()) {
//     std::vector<llvm::Type*> pty;
//     for (auto&& i : p->params) {
//       pty.push_back(self(i));
//     }
//     return llvm::FunctionType::get(self(&subt), pty, false);

//   }

llvm::Value*
EmitIR::operator()(CallExpr* obj)
{
  /// llvm::Module 的成员函数
  /// 通过函数名字，在 llvm::Module 的符号表找到对应的函数
  /// Name：要调用的函数的名字
  // Function *getFunction(StringRef Name) const;
  // // 利用 llvm::IRBuilder 创建 call 指令
  // TheBuilder.CreateCall(func);
  //   auto p = obj->dcst<CallExpr>();

  std::vector<llvm::Value*> arguments;
  for (Expr* argExpr : obj->args) { // 遍历参数
    llvm::Value* argValue = (*this)(argExpr);
    arguments.push_back(argValue);
  }
  llvm::Function* func = mMod.getFunction(obj->head->dcst<ImplicitCastExpr>()
                                            ->sub->dcst<DeclRefExpr>()
                                            ->decl->name); // 获取函数名
  /// llvm::IRBuilder的成员函数
  /// 创建 call 指令
  /// Callee：    要调用的函数
  /// Args：    调用函数时传入的实参列表，可不传参
  // CallInst *CreateCall(FunctionCallee Callee, ArrayRef<Value *> Args = None,
  //                    const Twine &Name = "", MDNode *FPMathTag = nullptr);
  // llvm::IRBuilder<> builder(TheContext);
  // llvm::Value* callInst = builder.CreateCall(func, arguments);
  /// 利用 llvm::IRBuilder 创建 call 指令
  auto& irb = *mCurIrb;
  if (arguments.size() == 0)
    return irb.CreateCall(func);
  else
    return irb.CreateCall(func, arguments);
}

//==============================================================================
// 语句
//==============================================================================

void
EmitIR::operator()(Stmt* obj)
{
  // TODO: 在此添加对更多Stmt类型的处理的跳转

  if (auto p = obj->dcst<CompoundStmt>())
    return self(p);

  if (auto p = obj->dcst<ReturnStmt>())
    return self(p);

  if (auto p = obj->dcst<DeclStmt>())
    return self(p);

  if (auto p = obj->dcst<ExprStmt>())
    return self(p);

  if (auto p = obj->dcst<IfStmt>())
    return self(p);

  if (auto p = obj->dcst<WhileStmt>())
    return self(p);

  if (auto p = obj->dcst<BreakStmt>())
    return self(p);

  if (auto p = obj->dcst<ContinueStmt>())
    return self(p);

  if(auto p = obj->dcst<NullStmt>())
    return self(p);

  ABORT();
}

// TODO: 在此添加对更多Stmt类型的处理

void
EmitIR::operator()(CompoundStmt* obj)
{
  // TODO: 可以在此添加对符号重名的处理
  for (auto&& stmt : obj->subs)
    self(stmt);
}

void
EmitIR::operator()(ReturnStmt* obj)
{
  auto& irb = *mCurIrb;

  llvm::Value* retVal;
  if (!obj->expr)
    retVal = nullptr;
  else
    retVal = self(obj->expr);

  mCurIrb->CreateRet(retVal);

  auto exitBb = llvm::BasicBlock::Create(mCtx, "return_exit", mCurFunc);
  mCurIrb = std::make_unique<llvm::IRBuilder<>>(exitBb);
}

void
EmitIR::operator()(DeclStmt* obj) // 对声明语句进行处理的operator()重载函数
{
  for (auto&& decl : obj->decls) // 对于声明语句中的每一个声明
  {
    auto& irb = *mCurIrb;
    auto p = decl->dcst<VarDecl>();
    auto ty = self(p->type);
    llvm::AllocaInst* lvar = irb.CreateAlloca(ty, nullptr, p->name);
    // irb.CreateStore(irb.getInt32(10), lvar);
    p->any = lvar;
    // lvar->setInitializer(llvm::ConstantInt::get(ty, 0));
    if (p->init == nullptr)
      continue;
    trans_init(lvar, p->init);
    // // mCurIrb->CreateRet(nullptr);
  }
}

void
EmitIR::operator()(ExprStmt* obj)
{
  auto exprVal = self(obj->expr);
  // (void)exprVal;  // 用于处理可能未使用的警告
  //   auto& irb = *mCurIrb;
  //   auto p = obj->expr->dcst<BinaryExpr>();
  //   auto lft = self(p->lft);
  //   auto rht = self(p->rht);
  //   irb.CreateStore(rht, lft);
  // ABORT();
}

// struct IfStmt : Stmt
// {
//   Expr* cond{ nullptr };
//   Stmt *then{ nullptr }, *else_{ nullptr };

// private:
//   void __mark__(Mark mark) override;
// };

void EmitIR::operator()(IfStmt* obj) {
  auto& irb = *mCurIrb;
  auto condVal = self(obj->cond);
  auto func = irb.GetInsertBlock()->getParent();

  auto thenBb = llvm::BasicBlock::Create(mCtx, "if.then", func);
  llvm::BasicBlock* elseBb = nullptr;
  auto endBb = llvm::BasicBlock::Create(mCtx, "if.end", func);

  if (obj->else_) {
    elseBb = llvm::BasicBlock::Create(mCtx, "if.else", func);
    irb.CreateCondBr(condVal, thenBb, elseBb);
    irb.SetInsertPoint(elseBb);
    self(obj->else_);
    if (!irb.GetInsertBlock()->getTerminator()) {
      irb.CreateBr(endBb);
    }
  } else {
    irb.CreateCondBr(condVal, thenBb, endBb);
  }

  irb.SetInsertPoint(thenBb);
  self(obj->then);
  if (!irb.GetInsertBlock()->getTerminator()) {
    irb.CreateBr(endBb);
  }

  irb.SetInsertPoint(endBb);
}

/*
struct WhileStmt : Stmt
{
  Expr* cond{ nullptr };
  Stmt* body{ nullptr };

private:
  void __mark__(Mark mark) override;
};*/
void
EmitIR::operator()(WhileStmt* obj)
{
  auto& irb = *mCurIrb;
  auto func = irb.GetInsertBlock()->getParent();
  auto condBb = llvm::BasicBlock::Create(mCtx, "while.cond", func);
  auto bodyBb = llvm::BasicBlock::Create(mCtx, "while.body", func);
  auto endBb = llvm::BasicBlock::Create(mCtx, "while.end", func);
  
  auto prevLoopEnd = mCurrentLoopEnd;
  auto prevLoopCond = mCurrentLoopCond;
  mCurrentLoopEnd = endBb;
  mCurrentLoopCond = condBb;

  irb.CreateBr(condBb);

  irb.SetInsertPoint(condBb);
  auto condVal = self(obj->cond);
  irb.CreateCondBr(condVal, bodyBb, endBb);

  irb.SetInsertPoint(bodyBb);
  self(obj->body);
  if (!irb.GetInsertBlock()->getTerminator()) {
    irb.CreateBr(condBb);
  }

  irb.SetInsertPoint(endBb);

  mCurrentLoopEnd = prevLoopEnd;
  mCurrentLoopCond = prevLoopCond;
}

// struct BreakStmt : Stmt
// {
//   Stmt* loop{ nullptr };

// private:
//   void __mark__(Mark mark) override;
// };
void
EmitIR::operator()(BreakStmt* obj)
{
   auto& irb = *mCurIrb;
  irb.CreateBr(mCurrentLoopEnd);  // 跳转到当前循环的结束块
}

// struct ContinueStmt : Stmt
// {
//   Stmt* loop{ nullptr };

// private:
//   void __mark__(Mark mark) override;
// };
void
EmitIR::operator()(ContinueStmt* obj)
{
  auto& irb = *mCurIrb;
  irb.CreateBr(mCurrentLoopCond);  // 跳转到当前循环的条件检查块
}

void 
EmitIR::operator()(NullStmt* obj)
{
  return;
}
//==============================================================================
// 声明
//==============================================================================

void
EmitIR::operator()(Decl* obj) // 对申明进行处理的operator()重载函数
{
  // TODO: 添加变量声明处理的跳转

  if (auto p = obj->dcst<FunctionDecl>())
    return self(p);

  if (auto p = obj->dcst<VarDecl>()) // 如果是变量声明
    return self(p);

  ABORT();
}

void
EmitIR::operator()(FunctionDecl* obj)
{
  // 创建函数
  auto fty =
    llvm::dyn_cast<llvm::FunctionType>(self(obj->type)); // 处理函数类型
  auto func = llvm::Function::Create(                    // 创建函数
    fty,
    llvm::GlobalVariable::ExternalLinkage,
    obj->name,
    mMod);

  obj->any = func;

  if (obj->body == nullptr)
    return;
  auto entryBb =
    llvm::BasicBlock::Create(mCtx, "entry", func); // entry基本块，设置插入点
  mCurIrb = std::make_unique<llvm::IRBuilder<>>(entryBb);
  auto& entryIrb = *mCurIrb;

  // TODO: 添加对函数参数的处理
  auto argIter = func->arg_begin();
  // const Type* type;
  // std::string name;
  if (obj->params.size() > 0)
    for (auto&& param : obj->params) {
      // 为函数参数创建 LLVM IR 中的局部变量
      auto lvar =
        entryIrb.CreateAlloca(self(param->type), nullptr, param->name);
      // 将函数参数的值存储到局部变量中
      entryIrb.CreateStore(&*argIter, lvar);
      // 将局部变量添加到函数参数的映射中
      param->any = lvar;
      // 设置函数参数的名称
      argIter++->setName(param->name);
    }

  // 翻译函数体
  mCurFunc = func; // 处理函数体
  self(obj->body);
  auto& exitIrb = *mCurIrb;

  if (fty->getReturnType()->isVoidTy())
    exitIrb.CreateRetVoid();
  else
    exitIrb.CreateUnreachable();
}

// TODO: 添加变量声明的处理
void
EmitIR::trans_init(llvm::Value* val, Expr* obj)
{
  auto& irb = *mCurIrb;

  if (auto p = obj->dcst<IntegerLiteral>()) { // 仅处理整数字面量的初始化
    auto initVal = irb.getInt32(p->val);
    irb.CreateStore(initVal, val);
    return;
  }

  // // llvm::ArrayType *arrType = /* 获得数组实例指针 */;

  // /// 获得数组元素个数
  // uint64_t len = arrType->getNumElements();
  // /// 获得数组元素类型
  // llvm::Type* elementType = arrType->getElementType();
  // Value *CreateInBoundsGEP(Type *Ty, Value *Ptr, ArrayRef<Value *> IdxList),
  // const Twine &Name="");

  if (auto p = obj->dcst<InitListExpr>()) {
    auto arrType = llvm::dyn_cast<llvm::ArrayType>(self(p->type));
    for (int i = 0; i < arrType->getNumElements(); ++i) {
      auto index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), i);
      auto elementPtr =
        irb.CreateInBoundsGEP(arrType, val, { irb.getInt32(0), index });
      if (i < p->list.size()) {
        trans_init(elementPtr, p->list[i]);
      } else {
        // 处理初始化列表不足时，剩余元素的默认初始化
        if (val->getType()->isIntegerTy())
          irb.CreateStore(irb.getInt32(0), val);
        else
          trans_init(elementPtr, p->list[0]);
      }
    }
    return;
  }

  if (auto p = obj->dcst<ImplicitInitExpr>()) { // 隐式初始化
    auto arrType = self(p->type);
    if (arrType->isIntegerTy()) {
      irb.CreateStore(irb.getInt32(0), val);
      return;
    }
    if (auto subType = llvm::dyn_cast<llvm::ArrayType>(arrType)) {
      for (int i = 0; i < subType->getNumElements(); ++i) {
        auto index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), i);
        auto elementPtr =
          irb.CreateInBoundsGEP(subType, val, { irb.getInt32(0), index });
        irb.CreateStore(irb.getInt32(0), elementPtr);
      }
      return;
    }
  }

  if (auto p = obj->dcst<ImplicitCastExpr>()) { // 隐式转换
    auto sub = self(p);
    irb.CreateStore(sub, val); // CreateStore用法是将sub的值存储到val中
    return;
  }

  if (auto p = obj->dcst<CallExpr>()) { // 函数调用
    auto sub = self(p);
    irb.CreateStore(sub, val);
    return;
  }
  if (auto p = obj->dcst<DeclRefExpr>()){
    auto sub = self(p);
    irb.CreateStore(sub, val);
    return;
  }
  if (auto p = obj->dcst<BinaryExpr>()){
    auto sub = self(p);
    irb.CreateStore(sub, val);
    return;
  }
  if (auto p = obj->dcst<UnaryExpr>()){
    auto sub = self(p);
    irb.CreateStore(sub, val);
    return;
  }
  ABORT();
}

void
EmitIR::operator()(VarDecl* obj)
{
  // mCtx是LLVM的上下文
  auto ty = self(obj->type); // 直接使用 LLVM 的 int32 类型
  auto gvar = new llvm::GlobalVariable(
    mMod, ty, false, llvm::GlobalVariable::ExternalLinkage, nullptr, obj->name);

  obj->any = gvar;

  // 默认初始化为 0
  // gvar->setInitializer(llvm::ConstantInt::get(ty, 0));
  gvar->setInitializer(llvm::Constant::getNullValue(ty));

  if (obj->init == nullptr)
    return;

  // 创建构造函数用于初始化
  // mCurFunc是当前函数
  mCurFunc = llvm::Function::Create(
    mCtorTy, llvm::GlobalVariable::PrivateLinkage, "ctor_" + obj->name, mMod);
  llvm::appendToGlobalCtors(mMod, mCurFunc, 65535);

  auto entryBb = llvm::BasicBlock::Create(mCtx, "entry", mCurFunc);
  mCurIrb = std::make_unique<llvm::IRBuilder<>>(entryBb);
  trans_init(gvar, obj->init);
  mCurIrb->CreateRet(nullptr);
}