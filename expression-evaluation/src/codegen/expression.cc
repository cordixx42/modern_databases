#include "moderndbs/codegen/expression.h"
#include "moderndbs/error.h"

using JIT = moderndbs::JIT;
using Expression = moderndbs::Expression;
using ExpressionCompiler = moderndbs::ExpressionCompiler;
using data64_t = moderndbs::data64_t;
using Constant = moderndbs::Constant;
using Argument = moderndbs::Argument;
using Cast = moderndbs::Cast;
using AddExpression = moderndbs::AddExpression;
using SubExpression = moderndbs::SubExpression;
using MulExpression = moderndbs::MulExpression;
using DivExpression = moderndbs::DivExpression;

/// Evaluate the expresion.
data64_t
Expression::evaluate(const data64_t* /*args*/) {
   throw NotImplementedException();
}

/// Build the expression
llvm::Value* Expression::build(llvm::IRBuilder<>& /*builder*/, llvm::Value* /*args*/) {
   throw NotImplementedException();
}

/// Constructor.
ExpressionCompiler::ExpressionCompiler(llvm::orc::ThreadSafeContext& context)
   : context(context), module(std::make_unique<llvm::Module>("meaningful_module_name", *context.getContext())), jit(context), fnPtr(nullptr) {}

/// Compile an expression.
void ExpressionCompiler::compile(Expression& expression, bool verbose) {
   /// TODO compile a function for the expression

   std::string name = "toplevel";

   // insert function into module
   llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getInt64Ty(*context.getContext()), {llvm::Type::getInt64PtrTy(*context.getContext())}, true);
   llvm::Function* func = llvm::cast<llvm::Function>(module->getOrInsertFunction(name, funcType).getCallee());

   // create basic block entry for the function
   llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*context.getContext(), "entry", func);
   builder.SetInsertPoint(entryBlock);

   // generate code for expression tree
   llvm::IRBuilder<> builder(*(this->context.getContext()));
   llvm::Value* res = expression.build(builder, func->getArg(0));

   builder.CreateRet(builder.CreateBitCast(res, func->getReturnType()));

   jit.addModule(std::move(this->module));

   fnPtr = reinterpret_cast<data64_t (*)(data64_t*)>(jit.getPointerToFunction(name));

   // hi
   // throw NotImplementedException();
}

/// Compile an expression.
data64_t ExpressionCompiler::run(data64_t* args) {
   return fnPtr(args);
}

// constant
data64_t Constant::evaluate(const data64_t* args) {
   return this->value;
}

llvm::Value* Constant::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   if (this->getType() == ValueType::INT64) {
      // llvm::ConstantInt::getSigned
      return builder.getInt64(std::bit_cast<int64_t>(this->value));
   } else {
      return llvm::ConstantFP::get(builder.getDoubleTy(), std::bit_cast<double>(this->value));
   }
}

//argument
data64_t Argument::evaluate(const data64_t* args) {
   uint64_t index = this->index;
   return args[index];
}

llvm::Value* Argument::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   if (this->type == ValueType::INT64) {
      llvm::Value* idx = builder.getInt64(std::bit_cast<int64_t>(this->index));
      return builder.CreateLoad(builder.getInt64Ty(), builder.CreateGEP(builder.getInt64Ty(), args, idx));
   } else {
      llvm::Value* idx = builder.getInt64(std::bit_cast<int64_t>(this->index));
      return builder.CreateLoad(builder.getDoubleTy(), builder.CreateGEP(builder.getDoubleTy(), args, idx));
   }
}

//cast

data64_t Cast::evaluate(const data64_t* args) {
   // cast from childType to valueType
   if (this->childType != this->type) {
      if (this->childType == ValueType::DOUBLE) {
         double val = std::bit_cast<double>(child.evaluate(args));
         int64_t v = static_cast<int64_t>(val);
         return std::bit_cast<data64_t>(v);
      } else {
         int64_t val = std::bit_cast<int64_t>(child.evaluate(args));
         double v = static_cast<double>(val);
         return std::bit_cast<data64_t>(v);
      }
   }
   return child.evaluate(args);
}

