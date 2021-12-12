# Lab4 实验报告

组长 汤力宇 PB19111717

组员 李铭铠 PB19111701

## 实验要求

1. 完成**常量传播与死代码删除**优化pass的开发，并保证优化后时间减少超过规定值；

2. 完成**循环不变式外提**优化pass的开发，并保证优化后时间减少超过规定值；

3. 完成**活跃变量分析**pass，并保证生成的`.json`文件与给出结果一致。

## 实验难点

#### 常量传播



#### 循环不变式外提

- 确定那些语句不能外提；
- 判断是不是循环不变式；
- 代码语法问题：在for循环中不能对遍历的结构进行更改；

#### 活跃变量分析

- use、phi_use、def与基本块对应结构体的设计；
- 判断语句的右值中哪些是变量；
- 判断哪些语句的返回值是逻辑上的左值；



## 实验设计

### 常量传播
#### 实现思路：



#### 相应代码：



#### 优化前后的IR对比（举一个例子）并辅以简单说明：





### 循环不变式外提

#### 实现思路：

首先，被外提的语句应当不会产生副作用，经过分析，以下6条语句不能够被外提：

1. terminator: 如`br`语句和`ret`语句，如果进行外提的话，会导致原来的基本块结构不完整；
2. `phi`语句:`phi`语句的外提会导致该语句的前驱基本块发生变化，影响该语句的结果；
3. `call`语句:函数调用的外提会导致被调用函数中所有语句的整体外提，可能产生影响（如对全局变量）；
4. `load`语句:`load`语句的右值是变量的地址，即使地址不变，每次`load`的结果也很有可能会发生变化；
5. `store`语句:`store`语句的外提，使得该语句的执行先于一些对相同地址的`load`进行，可能会导致这些`load`的结果发生变化。

在确定以上语句不能被外提后，基本的思路是一次遍历扫描循环中的所有左值，二次遍历扫描语句的右值是否与某个左值相同，若没有与之相同的左值，提到这个循环在深度优先遍历下的前驱基本块。有2点需要注意：

1. 对一个循环不用重复进行一次遍历，在第一次进行后，只需要删去提出循环不变式的左值即可（由SSA保证左值互不相同）；
2. 一个外提导致和左值集合的改变，需要重复二次遍历直到无变化。

#### 相应代码：

一次遍历代码：

```cpp
			std::unordered_set<Value*> left_values;
            for(auto bb : *loop){
                for(auto inst : bb->get_instructions()){
                    left_values.insert(inst);
                }
            }
```

二次遍历代码：

```cpp
				for(auto bb : *loop){
                    for(auto inst : bb->get_instructions()){
                        // instructions that can't be hoisted
                        if(inst->is_br() || inst->is_ret()) continue;
                        if(inst->is_phi()) continue;
                        if(inst->is_call()) continue;
                        if(inst->is_load()) continue;
                        if(inst->is_store()) continue;

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
```

#### 优化前后的IR对比（举一个例子）并辅以简单说明：

优化前：

