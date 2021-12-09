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


  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
  auto bb = BasicBlock::create(module, "entry", mainFun);
  builder->set_insert_point(bb);

  auto retAlloca = builder->create_alloca(Int32Type);
  auto aAlloca = builder->create_alloca(Int32Type);
  auto iAlloca = builder->create_alloca(Int32Type);

  builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0
  builder->create_store(CONST_INT(10), aAlloca);
  builder->create_store(CONST_INT(0), iAlloca);

  auto whileBB = BasicBlock::create(module, "whileBB", mainFun);
  auto doBB = BasicBlock::create(module, "doBB", mainFun);
  auto retBB = BasicBlock::create(module, "retBB", mainFun);
  
  builder->create_br(whileBB);  // br retBB


  //whileBB
  builder->set_insert_point(whileBB);
  
  auto iLoad = builder->create_load(iAlloca);   //load i
  auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));

  builder->create_cond_br(icmp, doBB, retBB);  // 条件BR

  //doBB
  builder->set_insert_point(doBB);

  auto i0Load = builder->create_load(iAlloca);
  auto i1 = builder->create_iadd(i0Load, CONST_INT(1));
  builder->create_store(i1, iAlloca);

  auto a0Load = builder->create_load(aAlloca);
  auto i2Load = builder->create_load(iAlloca);
  auto a1 = builder->create_iadd(a0Load, i2Load);
  builder->create_store(a1, aAlloca);

  builder->create_br(whileBB);  // br retBB

  //retBB
  builder->set_insert_point(retBB);
  auto aLoad = builder->create_load(aAlloca);
  builder->create_store(aLoad, retAlloca);
  builder->create_ret(aLoad);


  std::cout << module->print();
  delete module;
  return 0;
}