llvm::Value* Cast::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   if (this->childType != this->type) {
      if (this->childType == ValueType::DOUBLE) {
         return builder.CreateFPToSI(child.build(builder, args), builder.getInt64Ty());
      } else {
         return builder.CreateSIToFP(child.build(builder, args), builder.getDoubleTy());
      }
   }
   return child.build(builder, args);
}

// add
data64_t AddExpression::evaluate(const data64_t* args) {
   assert(this->left.getType() == this->right.getType());
   if (this->left.getType() == ValueType::INT64) {
      int64_t l = std::bit_cast<int64_t>(this->left.evaluate(args));
      int64_t r = std::bit_cast<int64_t>(this->right.evaluate(args));
      return std::bit_cast<data64_t>(l + r);
   } else {
      double l = std::bit_cast<double>(this->left.evaluate(args));
      double r = std::bit_cast<double>(this->right.evaluate(args));
      return std::bit_cast<data64_t>(l + r);
   }
}

llvm::Value* AddExpression::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   if (this->left.getType() == ValueType::INT64) {
      return builder.CreateAdd(this->left.build(builder, args), this->right.build(builder, args));
   } else {
      return builder.CreateFAdd(this->left.build(builder, args), this->right.build(builder, args));
   }
}

//sub
data64_t SubExpression::evaluate(const data64_t* args) {
   assert(this->left.getType() == this->right.getType());
   if (this->left.getType() == ValueType::INT64) {
      int64_t l = std::bit_cast<int64_t>(this->left.evaluate(args));
      int64_t r = std::bit_cast<int64_t>(this->right.evaluate(args));
      return std::bit_cast<data64_t>(l - r);
   } else {
      // double *reinterpret_cast<double*>(&l)
      double l = std::bit_cast<double>(this->left.evaluate(args));
      double r = std::bit_cast<double>(this->right.evaluate(args));
      return std::bit_cast<data64_t>(l - r);
   }
}

llvm::Value* SubExpression::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   if (this->left.getType() == ValueType::INT64) {
      return builder.CreateSub(this->left.build(builder, args), this->right.build(builder, args));
   } else {
      return builder.CreateFSub(this->left.build(builder, args), this->right.build(builder, args));
   }
}

//mul
data64_t MulExpression::evaluate(const data64_t* args) {
   assert(this->left.getType() == this->right.getType());
   if (this->left.getType() == ValueType::INT64) {
      int64_t l = std::bit_cast<int64_t>(this->left.evaluate(args));
      int64_t r = std::bit_cast<int64_t>(this->right.evaluate(args));
      return std::bit_cast<data64_t>(l * r);
   } else {
      double l = std::bit_cast<double>(this->left.evaluate(args));
      double r = std::bit_cast<double>(this->right.evaluate(args));
      return std::bit_cast<data64_t>(l * r);
   }
}

llvm::Value* MulExpression::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   if (this->left.getType() == ValueType::INT64) {
      return builder.CreateMul(this->left.build(builder, args), this->right.build(builder, args));
   } else {
      return builder.CreateFMul(this->left.build(builder, args), this->right.build(builder, args));
   }
}

//div
data64_t DivExpression::evaluate(const data64_t* args) {
   assert(this->left.getType() == this->right.getType());
   if (this->left.getType() == ValueType::INT64) {
      int64_t l = std::bit_cast<int64_t>(this->left.evaluate(args));
      int64_t r = std::bit_cast<int64_t>(this->right.evaluate(args));
      return std::bit_cast<data64_t>(l / r);
   } else {
      double l = std::bit_cast<double>(this->left.evaluate(args));
      double r = std::bit_cast<double>(this->right.evaluate(args));
      return std::bit_cast<data64_t>(l / r);
   }
}

llvm::Value* DivExpression::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   if (this->left.getType() == ValueType::INT64) {
      return builder.CreateSDiv(this->left.build(builder, args), this->right.build(builder, args));
   } else {
      return builder.CreateFDiv(this->left.build(builder, args), this->right.build(builder, args));
   }
}
