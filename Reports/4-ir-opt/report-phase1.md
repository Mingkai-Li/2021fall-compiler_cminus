# Lab 4 实验报告-阶段一

组长 汤力宇 PB19111717

组员 李铭铠 PB19111701

## 实验要求

- 阅读`LoopSearch`实验代码，回答思考题；
- 阅读`Mem2Reg`实验代码以及“静态单赋值格式构造.pdf”，回答思考题。



## 思考题

### LoopSearch

#### 1

`LoopSearch`中直接用于描述一个循环的数据结构是什么？需要给出其具体类型。

**Answer:**

在`Class LoopSearch`中，循环用`BBset_t`这一数据结构进行描述，其定义如下。即记录了这一循环中所有基本块的指针。

```cpp
using BBset_t = std::unordered_set<BasicBlock *>;
```

另外，在辅助函数中，循环使用了另一数据结构`CFGNodePtrSet`进行描述，在`void LoopSearch::run()`中才被转化为`BBset_t`的结构。`CFGNodePtrSet`的定义如下。相比于`BBset_t`，该数据结构记录了基本块在CFG中的前驱、后继节点，以及在Tarjan算法中用到的一些辅助信息。

```cpp
using CFGNodePtrSet = std::unordered_set<CFGNode*>;

struct CFGNode
{
    std::unordered_set<CFGNodePtr> succs;
    std::unordered_set<CFGNodePtr> prevs;
    BasicBlock *bb;
    int index;   // the index of the node in CFG
    int lowlink; // the min index of the node in the strongly connected componets
    bool onStack;
};
```

#### 2

循环入口是重要的信息，请指出`LoopSearch`中如何获取一个循环的入口？需要指出具体代码，并解释思路。

**Answer:**

`LoopSearch`利用`CFGNodePtr LoopSearch::find_loop_base(CFGNodePtrSet *set, CFGNodePtrSet &reserved)`函数来获取一个循环的入口。该函数的设计是基于两个前提来进行的：

1. `set`所指向的数据结构必须代表一个循环。函数没有对`set`是否真正代表循环进行检查，需要在调用函数时进行保证。在实验框架的实现中，`set`传入的值均由函数`strongly_connected_components`得到，因为不会出现`set`不是循环的情况；

2. CFG的结构在`void LoopSearch::run()`中遭到了破坏（当然，这一破坏是为了寻找内层循环，是合理的），因而需要使用额外的信息（`reserved`）来辅助函数找到循环的入口。

首先，试考虑CFG在构建结束后始终保持完整（未遭到破坏）的情况。此时，对于一个循环而言，除了该循环入口（`base`）的前驱可能在循环之外，其他所有的基本块的前驱均在循环内（这是由循环的定义决定的，类似的，除了循环出口的后继可能在循环外，其他所有基本块的后继均在循环内）。同时，循环入口（`base`）至少有一个前驱在循环外，否则该函数无法从初始的基本块进入该循环。所以，在`PART A`中，函数对循环中基本块进行遍历，如果该基本块存在循环外的前驱，则其为循环入口。这在CFG始终完整的情况下已经足够。

但是，CFG在函数`void LoopSearch::run()`中遭到了破坏，在CFG中完全删除了外部循环入口的所有信息。试考虑该外部循环的一个下一层的内部循环，该内部循环入口的一个前驱正好是外部循环入口。如果该内部循环入口没有其他前驱的话，则此时内部循环入口没有任何前驱，无法通过`PART A`的算法判断是否为循环入口。此时则需要利用额外维护的`reserved`集合进行判断。`reserved`集合记录了所有已经找到的`base`（循环入口）。那么，如果在当前循环里的一个基本块是已经找到的一个循环入口的后继的话，则它也必然是一个新的循环入口，即当前循环的循环入口。这部分的运算逻辑由`PART B`来执行。

```cpp
CFGNodePtr LoopSearch::find_loop_base(
    CFGNodePtrSet *set,
    CFGNodePtrSet &reserved)
{

    // PART A
    CFGNodePtr base = nullptr;
    for (auto n : *set)
    {
        for (auto prev : n->prevs)
        {
            if (set->find(prev) == set->end())
            {
                base = n;
            }
        }
    }
    if (base != nullptr)
        return base;
    // PART B
    // only when base has no prev due to destruction in run()
    for (auto res : reserved)
    {
        for (auto succ : res->succs)
        {
            if (set->find(succ) != set->end())
            {
                base = succ;
            }
        }
    }

    return base;
}
```

