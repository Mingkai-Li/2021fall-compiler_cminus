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
std::string tmp_string;
bool tmp_bool;

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

void CminusfBuilder::visit(ASTNum &node) { }

void CminusfBuilder::visit(ASTVarDeclaration &node) { 
    Type* type;
    Value* value;

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

        if(FunctionType::is_valid_argument_type(tmp_Type)){
            types.push_back(tmp_Type);
        }
        else{
            LOG(ERROR)<< "invalid argument type";
            return;
        }

        ids.push_back(tmp_string);
    }

    function = Function::create(FunctionType::get(CminusType2Type(node.type), types), node.id, module.get());
    if(!scope.push(node.id, function)){
        LOG(ERROR) << "function name declared twice";
        return;
    }
    scope.enter();
    tmp_bool = false;
    builder->set_insert_point(BasicBlock::create(module.get(), node.id, function));

    for(auto iter=function->arg_begin(); iter != function->arg_end(); iter++) {
        values.push_back(*iter);
    }
    auto retAlloc = builder->create_alloca(CminusType2Type(node.type));
    tmp_AllocaInst = retAlloc;
    
    for(int i=0;i < values.size();i++){
        auto argAlloc = builder->create_alloca(types[i]);
        builder->create_store(values[i], argAlloc);
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
    
    if(node.isarray){
        tmp_Type = CminusType2TypePtr(node.type);
    }
    else{
        tmp_Type = CminusType2Type(node.type);
    }
}

void CminusfBuilder::visit(ASTCompoundStmt &node) { }

void CminusfBuilder::visit(ASTExpressionStmt &node) { }

void CminusfBuilder::visit(ASTSelectionStmt &node) { }

void CminusfBuilder::visit(ASTIterationStmt &node) { }

void CminusfBuilder::visit(ASTReturnStmt &node) { }

void CminusfBuilder::visit(ASTVar &node) { }

void CminusfBuilder::visit(ASTAssignExpression &node) { }

void CminusfBuilder::visit(ASTSimpleExpression &node) { }

void CminusfBuilder::visit(ASTAdditiveExpression &node) { }

void CminusfBuilder::visit(ASTTerm &node) { }

void CminusfBuilder::visit(ASTCall &node) { }
