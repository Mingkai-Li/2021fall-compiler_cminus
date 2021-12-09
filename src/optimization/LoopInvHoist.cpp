#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"

struct pair{
    BasicBlock* bb;
    Instruction* inst;
};

void LoopInvHoist::run()
{
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();

    // 接下来由你来补充啦！
    auto func_list = m_->get_functions();
    for(auto func : func_list){
        for(auto loop : loop_searcher.get_loops_in_func(func)){
            bool flag = true;

            BasicBlock* pre_base_bb = *(loop_searcher.get_loop_base(loop)->get_pre_basic_blocks().begin());
            Instruction* pre_base_bb_terminator = pre_base_bb->get_terminator();
            pre_base_bb->get_instructions().remove(pre_base_bb_terminator);

            // set only make search easier
            // each left value only appears once
            std::unordered_set<Value*> left_values;
            for(auto bb : *loop){
                for(auto inst : bb->get_instructions()){
                    left_values.insert(inst);
                }
            }
            
            while(flag){
                flag = false;
                
                std::vector<struct pair> hoist_pairs;
                for(auto bb : *loop){
                    for(auto inst : bb->get_instructions()){
                        // instructions that can't be hoisted
                        if(inst->is_br() || inst->is_ret()) continue; //terminator can't be hoisted
                        if(inst->is_phi()) continue; //hoist of phi will cause phi's precessor basic block changes
                        if(inst->is_call()) continue; //hoist of function call will cause many instructions hoisted
                        if(inst->is_load()) continue; //even if the address remains the same, the result of the load differs
                        if(inst->is_store()) continue; //hoist of store may change the result of load

                        bool hoist = true;
                        for(auto right_value : inst->get_operands()){
                            if(left_values.find(right_value) != left_values.end()){
                                hoist = false;
                                break;
                            }
                        }
                        if(hoist){
                            // instruction can't be modified here
                            // otherwise, bb will be changed thus violates the for iteration attribute
                            flag = true;
                            left_values.erase(inst);
                            struct pair new_pair;
                            new_pair.bb = bb;
                            new_pair.inst = inst;
                            hoist_pairs.push_back(new_pair);
                        }
                    }
                }

                for(auto pair : hoist_pairs){
                    pair.inst->set_parent(pre_base_bb);
                    pre_base_bb->add_instruction(pair.inst);
                    pair.bb->get_instructions().remove(pair.inst);
                }
            }

            pre_base_bb->add_instruction(pre_base_bb_terminator);
        }
    }
}