#### 3

仅仅找出强连通分量并不能表达嵌套循环的结构。为了处理嵌套循环，`LoopSearch`在Tarjan algorithm的基础之上做了什么特殊处理？

**Answer:**

`LoopSearch`并没有改进Tarjan算法，该算法由函数`strongly_connected_components`实现，仍然只能够找到当前的CFG中所有不相交的最外层循环（即当前CFG的所有不是单个结点的强连通分量）。为了找到最外层循环中可能嵌套的内层循环，`LoopSearch`在`run`的`step 6`中破坏了CFG的结构。具体的说，在CFG中删去了所有已经发现的`base`基本块。在经过了这样的处理后，由于外层循环缺少了一个结点，不再保持原来的强连通性质，再次调用函数`strongly_connected_components`所得到的强连通分量均是之前被嵌套的内层循环。如此不断重复地删去已经发现的所有`base`，重复调用函数`strongly_connected_components`直到不再有强连通分量，则可以到达所有的内外层循环。

```cpp
						// step 6: remove loop base node for researching inner loop
                        reserved.insert(base);
                        dump_graph(*scc, func->get_name() + '_' + std::to_string(scc_index));
                        nodes.erase(base);
                        for (auto su : base->succs)
                        {
                            su->prevs.erase(base);
                        }
                        for (auto prev : base->prevs)
                        {
                            prev->succs.erase(base);
                        }
```

#### 4

某个基本块可以属于多层循环中，`LoopSearch`找出其所属的最内层循环的思路是什么？这里需要用到什么数据？这些数据在何时被维护？需要指出数据的引用与维护的代码，并简要分析。

**Answer:**

为了找到基本块对应的最内层循环，`LoopSearch`维护了两个数据结构。如下所示。

```cpp
	// { entry bb of loop : loop }
    std::unordered_map<BasicBlock *, BBset_t *> base2loop;
	// { bb :  entry bb of loop} 默认最低层次的loop
    std::unordered_map<BasicBlock *, BasicBlock *> bb2base;
```

其中`base2loop`记录了循环入口与循环的一一对应关系，会在找到循环的循环入口（`base`）时进行插入更新。由于一个循环对应的`base`只存在一个，`base2loop`每个条目在创建后不会再发生改变。

与之不同的是，`bb2base`的每个条目可能会在for循环中被更新多次，由新发现的`base`替换旧发现的`base`。这是因为，一个基本块会在嵌套的循环中多次出现，那么每一次在这些循环找到`base`时，都会将这个`base`作为`bb2base`中该基本块的最新值。对该数据结构的维护在`run`的`step 5`中实现。同时，可以发现，在for循环中，外层循环会先于内层循环进行处理。那么`bb2base`中每个条目的最新的`base`都是当前最内部的循环的入口。在for循环结束时，则对应了最内层的循环入口。再通过`base2loop`找到该`base`对应的循环，则可以得到该基本块对应的最内层循环。

```cpp
						// step 5: map each node to loop base
                        for (auto bb : *bb_set)
                        {
                            if (bb2base.find(bb) == bb2base.end())
                                bb2base.insert({bb, base->bb});
                            else
                                bb2base[bb] = base->bb;
                        }
```



### Mem2reg

#### 1

请**简述**概念：支配性、严格支配性、直接支配性、支配边界。

**Answer:**

- 支配性
  - a∈Dom(n) 当且仅当 即若从初始结点起，每条到达n的路径，都必然经过a
- 严格支配性
  - a严格支配n 当且仅当 a∈Dom(n)，且a≠n
- 直接支配性
  - a=IDom(n)，当且仅当 a在CFG中是离n最近的，严格支配n的节点
- 支配边界
  - a∈DF(n),当且仅当
  1.  在CFG中，a有多个前驱节点（a是汇合点）（含于2,3条）
  2.  n支配a的一个前驱节点
  3.  n不严格支配a


#### 2

