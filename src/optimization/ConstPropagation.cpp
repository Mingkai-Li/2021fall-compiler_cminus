#include "ConstPropagation.hpp"
#include "logging.hpp"

// 给出了返回整形值的常数折叠实现，大家可以参考，在此基础上拓展
// 当然如果同学们有更好的方式，不强求使用下面这种方式



//return nullptr if not a const instr
//return const* result if it's a const instr 
Constant *ConstFolder::compute(Instruction *instr)
{    
    //check parameter
    std::vector<Constant *> const_operands;
    for(auto operand : instr->get_operands()){
        auto constant_operand = dynamic_cast<Constant *>(operand);
        //instr not const, abort
        if(!constant_operand){
            return nullptr;
        }
        const_operands.push_back(constant_operand);
    }
    //cal const result
    auto op = instr->get_instr_type();
    switch (op){
        //2 int operands
        case Instruction::add:
        {
            int c_value0 = dynamic_cast<ConstantInt *>(const_operands[0])->get_value();
            int c_value1 = dynamic_cast<ConstantInt *>(const_operands[1])->get_value();
            return ConstantInt::get(c_value0 + c_value1, module_);
            break;
        }
        case Instruction::sub:
        {
            int c_value0 = dynamic_cast<ConstantInt *>(const_operands[0])->get_value();
            int c_value1 = dynamic_cast<ConstantInt *>(const_operands[1])->get_value();
            return ConstantInt::get(c_value0 - c_value1, module_);
            break;
        }
        case Instruction::mul:
        {
            int c_value0 = dynamic_cast<ConstantInt *>(const_operands[0])->get_value();
            int c_value1 = dynamic_cast<ConstantInt *>(const_operands[1])->get_value();
            return ConstantInt::get(c_value0 * c_value1, module_);
            break;
        }
        case Instruction::sdiv:
        {
            int c_value0 = dynamic_cast<ConstantInt *>(const_operands[0])->get_value();
            int c_value1 = dynamic_cast<ConstantInt *>(const_operands[1])->get_value();
            return ConstantInt::get(int(c_value0 / c_value1), module_);
            break;
        }
        //2 fp operands
        case Instruction::fadd:
        {
            float c_value0 = dynamic_cast<ConstantFP *>(const_operands[0])->get_value();
            float c_value1 = dynamic_cast<ConstantFP *>(const_operands[1])->get_value();
            return ConstantFP::get(c_value0 + c_value1, module_);
            break;
        }
        case Instruction::fsub:
        {
            float c_value0 = dynamic_cast<ConstantFP *>(const_operands[0])->get_value();
            float c_value1 = dynamic_cast<ConstantFP *>(const_operands[1])->get_value();
            return ConstantFP::get(c_value0 - c_value1, module_);
            break;
        }
        case Instruction::fmul:
        {
            float c_value0 = dynamic_cast<ConstantFP *>(const_operands[0])->get_value();
            float c_value1 = dynamic_cast<ConstantFP *>(const_operands[1])->get_value();
            return ConstantFP::get(c_value0 * c_value1, module_);
            break;
        }
        case Instruction::fdiv:
        {
            float c_value0 = dynamic_cast<ConstantFP *>(const_operands[0])->get_value();
            float c_value1 = dynamic_cast<ConstantFP *>(const_operands[1])->get_value();
            return ConstantFP::get(c_value0 / c_value1, module_);
            break;
        }
        //cmp
        case Instruction::cmp:
        {
            int c_value0 = dynamic_cast<ConstantInt *>(const_operands[0])->get_value();
            int c_value1 = dynamic_cast<ConstantInt *>(const_operands[1])->get_value();
            auto op = dynamic_cast<CmpInst*>(instr)->get_cmp_op();
            return compute_cmp(op, c_value0, c_value1);
            break;
        }
        case Instruction::fcmp:
        {
            float c_value0 = dynamic_cast<ConstantFP *>(const_operands[0])->get_value();
            float c_value1 = dynamic_cast<ConstantFP *>(const_operands[1])->get_value();
            auto op = dynamic_cast<FCmpInst*>(instr)->get_cmp_op();
            return compute_cmp(op, c_value0, c_value1);
            break;
        }
        //1 operand
        case Instruction::zext:
        {
            int c_value0 = dynamic_cast<ConstantInt *>(const_operands[0])->get_value();
            return ConstantInt::get(bool(c_value0), module_);
            break;
        }
        case Instruction::sitofp:
        {
            int c_value0 = dynamic_cast<ConstantInt *>(const_operands[0])->get_value();
            return ConstantFP::get(float(c_value0), module_);
            break;   
        }
        case Instruction::fptosi:
        {
            float c_value0 = dynamic_cast<ConstantFP *>(const_operands[0])->get_value();
            return ConstantInt::get(int(c_value0), module_);
            break;
        }
        //if its other type of instr, also return nullptr
        default:
            return nullptr;
            break;
    }
     
}





