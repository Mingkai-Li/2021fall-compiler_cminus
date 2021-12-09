define dso_local i32 @callee(i32 %0) {
    %2 = alloca i32             ;ret val
    %3 = alloca i32             ;para int a
    store i32 %0, i32* %3

    %4 = load i32, i32* %3
    %5 = mul i32 2, %4

    store i32 %5, i32* %2
    ret i32 %5
}


define dso_local i32 @main() {
    %1 = alloca i32             ;ret val
    store i32 0, i32* %1

    %2 = call i32 @callee(i32 110)
    
    store i32 %2, i32* %1
    ret i32 %2
}