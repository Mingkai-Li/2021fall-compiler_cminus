#include "cminusf_builder.hpp"
#include "logging.hpp"

// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_INT(num) \
    ConstantInt::get(num, module.get())


// You can define global variables here
// to store state
Type* TyInt32;
Type* TyFloat;
Type* TyVoid;
Type* TyInt32Ptr;
Type* TyFloatPtr;

Type* tmp_Type;
AllocaInst* tmp_AllocaInst;
Value* tmp_Value;
std::string tmp_string;
Function* tmp_Function;
bool tmp_bool;
bool is_lvalue;

// auxiliary functions here
Type* CminusType2Type(CminusType ctype){
    if(ctype == TYPE_INT)
        return TyInt32;
    else if(ctype == TYPE_FLOAT)
        return TyFloat;
    else
        return TyVoid;
}

Type* CminusType2Type(CminusType ctype, int size){
    if(size > 0){
        if(ctype == TYPE_INT)
            return ArrayType::get(TyInt32, size);
        else if(ctype == TYPE_FLOAT)
            return ArrayType::get(TyFloat, size);
        else
            return nullptr;
    }
    else{
        return nullptr;
    }
}

Type* CminusType2TypePtr(CminusType ctype){
    if(ctype == TYPE_INT)
        return TyInt32Ptr;
    else if(ctype == TYPE_FLOAT)
        return TyFloatPtr;
    else
        return nullptr;
}

CminusType TypeTansfer(Value* &lvalue, Value* &rvalue, IRBuilder* builder, bool left_first){
    Type* ltype;
    Type* rtype;

    if(left_first){
        ltype = lvalue->get_type()->get_pointer_element_type();
    }
    else{
        ltype = lvalue->get_type();
    }
    rtype = rvalue->get_type();

    // same type
    if(ltype == rtype){
        if(ltype == TyInt32)
            return TYPE_INT;
        else
            return TYPE_FLOAT;
    }

    // type transfer
    if(left_first){
        if(ltype == TyInt32){
            rvalue = builder->create_fptosi(rvalue, TyInt32);
            return TYPE_INT;
        }
        else{
            rvalue = builder->create_sitofp(rvalue, TyFloat);
            return TYPE_FLOAT;
        }
    }
    else{
        if(ltype == TyInt32){
            lvalue = builder->create_sitofp(lvalue, TyFloat);
        }
        else{
            rvalue = builder->create_sitofp(rvalue, TyFloat);
        }

        return TYPE_FLOAT;
    }
}

CminusType TypeTansfer(CminusType type, Value* &value, IRBuilder* builder){
    CminusType value_type;
    
    if(value->get_type() == TyInt32){
        value_type = TYPE_INT;
    }
    else if(value->get_type() == TyFloat){
        value_type = TYPE_FLOAT;
    }
    else{
        LOG(ERROR) << "return type declared as neither int nor float";
        return TYPE_VOID;
    }

    if(value_type == type){
        return type;
    }
    else{
        if(type == TYPE_INT){
            value = builder->create_fptosi(value, TyInt32);
        }
        else{
            value = builder->create_sitofp(value, TyFloat);
        }

        return type;
    }
}

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) {
    TyInt32 = Type::get_int32_type(module.get());
    TyInt32Ptr = Type::get_int32_ptr_type(module.get());
    TyFloat = Type::get_float_type(module.get());
    TyFloatPtr = Type::get_float_ptr_type(module.get());
    TyVoid = Type::get_void_type(module.get());
    is_lvalue = false;

    for(auto declaration : node.declarations){
        declaration->accept(*this);
    }
    
    if(scope.find("main") == nullptr){
        LOG(ERROR) << "missing of main function";
    }
}

void CminusfBuilder::visit(ASTNum &node) { 
    // type is either INT or FP, guaranteed by parsing
    if(node.type == TYPE_INT){
        tmp_Value = CONST_INT(node.i_val);
    }
    else{
        tmp_Value = CONST_FP(node.f_val);
    }
}