void ConstPropagation::run()
{
    // 从这里开始吧！
    //init
    const_folder_ = new ConstFolder(m_);
    //
    for (auto f : m_->get_functions()){
        if (f->get_basic_blocks().size() == 0){
                continue;
            }
        for(auto bb: f->get_basic_blocks()){
            run_bb(bb);
        }
            
    }
}





void ConstPropagation::run_bb(BasicBlock *bb){
    std::vector<Instruction *> instr2del;

    auto instrs = bb->get_instructions();
    for(auto pinstr = instrs.begin(); pinstr != instrs.end(); pinstr++){
        auto instr = *pinstr;

        //1.expression statement:(f)binary+(f)cmp+zext+fptosi+sitofp
        auto const_result = const_folder_->compute(instr); 
        //propagate
        if(const_result != nullptr){
            instr->replace_all_use_with(const_result);
            instr2del.push_back(instr);
            continue;
        }

        //2.load-store statement:load+store
        auto instr_store = dynamic_cast<StoreInst*>(instr);
        if(instr_store != nullptr){     //store instr
            auto const_rval = dynamic_cast<Constant*>(instr_store->get_rval());
            if(const_rval != nullptr){    //const rval
                //propagate to rest of the bb
                auto lval = instr_store->get_lval();
                for(auto pinstr_res = pinstr; pinstr_res != instrs.end(); pinstr_res++){
                    auto instr_res = *pinstr_res;
                    auto instr_load = dynamic_cast<LoadInst*>(instr_res);
                    if(instr_load != nullptr && instr_load->get_lval() == lval){    //load lval
                        instr_load->replace_all_use_with(const_rval);
                        instr2del.push_back(instr_load);
                    }
                }
            }
            continue;
        }

        //3.br statement with const-cond
        auto instr_br = dynamic_cast<BranchInst*>(instr);
        if(instr_br != nullptr && instr_br->is_cond_br()){      //br instr with cond 
            auto const_cond = dynamic_cast<ConstantInt*>(instr_br->get_operand(0)); 
            if(const_cond != nullptr){      //const cond
            
                auto if_true = dynamic_cast<BasicBlock*>(instr_br->get_operand(1));
                auto if_false = dynamic_cast<BasicBlock*>(instr_br->get_operand(2));
                instr_br->remove_operands(1, 2);

                if(const_cond->get_value()){    //if_true
                    instr_br->set_operand(0, if_true);
                    bb->remove_succ_basic_block(if_false);
                    if_false->remove_pre_basic_block(bb);
                }
                else{   //if_false
                    instr_br->set_operand(0, if_false);
                    bb->remove_succ_basic_block(if_true);
                    if_true->remove_pre_basic_block(bb);
                }

            }
            

        }

    }

    //del instr
    for(auto instr : instr2del){
        bb->delete_instr(instr);
    }


}