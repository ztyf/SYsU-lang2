#include "asg.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

class EmitIR
{
public:
  Obj::Mgr& mMgr;//对象管理器
  llvm::Module mMod;//LLVM模块

  //构造函数
  EmitIR(Obj::Mgr& mgr, llvm::LLVMContext& ctx, llvm::StringRef mid = "-");

  llvm::Module& operator()(asg::TranslationUnit* tu);//入口函数

private:
  llvm::LLVMContext& mCtx;//LLVM上下文

  llvm::Type* mIntTy;//整型
  llvm::FunctionType* mCtorTy;//函数类型

  llvm::Function* mCurFunc;//当前函数
  std::unique_ptr<llvm::IRBuilder<>> mCurIrb;//当前IR构建器

  //============================================================================
  // 类型
  //============================================================================

  llvm::Type* operator()(const asg::Type* type);

  //============================================================================
  // 表达式
  //============================================================================

  llvm::Value* operator()(asg::Expr* obj);

  llvm::Constant* operator()(asg::IntegerLiteral* obj);

  // TODO: 添加表达式处理相关声明
  
  llvm::Value* operator()(asg::BinaryExpr* obj);

  llvm::Value* operator()(asg::UnaryExpr* obj);

  llvm::Value* operator()(asg::ParenExpr* obj);
  
  llvm::Value* operator()(asg::ImplicitCastExpr* obj);

  llvm::Value* operator()(asg::DeclRefExpr* obj);

  llvm::Value* operator()(asg::CallExpr* obj);
  


  //============================================================================
  // 语句
  //============================================================================

  void operator()(asg::Stmt* obj);

  void operator()(asg::CompoundStmt* obj);

  void operator()(asg::ReturnStmt* obj);

  void operator()(asg::DeclStmt* obj);

  void operator()(asg::ExprStmt* obj);

  void operator()(asg::IfStmt* obj);

  void operator()(asg::WhileStmt* obj);

  void operator()(asg::BreakStmt* obj);

  void operator()(asg::ContinueStmt* obj);

  void operator()(asg::NullStmt* obj);

    llvm::BasicBlock* mCurrentLoopEnd = nullptr;  // 当前循环的结束块
    llvm::BasicBlock* mCurrentLoopCond = nullptr; // 当前循环的条件块



  // TODO: 添加语句处理相关声明

  //============================================================================
  // 声明
  //============================================================================

  void operator()(asg::Decl* obj);

  void operator()(asg::FunctionDecl* obj);

  // TODO: 添加声明处理相关声明
  void trans_init(llvm::Value* val, asg::Expr* obj);

  void operator()(asg::VarDecl* obj);
  
};
