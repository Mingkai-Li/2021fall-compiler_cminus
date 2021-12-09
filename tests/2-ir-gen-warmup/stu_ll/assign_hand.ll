define dso_local i32 @main() #0 {
    %1 = alloca i32                     ;ret val
    %2 = alloca [10 x i32]              ;int a[10]
    store i32 0, i32* %1

    %3 = getelementptr [10 x i32], [10 x i32]* %2, i32 0, i32 0     ;get a[0]
    store i32 10, i32* %3               ;store a[0]

    %4 = load i32, i32* %3              ;load a[0]
    %5 = mul i32 %4, 2                  ;cal a[0]*2

    %6 = getelementptr [10 x i32], [10 x i32]* %2, i32 0, i32 1     ;get a[1]
    store i32 %5, i32* %6               ;store a[1]

    ret i32 %5                          ;ret a[1]
}


