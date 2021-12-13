#ifndef CONSTPROPAGATION_HPP
#define CONSTPROPAGATION_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>

// tips: 用来判断value是否为ConstantFP/ConstantInt
ConstantFP* cast_constantfp(Value *value);
ConstantInt* cast_constantint(Value *value);


// tips: ConstFloder类

class ConstFolder
{
public:
    ConstFolder(Module *m) : module_(m) {}

    Constant *compute(Instruction *instr);

    // ...
private:
    Module *module_;

    //inline method
    Constant *compute_cmp(
        CmpInst::CmpOp cmp_op, int lhs, int rhs){
            switch (cmp_op)
            {
                case CmpInst::CmpOp::EQ:
                {
                    return ConstantInt::get(lhs == rhs, module_);
                    break;
                }
                case CmpInst::CmpOp::NE:
                {
                    return ConstantInt::get(lhs != rhs, module_);
                    break;
                }
                case CmpInst::CmpOp::GT:
                {
                    return ConstantInt::get(lhs > rhs, module_);
                    break;
                }
                case CmpInst::CmpOp::GE:
                {
                    return ConstantInt::get(lhs >= rhs, module_);
                    break;
                }
                case CmpInst::CmpOp::LT:
                {
                    return ConstantInt::get(lhs < rhs, module_);
                    break;
                }
                case CmpInst::CmpOp::LE:
                {
                    return ConstantInt::get(lhs <= rhs, module_);
                    break;
                }
                default:
                    return nullptr;
                    break;
            }
        }

    Constant *compute_cmp(
        FCmpInst::CmpOp cmp_op, float lhs, float rhs){
            switch (cmp_op)
            {
                case FCmpInst::CmpOp::EQ:
                {
                    return ConstantInt::get(lhs == rhs, module_);
                    break;
                }
                case FCmpInst::CmpOp::NE:
                {
                    return ConstantInt::get(lhs != rhs, module_);
                    break;
                }
                case FCmpInst::CmpOp::GT:
                {
                    return ConstantInt::get(lhs > rhs, module_);
                    break;
                }
                case FCmpInst::CmpOp::GE:
                {
                    return ConstantInt::get(lhs >= rhs, module_);
                    break;
                }
                case FCmpInst::CmpOp::LT:
                {
                    return ConstantInt::get(lhs < rhs, module_);
                    break;
                }
                case FCmpInst::CmpOp::LE:
                {
                    return ConstantInt::get(lhs <= rhs, module_);
                    break;
                }
                default:
                    return nullptr;
                    break;
            }
        }

};

class ConstPropagation : public Pass
{
private:
    ConstFolder* const_folder_;
    void run_bb(BasicBlock *bb);
public:
    ConstPropagation(Module *m) : Pass(m) {}
    void run();
    
};

#endif