```c
; ModuleID = 'cminus'
source_filename = "testcase-1.cminus"

declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define void @main() {
label_entry:
  %op0 = alloca i32
  %op1 = alloca i32
  store i32 0, i32* %op0
  store i32 0, i32* %op1
  br label %label2
label2:                                                ; preds = %label_entry, %label7
  %op3 = load i32, i32* %op0
  %op4 = icmp slt i32 %op3, 100000000
  %op5 = zext i1 %op4 to i32
  %op6 = icmp ne i32 %op5, 0
  br i1 %op6, label %label7, label %label74
label7:                                                ; preds = %label2
  %op8 = add i32 1, 1
  %op9 = add i32 %op8, 1
  %op10 = add i32 %op9, 1
  %op11 = add i32 %op10, 1
  %op12 = add i32 %op11, 1
  %op13 = add i32 %op12, 1
  %op14 = add i32 %op13, 1
  %op15 = add i32 %op14, 1
  %op16 = add i32 %op15, 1
  %op17 = add i32 %op16, 1
  %op18 = add i32 %op17, 1
  %op19 = add i32 %op18, 1
  %op20 = add i32 %op19, 1
  %op21 = add i32 %op20, 1
  %op22 = add i32 %op21, 1
  %op23 = add i32 %op22, 1
  %op24 = add i32 %op23, 1
  %op25 = add i32 %op24, 1
  %op26 = add i32 %op25, 1
  %op27 = add i32 %op26, 1
  %op28 = add i32 %op27, 1
  %op29 = add i32 %op28, 1
  %op30 = add i32 %op29, 1
  %op31 = add i32 %op30, 1
  %op32 = add i32 %op31, 1
  %op33 = add i32 %op32, 1
  %op34 = add i32 %op33, 1
  %op35 = add i32 %op34, 1
  %op36 = add i32 %op35, 1
  %op37 = add i32 %op36, 1
  %op38 = add i32 %op37, 1
  %op39 = add i32 %op38, 1
  %op40 = add i32 %op39, 1
  store i32 %op40, i32* %op1
  %op41 = load i32, i32* %op0
  %op42 = load i32, i32* %op1
  %op43 = load i32, i32* %op1
  %op44 = mul i32 %op42, %op43
  %op45 = load i32, i32* %op1
  %op46 = mul i32 %op44, %op45
  %op47 = load i32, i32* %op1
  %op48 = mul i32 %op46, %op47
  %op49 = load i32, i32* %op1
  %op50 = mul i32 %op48, %op49
  %op51 = load i32, i32* %op1
  %op52 = mul i32 %op50, %op51
  %op53 = load i32, i32* %op1
  %op54 = mul i32 %op52, %op53
  %op55 = load i32, i32* %op1
  %op56 = mul i32 %op54, %op55
  %op57 = load i32, i32* %op1
  %op58 = load i32, i32* %op1
  %op59 = mul i32 %op57, %op58
  %op60 = load i32, i32* %op1
  %op61 = mul i32 %op59, %op60
  %op62 = load i32, i32* %op1
  %op63 = mul i32 %op61, %op62
  %op64 = load i32, i32* %op1
  %op65 = mul i32 %op63, %op64
  %op66 = load i32, i32* %op1
  %op67 = mul i32 %op65, %op66
  %op68 = load i32, i32* %op1
  %op69 = mul i32 %op67, %op68
  %op70 = load i32, i32* %op1
  %op71 = mul i32 %op69, %op70
  %op72 = sdiv i32 %op56, %op71
  %op73 = add i32 %op41, %op72
  store i32 %op73, i32* %op0
  br label %label2
label74:                                                ; preds = %label2
  %op75 = load i32, i32* %op1
  %op76 = load i32, i32* %op1
  %op77 = mul i32 %op75, %op76
  call void @output(i32 %op77)
  ret void
}
```

优化后：

