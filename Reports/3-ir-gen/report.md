# Lab3 实验报告

组长 汤力宇 PB19111717

组员 李铭铠 PB19111701


## 实验难点
- 如何处理左值右值
- 如何处理函数参数的作用域
- 如何确保基本快的结束

## 实验设计

1. 相关全局变量
```
Type* TyInt32;
Type* TyFloat;
Type* TyVoid;
Type* TyInt32Ptr;
Type* TyFloatPtr;
```
- 为了得到llvm ir的类型，设计如下全局变量以供使用
- 在最初处理ASTProgram节点的时候，将其初始化

```
Function* tmp_Function;       
```
- 对于函数，需要记录当前所在的函数

```
AllocaInst* tmp_AllocaInst;
```
- 用于保存当前函数的返回值的地址
- 在处理返回语句时（ASTReturnStmt节点），将返回值store到该位置

```
Type* tmp_Type;
std::string tmp_string;    
```
- 处理函数参数列表时，使用其传递参数的类型
- 处理函数的参数列表时，使用其传递参数的ID

```
Value* tmp_Value;
```
遍历ast时，用于储存对于子节点的返回值，

2. 难点

```
bool is_lvalue;
```
- 左值，右值的处理
  - 原则
    - 如果Var是左值，当且仅当其出现在赋值语句左边
      - 此时只需要将其地址返回（设置tmp_Value）
    - 其他情况下Var是右值
      - 此时需要添加load指令，将其值取出
  - 实现：设置全局变量is_lvalue
    - 当进入ASTAssignExpression节点是，将is_lvalue设置为true
    - 进入Var节点时，根据is_lvalue情况判断其是否是左右值
      - is_lvalue==ture，即左值
        - 只需要查询符号表，将其地址返回
        - 在ASTAssignExpression节点的后续处处理中，即可使用该地址进行生成store指令
        - 同时，需要在ASTVar节点将is_lvalue重新置为false，以防止数组取索引时用到的var也被当成左值
      - is_lvalue==ture，即右值
        - 查询符号表，得到其地址
        - 根据其地址，再生成对应的load执行，将其值取出


```
bool tmp_bool;
```
- 函数的参数传递
  - 难点：函数参数与之后跟随的复合语句，处于同一个作用域中
  - 解决：
      - 对于一般的复合语句
        - 在处理ASTCompoundStmt时进入新作用域
      - 对于函数声明中的复合语句
        - ASTFunDeclaration时就进入新作用域，并将函数参数压入
        - 之后的ASTCompoundStmt，不再需要进入新作用域
    - 实现：设计全局状态变量tmp_bool，表示是否需要进入新作用域
      - 在ASTFunDeclaration节点，进入作用于后，设置为false
      - 在ASTCompoundStmt节点，
        - 若其为false，则不需要再进入新作用域，并重置其为true
        - 若其为true，则进入新作用域
    - 对于推出作用域，做对应处理



### 实验总结

此次实验有什么收获

- 具体实践了访问者模式
- 详细了解了如何基于ast结构生成llvm ir这一转换过程



### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