void CminusfBuilder::visit(ASTVarDeclaration &node) { 
    Type* type;
    Value* value;

    //get type
    if(node.num == nullptr){
        if(node.type == TYPE_VOID){
            LOG(ERROR) << "variable declared with void type";
            return;
        }
        type = CminusType2Type(node.type);
    }
    else{
        type = CminusType2Type(node.type, node.num->i_val);
        if(type == nullptr){
            LOG(ERROR) << "array declared with void type or negtive/zero size";
            return;
        }
    }

    //get value
    if(scope.in_global()){
        auto initializer = ConstantZero::get(type, module.get());
        value = GlobalVariable::create(node.id, module.get(), type, false, initializer);
    }
    else{
        value = builder->create_alloca(type);
    }

    if(!scope.push(node.id, value)){
        LOG(ERROR) << "variable name declared twice in this scope";
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) { 
    std::vector<Type*> types;
    std::vector<std::string> ids;
    std::vector<Value*> values;
    Function* function;

    for(auto param : node.params){
        param->accept(*this);

        // fill type vector
        if(FunctionType::is_valid_argument_type(tmp_Type)){
            types.push_back(tmp_Type);
        }
        else{
            LOG(ERROR)<< "invalid argument type";
            return;
        }

        // fill id vector
        ids.push_back(tmp_string);
    }

    function = Function::create(FunctionType::get(CminusType2Type(node.type), types), node.id, module.get());
    tmp_Function = function;
    if(!scope.push(node.id, function)){
        LOG(ERROR) << "function name declared twice";
        return;
    }
    scope.enter();
    tmp_bool = false; // no need to enter scope in compoundStmt 
    builder->set_insert_point(BasicBlock::create(module.get(), node.id, function));

    // get param values
    for(auto iter=function->arg_begin(); iter != function->arg_end(); iter++) {
        values.push_back(*iter);
    }

    // return value
    if(node.type != TYPE_VOID){
        auto retAlloc = builder->create_alloca(CminusType2Type(node.type));
        tmp_AllocaInst = retAlloc;
    }
    
    // parameter value
    for(int i=0;i < values.size();i++){
        auto argAlloc = builder->create_alloca(types[i]);
        builder->create_store(values[i], argAlloc);

        // push <id, value> into current scope
        if(!scope.push(ids[i], values[i])){
            LOG(ERROR) << "argument name declared twice in this function";
            return;
        }
    }

    node.compound_stmt->accept(*this);
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) { 
    tmp_string = node.id;
    
    if(node.type == TYPE_VOID){
        LOG(ERROR) << "parameter type declared as void or void*";
        return;
    }

    if(node.isarray){
        tmp_Type = CminusType2TypePtr(node.type);
    }
    else{
        tmp_Type = CminusType2Type(node.type);
    }
}

void CminusfBuilder::visit(ASTCompoundStmt &node) { 
    bool flag; // flag indicating need of exit
    
    if(tmp_bool){
        scope.enter();
        flag = true;
    }
    else{
        tmp_bool = true;
        flag = false;
    }

    for(auto local_declaration : node.local_declarations){
        local_declaration->accept(*this);
    }
    
    for(auto statement : node.statement_list){
        statement->accept(*this);
        if(builder->get_insert_block()->get_terminator() != nullptr){
            break;
        }
    }

    if(flag){
        scope.exit();
    }
}

void CminusfBuilder::visit(ASTExpressionStmt &node) { 
    if(node.expression != nullptr)
        node.expression->accept(*this);
}

void CminusfBuilder::visit(ASTSelectionStmt &node) { }

void CminusfBuilder::visit(ASTIterationStmt &node) { }

void CminusfBuilder::visit(ASTReturnStmt &node) { 
    if(node.expression == nullptr){
        builder->create_void_ret();
    }
    else{
        node.expression->accept(*this);
        Value* ret_value = tmp_Value;
        CminusType ret_type;
        if(tmp_Function->get_return_type() == TyInt32){
            ret_type = TYPE_INT;
        }
        else{
            ret_type = TYPE_FLOAT;
        }

        TypeTansfer(ret_type, ret_value, builder);
        builder->create_store(ret_value, tmp_AllocaInst);
        builder->create_ret(ret_value);
    }
}

void CminusfBuilder::visit(ASTVar &node) { 
    Value* value = scope.find(node.id);
    bool flag = is_lvalue;
    is_lvalue = false;

    if(node.expression == nullptr){
        if(flag){
            tmp_Value = value;
        }
        else{
            if(value->get_type()->get_pointer_element_type()->is_array_type()){
                tmp_Value = builder->create_gep(value, {CONST_INT(0), CONST_INT(0)});
            }
            else{
                tmp_Value = builder->create_load(value);
            }
        }
    }
    else{
        node.expression->accept(*this);
        Value* expr = tmp_Value;

        // check if is negative index
        Value* is_neg;

        if(expr->get_type()->is_integer_type()){
            is_neg = builder->create_icmp_lt(expr, CONST_INT(0));
        }
        else{
            is_neg = builder->create_fcmp_lt(expr, CONST_FP(0.));
        }
        auto neg_bb = BasicBlock::create(module.get(), "neg_bb", tmp_Function);
        auto pos_bb = BasicBlock::create(module.get(), "pos_bb", tmp_Function);
        builder->create_cond_br(is_neg, neg_bb, pos_bb);

        // neg_bb
        builder->set_insert_point(neg_bb);
        auto neg_idx_except_fun = scope.find("neg_idx_except_fun");
        builder->create_call(neg_idx_except_fun, {});

        if(tmp_Function->get_return_type()->is_void_type()){
            builder->create_void_ret();
        }
        else if(tmp_Function->get_return_type()->is_integer_type()){
            builder->create_ret(CONST_INT(0));
        }
        else{
            builder->create_ret(CONST_FP(0.));
        }

        // pos_bb
        builder->set_insert_point(pos_bb);
        Value* ptr;

        if(value->get_type()->get_pointer_element_type()->is_pointer_type()){
            auto load = builder->create_load(value);
            ptr = builder->create_gep(load, {expr});
        }
        else if(value->get_type()->is_array_type()){
            ptr = builder->create_gep(value, {CONST_INT(0), expr});
        }
        else{
            ptr = builder->create_gep(value, {expr});
        }

        if(flag){
            tmp_Value = ptr;
        }
        else{
            tmp_Value = builder->create_load(ptr);
        }
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) { 
    is_lvalue = true;
    node.var->accept(*this);
    Value* lvalue = tmp_Value;
    
    node.expression->accept(*this);
    Value* rvalue = tmp_Value;

    TypeTansfer(lvalue, rvalue, builder, true);
    builder->create_store(rvalue, lvalue);
    tmp_Value = rvalue;
}

void CminusfBuilder::visit(ASTSimpleExpression &node) { 
    if(node.additive_expression_r == nullptr){
        node.additive_expression_l->accept(*this);
    }
    else{
        node.additive_expression_l->accept(*this);
        Value* lvalue = tmp_Value;
        node.additive_expression_r->accept(*this);
        Value* rvalue = tmp_Value;
        
        CminusType type = TypeTansfer(lvalue, rvalue, builder, false);
        if(type == TYPE_INT){
           if(node.op == OP_LT){
               tmp_Value = builder->create_icmp_lt(lvalue, rvalue);
           }
           else if(node.op == OP_LE){
               tmp_Value = builder->create_icmp_le(lvalue, rvalue);
           }
           else if(node.op == OP_GT){
               tmp_Value == builder->create_icmp_gt(lvalue, rvalue);
           }
           else if(node.op == OP_GE){
               tmp_Value == builder->create_icmp_ge(lvalue, rvalue);
           }
           else if(node.op == OP_EQ){
               tmp_Value = builder->create_icmp_eq(lvalue, rvalue);
           }
           else{
               tmp_Value == builder->create_icmp_ne(lvalue, rvalue);
           }
        }
        else{
            if(node.op == OP_LT){
               tmp_Value = builder->create_fcmp_lt(lvalue, rvalue);
           }
           else if(node.op == OP_LE){
               tmp_Value = builder->create_fcmp_le(lvalue, rvalue);
           }
           else if(node.op == OP_GT){
               tmp_Value == builder->create_fcmp_gt(lvalue, rvalue);
           }
           else if(node.op == OP_GE){
               tmp_Value == builder->create_fcmp_ge(lvalue, rvalue);
           }
           else if(node.op == OP_EQ){
               tmp_Value = builder->create_fcmp_eq(lvalue, rvalue);
           }
           else{
               tmp_Value == builder->create_fcmp_ne(lvalue, rvalue);
           }
        }
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) { }

void CminusfBuilder::visit(ASTTerm &node) { }

void CminusfBuilder::visit(ASTCall &node) { }