```c
; ModuleID = 'cminus'
source_filename = "testcase-1.cminus"

declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define void @main() {
label_entry:
  %op0 = alloca i32
  %op1 = alloca i32
  store i32 0, i32* %op0
  store i32 0, i32* %op1
  %op8 = add i32 1, 1
  %op9 = add i32 %op8, 1
  %op10 = add i32 %op9, 1
  %op11 = add i32 %op10, 1
  %op12 = add i32 %op11, 1
  %op13 = add i32 %op12, 1
  %op14 = add i32 %op13, 1
  %op15 = add i32 %op14, 1
  %op16 = add i32 %op15, 1
  %op17 = add i32 %op16, 1
  %op18 = add i32 %op17, 1
  %op19 = add i32 %op18, 1
  %op20 = add i32 %op19, 1
  %op21 = add i32 %op20, 1
  %op22 = add i32 %op21, 1
  %op23 = add i32 %op22, 1
  %op24 = add i32 %op23, 1
  %op25 = add i32 %op24, 1
  %op26 = add i32 %op25, 1
  %op27 = add i32 %op26, 1
  %op28 = add i32 %op27, 1
  %op29 = add i32 %op28, 1
  %op30 = add i32 %op29, 1
  %op31 = add i32 %op30, 1
  %op32 = add i32 %op31, 1
  %op33 = add i32 %op32, 1
  %op34 = add i32 %op33, 1
  %op35 = add i32 %op34, 1
  %op36 = add i32 %op35, 1
  %op37 = add i32 %op36, 1
  %op38 = add i32 %op37, 1
  %op39 = add i32 %op38, 1
  %op40 = add i32 %op39, 1
  br label %label2
label2:                                                ; preds = %label_entry, %label7
  %op3 = load i32, i32* %op0
  %op4 = icmp slt i32 %op3, 100000000
  %op5 = zext i1 %op4 to i32
  %op6 = icmp ne i32 %op5, 0
  br i1 %op6, label %label7, label %label74
label7:                                                ; preds = %label2
  store i32 %op40, i32* %op1
  %op41 = load i32, i32* %op0
  %op42 = load i32, i32* %op1
  %op43 = load i32, i32* %op1
  %op44 = mul i32 %op42, %op43
  %op45 = load i32, i32* %op1
  %op46 = mul i32 %op44, %op45
  %op47 = load i32, i32* %op1
  %op48 = mul i32 %op46, %op47
  %op49 = load i32, i32* %op1
  %op50 = mul i32 %op48, %op49
  %op51 = load i32, i32* %op1
  %op52 = mul i32 %op50, %op51
  %op53 = load i32, i32* %op1
  %op54 = mul i32 %op52, %op53
  %op55 = load i32, i32* %op1
  %op56 = mul i32 %op54, %op55
  %op57 = load i32, i32* %op1
  %op58 = load i32, i32* %op1
  %op59 = mul i32 %op57, %op58
  %op60 = load i32, i32* %op1
  %op61 = mul i32 %op59, %op60
  %op62 = load i32, i32* %op1
  %op63 = mul i32 %op61, %op62
  %op64 = load i32, i32* %op1
  %op65 = mul i32 %op63, %op64
  %op66 = load i32, i32* %op1
  %op67 = mul i32 %op65, %op66
  %op68 = load i32, i32* %op1
  %op69 = mul i32 %op67, %op68
  %op70 = load i32, i32* %op1
  %op71 = mul i32 %op69, %op70
  %op72 = sdiv i32 %op56, %op71
  %op73 = add i32 %op41, %op72
  store i32 %op73, i32* %op0
  br label %label2
label74:                                                ; preds = %label2
  %op75 = load i32, i32* %op1
  %op76 = load i32, i32* %op1
  %op77 = mul i32 %op75, %op76
  call void @output(i32 %op77)
  ret void
}
```

我们可以很明显的看到，一大堆label 7中的加1的add语句被提到了label entry中，因为这些语句在循环中所得到的值不会发生任何的变化。



### 活跃变量分析

#### 实现思路：

一次遍历，得到def、use、phi_use集合。需要注意的有以下4点：

1. 是变量集合，所以如果是常量，不加入该集合；
2. def集合是语句左值的集合，有以下三个语句由于没有左值，不记录Instruction的返回值：(1)terminator，`br`和`ret`语句；(2)`store`语句；
3. use集合从instruction的操作数中取值，不考虑`phi`语句。对于`br`语句，如果是条件跳转，第一个操作数是变量，其他是label。对于其他语句，操作数（可能没有）都是变量；对于`call`语句，由于其第一个操作数是lable，故对其除了第一个操作数之外的其他操作数进行处理。
4. phi_use集合从phi的操作数中取值，需要记录变量对应的label。在操作数中，变量和对应的label先后依次出现。

二次遍历，执行ppt上伪代码：

<img src="./figs/code.png" style="zoom:50%;" />

其中OUT集合需要作出改变，改为：

$`OUT[B] =\cup_{s是B的后继}IN[S]\cup_{s是B的后继} phi\_uses[S,B]`$。

#### 相应的代码：

一次遍历代码

```cpp
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
                                    phi_use.insert({pre_bb, new_set});
                                }
                                
                                phi_use[pre_bb].insert(right_value);
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
                        else if(inst->is_call()){
                            bool first = true;
                            for(auto right_value : inst->get_operands()){
                                if(first){
                                    first = false;
                                    continue;
                                }
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
```

二次遍历代码

```cpp
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
                    std::set<Value*> live_in_before;
                    live_in_before.insert(live_in[bb].begin(), live_in[bb].end());

                    for(auto value : live_out[bb]){
                        live_in[bb].insert(value);
                    }
                    for(auto value : bb2def[bb]){
                        live_in[bb].erase(value);
                    }
                    for(auto value : bb2use[bb]){
                        live_in[bb].insert(value);
                    }

                    if(live_in[bb] != live_in_before){
                        flag = true;
                    }
                }
            }
```



### 实验总结

- 对编译器优化的过程有了进一步认识；
- 在SSA和LLVM的前提下，具体分析了不同类型语句在常量传播、死代码删除、循环不变式外提、活跃变量分析中的不同之处，对各语句在LLVM中的数据结构有了进一步认识。



### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
