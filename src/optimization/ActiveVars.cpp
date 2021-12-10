#include "ActiveVars.hpp"
#define IS_CONST_INT(value) (dynamic_cast<ConstantInt*>(value)==nullptr)
#define IS_CONST_FP(value) (dynamic_cast<ConstantFP*>(value)==nullptr)
#define IS_CONST(value) (IS_CONST_INT(value) || IS_CONST_FP(value))

void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;  

            func_->set_instr_name();
            live_in.clear();
            live_out.clear();
            
            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内
            std::map<BasicBlock*, std::set<Value*>> bb2def, bb2use;
            std::map<BasicBlock*, std::map<Value*, std::set<Value*>>> bb2phi_use;
            
            for(auto bb : func_->get_basic_blocks()){
                // initialize live_in/live_out
                std::set<Value*> new_in;
                std::set<Value*> new_out;
                live_in.insert({bb, new_in});
                live_out.insert({bb, new_out});
                
                // use/phi_use/def for each basic block
                std::set<Value*> def, use;
                std::map<Value*, std::set<Value*>> phi_use;

                for(auto inst : bb->get_instructions()){
                    // use/phi_use
                    if(inst->is_phi()){
                        for(int i=0;i < inst->get_num_operand()-1;i++){
                            Value* right_value = inst->get_operand(i);
                            Value* pre_bb = inst->get_operand(i+1);

                            if(!IS_CONST(right_value) && def.find(right_value) == def.end()){
                                if(phi_use.find(pre_bb) == phi_use.end()){
                                    std::set<Value*> new_set;
                                    new_set.insert(right_value);
                                    phi_use.insert({pre_bb, new_set});
                                }
                                else{
                                    phi_use[pre_bb].insert(right_value);
                                }
                            }
                        }
                    }
                    else{
                        if(inst->is_br()){
                            if(dynamic_cast<BranchInst*>(inst)->is_cond_br()){
                                Value* right_value = inst->get_operand(0);
                                if(!IS_CONST(right_value) && def.find(right_value) == def.end()){
                                    use.insert(right_value);
                                }
                            }
                        }
                        else{
                            for(auto right_value : inst->get_operands()){
                                if(!IS_CONST(right_value) && def.find(right_value) == def.end()){
                                    use.insert(right_value);
                                }
                            }
                        }
                    }

                    //def
                    if(inst->is_store() || inst->isTerminator()){
                        continue;
                    }
                    else{
                        // SSA gaurantees that it can't be used before being defed
                        // otherwise, it will be defed twice
                        def.insert(inst);
                    }
                }

                bb2use.insert({bb, use});
                bb2phi_use.insert({bb, phi_use});
                bb2def.insert({bb, def});
            }

            bool flag = true;
            while(flag){
                flag = false;

                for(auto bb : func_->get_basic_blocks()){
                    // live_out[bb]
                    for(auto suc_bb : bb->get_succ_basic_blocks()){
                        for(auto value : live_in[suc_bb]){
                            live_out[bb].insert(value);
                        }
                        if(bb2phi_use[suc_bb].find(bb) != bb2phi_use[suc_bb].end()){
                            for(auto value : bb2phi_use[suc_bb][bb]){
                                live_out[bb].insert(value);
                            }
                        }
                    }

                    // live_in[bb]
                    for(auto value : bb2def[bb]){
                        if(live_in[bb].find(value) != live_in[bb].end()){
                            flag = true;
                            live_in[bb].erase(value);
                        }
                    }
                    for(auto value : bb2use[bb]){
                        if(live_in[bb].find(value) == live_in[bb].end()){
                            flag = true;
                            live_in[bb].insert(value);
                        }
                    }
                }
            }

            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]" ;
            active_vars += ",\n";   
        }
    }
    active_vars += "\n";
    active_vars +=  "    },\n";
    
    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}