define dso_local i32 @main() #0 {
    %1 = alloca i32         ;ret val
    %2 = alloca i32         ;a
    %3 = alloca i32         ;i
    store i32 0, i32* %1    ;init
    store i32 10, i32* %2   ;init
    store i32 0, i32* %3    ;init
    br label %4
4:
    %5 = load i32, i32* %3  ;load i
    %6 = icmp slt i32 %5, 10;cmp i<10
    br i1 %6, label %7, label %13

7:
    %8 = load i32, i32* %3  ;load i
    %9 = add i32 %8, 1      ;cal i+1
    store i32 %9, i32* %3   ;store i

    %10 = load i32, i32* %2 ;load a
    %11 = load i32, i32* %3 ;load i
    %12 = add i32 %10, %11  ;cal a+i
    store i32 %12, i32* %2  ;store a
    br label %4
13:
    %14 = load i32, i32* %2 ;load a
    store i32 %14, i32* %1  ;store ret val
    ret i32 %14
}

