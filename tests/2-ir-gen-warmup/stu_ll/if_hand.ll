define dso_local i32 @main() #0 {
    %1 = alloca i32                     ;ret val
    %2 = alloca float                   ;float a 
    store i32 0, i32* %1
    store float 0x40163851E0000000, float* %2

    %3 = load float, float* %2
    %4 = fcmp ugt float %3, 1.000000e+00
    br i1 %4, label %5, label %6

5:
    store i32 233, i32* %1
    br label %6

6:
    %7 = load i32, i32* %1
    ret i32 %7
}