`phi`节点是SSA的关键特征，请**简述**`phi`节点的概念，以及引入`phi`节点的理由。

**Answer:**

- 概念
  - 是一个声明语句：在汇合节点，从多个前驱节点中选择正确的SSA变量名的函数
- 理由
  - SSA形式下，不同位置上的同一个变量的定值，将对应不同的变量名
  - CFG中，若一个汇合节点的多个前驱节点中都有对某变量（如x）的定值，则在ssa形式下将会产生多个x对应的变量名，则在该汇合节点引用变量x时，就必须在这些变量名间做出选择


#### 3

下面给出的`cminus`代码显然不是SSA的，后面是使用lab3的功能将其生成的LLVM IR（**未加任何Pass**），说明对一个变量的多次赋值变成了什么形式？

**Answer:**

- 对每一条赋值语句，产生了一个新的变量
  - 该新变量替代了：该赋值语句中的左值，以及旧变量在后续语句中的出现
- 对同一个变量的多次赋值，产生了对应的一系列新变量

#### 4

对下面给出的`cminus`程序，使用lab3的功能，分别关闭/开启`Mem2Reg`生成LLVM IR。对比生成的两段LLVM IR，开启`Mem2Reg`后，每条`load`, `store`指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

**Answer:**

- func()内
 - 对函数参数的store和load，变化没了
   - 即不将函数另外store于新空间，故在使用时也不需再load出来
 - if语句内对x的sotre，变化没了
   - x=0的常量赋值，通过在后续的基本块中的phi函数实现
 - 函数返回时对x的load，变化没了
   - 通过phi函数实现定值
- main()内
 - 对全局变量globVar的赋值store和读取load，没有变化
 - 对数组元素的赋值store，没有变化
 - 对变量b的load和store，变化没了
   - main中，变量b被定值为常量，并仅在过程调用时作为参数传递时引用
   - 故在常量合并，死代码删除优化后变没了


#### 5

指出放置phi节点的代码，并解释是如何使用支配树的信息的。需要给出代码中的成员变量或成员函数名称。

**Answer:**

- 代码如下
```cpp
    std::map<std::pair<BasicBlock *,Value *>, bool> bb_has_var_phi; // bb has phi for var
    for (auto var : global_live_var_name )
    {
        std::vector<BasicBlock *> work_list;
        work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
        for (int i =0 ; i < work_list.size() ; i++ )
        {   
            auto bb = work_list[i];
            for ( auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb))
            {
                if ( bb_has_var_phi.find({bb_dominance_frontier_bb, var}) == bb_has_var_phi.end() )
                { 
                    // generate phi for bb_dominance_frontier_bb & add bb_dominance_frontier_bb to work list
                    auto phi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb);
                    phi->set_lval(var);
                    bb_dominance_frontier_bb->add_instr_begin( phi );
                    work_list.push_back( bb_dominance_frontier_bb );
                    bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
                }
            }
        }
    }
```
- 其中if语句内部执行了实际的插入操作
  - 创建phi函数，并设置左值var
  - 将指令插入到基本快的开始处
  - 再将本基本快加入work_list集合（因为刚刚插入的phi函数，也是对变量var的一个定值语句）
```cpp
auto phi = PhiInst::create_phi(var->get_type()->get_poi
phi->set_lval(var);
bb_dominance_frontier_bb->add_instr_begin( phi );
work_list.push_back( bb_dominance_frontier_bb );
bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
```

- 支配树信息：代码通过支配树，得到了基本块的支配边界对应的基本块集合
```cpp
auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb)
```
- 成员变量：dominators_
  - Mem2Reg的私有成员，是一个class Dominators类实例
  - 对应当前正在处理的基本块的支配树
- 成员函数：get_dominance_frontier(bb)
  - class Dominators类的公有方法，由成员变量dominators_调用
  - 得到基本块bb的支配边界对应的基本块集（通过bb_dominance_frontier_bb遍历）




### 代码阅读总结

- 了解了Tarjan算法，对循环的构造以及嵌套循环的查找方式进行了阅读和了解
- 对CFG、支配树（以及相关的支配性、支配边界等概念）进行了了解
- 对SSA格式构造、phi函数的构造和插入方法等进行了了解



### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
