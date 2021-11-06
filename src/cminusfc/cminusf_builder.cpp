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

std::vector<std::string> functions;

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
    functions.push_back("input");
    functions.push_back("output");
    functions.push_back("outputFloat");
    functions.push_back("neg_idx_except");
    
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

    if(scope.find(node.id) == nullptr){
        if(scope.in_global()){
            auto initializer = ConstantZero::get(type, module.get());
            value = GlobalVariable::create(node.id, module.get(), type, false, initializer);
        }
        else{
            value = builder->create_alloca(type);
        }

        scope.push(node.id, value);
    }
    else{
        LOG(ERROR) << "varible name declared twice in this scope";
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) { }

void CminusfBuilder::visit(ASTParam &node) { }

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
