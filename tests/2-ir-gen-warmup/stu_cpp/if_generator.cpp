#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main() {
  auto module = new Module("Cminus code");
  auto builder = new IRBuilder(nullptr, module);

  Type *Int32Type = Type::get_int32_type(module);
  Type *FloatingType = Type::get_float_type(module);


  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
  auto bb = BasicBlock::create(module, "entry", mainFun);
  builder->set_insert_point(bb);

  auto retAlloca = builder->create_alloca(Int32Type);
  auto aAlloca = builder->create_alloca(FloatingType);

  builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0
  builder->create_store(CONST_FP(5.555), aAlloca);

  auto aLoad = builder->create_load(aAlloca);
  auto fcmp = builder->create_fcmp_gt(aLoad, CONST_FP(1.0));

  auto trueBB = BasicBlock::create(module, "trueBB", mainFun);
  auto retBB = BasicBlock::create(module, "retBB", mainFun);
  auto br = builder->create_cond_br(fcmp, trueBB, retBB);  // 条件BR

  //trueBB
  builder->set_insert_point(trueBB);
  builder->create_store(CONST_INT(233), retAlloca);
  builder->create_br(retBB);  // br retBB
  //retBB
  builder->set_insert_point(retBB);
  auto retLoad = builder->create_load(retAlloca);
  builder->create_ret(retLoad);


  std::cout << module->print();
  delete module;
  return 0;
}