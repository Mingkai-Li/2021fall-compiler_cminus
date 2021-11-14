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
bool return_bool;

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

CminusType TypeID2CminusType(int type){
    if(type == 2){
        return TYPE_INT;
    }
    else if(type == 6){
        return TYPE_FLOAT;
    }
    else{
        return TYPE_VOID;
    }
}

CminusType TypeTansfer(Value* lvalue, Value* rvalue, IRBuilder* builder, bool left_first){
    CminusType ltype = TypeID2CminusType(lvalue->get_type()->get_type_id());
    CminusType rtype = TypeID2CminusType(rvalue->get_type()->get_type_id());

    // error checking
    if(ltype == TYPE_VOID){
        LOG(ERROR) << "factor type is neither INT or FP";
        return ltype;
    }
    if(rtype == TYPE_VOID){
        LOG(ERROR) << "factor type is neither INT or FP";
        return rtype;
    }
    // same type
    if(ltype == rtype){
        return ltype;
    }

    // type transfer
    if(left_first){
        if(ltype == TYPE_INT){
            rvalue = builder->create_fptosi(rvalue, TyInt32);
        }
        else{
            rvalue = builder->create_sitofp(rvalue, TyFloat);
        }

        return ltype;
    }
    else{
        if(ltype == TYPE_INT){
            lvalue = builder->create_sitofp(lvalue, TyFloat);
        }
        else{
            rvalue = builder->create_sitofp(rvalue, TyFloat);
        }

        return TYPE_FLOAT;
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
    auto retAlloc = builder->create_alloca(CminusType2Type(node.type));
    tmp_AllocaInst = retAlloc;
    
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
    
    return_bool = false;
    for(auto statement : node.statement_list){
        statement->accept(*this);
        if(return_bool){
            return_bool = false;
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

void CminusfBuilder::visit(ASTReturnStmt &node) { }

void CminusfBuilder::visit(ASTVar &node) { 
    Value* value = scope.find(node.id);

    if(node.expression == nullptr){
        tmp_Value = value;
    }
    else{
        node.expression->accept(*this);
        tmp_Value = builder->create_gep(value, {CONST_INT(0), tmp_Value});
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) { 
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
