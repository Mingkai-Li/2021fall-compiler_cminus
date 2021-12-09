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
  auto *arrayType = ArrayType::get(Int32Type, 10);


  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
  auto bb = BasicBlock::create(module, "entry", mainFun);
  builder->set_insert_point(bb);


  auto retAlloca = builder->create_alloca(Int32Type);
  auto aAlloca = builder->create_alloca(arrayType);
  builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0

  auto a0GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(0)});
  builder->create_store(CONST_INT(10), a0GEP);

  auto a0Load = builder->create_load(a0GEP);
  auto product = builder->create_imul(a0Load, CONST_INT(2));    //a[1]=a[0]*2

  auto a1GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(1)});
  builder->create_store(product, a1GEP);
  builder->create_ret(product);


  std::cout << module->print();
  delete module;
  return 0;
}