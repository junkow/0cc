#!/bin/bash

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) { 
    return a+b+c+d+e+f;
}
EOF

assert() {
    expected="$1"
    input="$2"

    # ./9cc <(echo "$input") > tmp.s || exit
    # echo "$input" | ./9cc - > tmp.s || exit
    echo "$input" | ./9cc -o tmp.s - || exit
    # cc -o tmp tmp.s
    # gcc -xc -c -o tmp3.o ./example/8queensproblem.c
    gcc -static -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

# Handle block scope
assert 2 'int main() { int x = 2; { int x = 3; } return x; }'
assert 2 'int main() { int x = 2; { int x = 3; } {int y = 4; return x;} }'
assert 3 'int main() { int x = 2; { x = 3; } return x; }'
# line comments and block comments
assert 2 'int main() { /* return 1; */ return 2; }'
assert 2 'int main() { // return 1;
return 2; }'
assert 2 'int main() { /* return ({0; 1; 2;}); */ ({1; return 2; 3;}); }'
# statement-expression: GNU C extension
assert 0 'int main() { return ({ 0; }); }'
assert 2 'int main() { return ({ 0; 1; 2; }); }'
assert 1 'int main() { ({0; return 1; 2;}); return 3; }'
assert 3 'int main() { return ({int x=3; x; }); }'
assert 1 'int main() { return ({1;}); }'
assert 6 'int main() { return ({1;}) + ({2;}) + ({3;}); }'
assert 119 'int main() { return ({"\x77"[0];}); }'
# \x<hexadecimal-sequence>
assert 0 'int main() { return "\x00"[0]; }'
assert 119 'int main() { return "\x77"[0]; }'
assert 165 'int main() { return "\xA5"[0]; }'
assert 255 'int main() { return "\x00ff"[0]; }'
# \<octal-sequence>
assert 0 'int main() { return "\0"[0]; }'
assert 16 'int main() { return "\20"[0]; }'
assert 65 'int main() { return "\101"[0]; }'
assert 104 'int main() { return "\1500"[0]; }'
# string literal + escape sequence
assert 7 'int main() { return "\ax\ny"[0]; }'
assert 120 'int main() { return "\ax\ny"[1]; }'
assert 10 'int main() { return "\ax\ny"[2]; }'
assert 121 'int main() { return "\ax\ny"[3]; }'

assert 7 'int main() { return "\a"[0]; }'
assert 8 'int main() { return "\b"[0]; }'
assert 9 'int main() { return "\t"[0]; }'
assert 10 'int main() { return "\n"[0]; }'
assert 11 'int main() { return "\v"[0]; }'
assert 12 'int main() { return "\f"[0]; }'
assert 13 'int main() { return "\r"[0]; }'
assert 27 'int main() { return "\e"[0]; }'
assert 0 'int main() { return "\0"[0]; }'

assert 106 'int main() { return "\j"[0]; }'
assert 107 'int main() { return "\k"[0]; }'
assert 108 'int main() { return "\l"[0]; }'
# string literal
# assert 92 ' int main() { return queenssol8(); }'
assert 97 'int main() { return "abc"[0]; }'
assert 98 'int main() { return "abc"[1]; }'
assert 99 'int main() { return "abc"[2]; }'
assert 0 'int main() { return "abc"[3]; }' # '\0'
assert 102 'int main() { char *a = "foo"; char *b = "bar"; return a[0]; }'
assert 4 'int main() { return sizeof("abc"); }'
assert 98 'char *g; int main() { g = "a"; char *g = "b"; return g[0]; }'
assert 3 'char g[3]; int main() { return sizeof(g); }'
assert 8 'char *g; int main() { return sizeof(g); }'
# char
assert 1 'int main() { char x=1; return x; }'
# global variable
assert 0 'int x; int main() { return x; }'
assert 3 'int x; int main() { x = 3; return x; }'
assert 0 'int x[4]; int main() { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; return x[3]; }'

assert 8 'int x; int main() { return sizeof(x); }'
assert 32 'int x[4]; int main() { return sizeof(x); }'
# sizeof
assert 8 'int main() { int x; return sizeof(x); }'
assert 8 'int main() { int x; return sizeof x; }'
assert 8 'int main() { int *x; return sizeof(x); }'
assert 32 'int main() { int x[4]; return sizeof(x); }'
assert 96 'int main() { int x[3][4]; return sizeof(x); }' # 8*3*4
assert 32 'int main() { int x[3][4]; return sizeof(*x); }' # 8*4
assert 8 'int main() { int x[3][4]; return sizeof(**x); }' # 8*1
assert 9 'int main() { int x[3][4]; return sizeof(**x) + 1; }'
assert 9 'int main() { int x[3][4]; return sizeof **x + 1; }'
assert 8 'int main() { int x[3][4]; return sizeof(**x + 1); }'
# []operator
assert 0 'int main() { int x[2][3]; int *y = x; y[0] = 0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y = x; y[1] = 1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y = x; y[2] = 2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y = x; y[3] = 3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y = x; y[4] = 4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y = x; y[5] = 5; return x[1][2]; }'
# arrays of arrays
assert 0 'int main() { int x[2][3]; int *y = x; *y = 0; return **x; }'
assert 1 'int main() { int x[2][3]; int *y = x; *(y+1) = 1; return *(*(x+0)+1); }'
assert 2 'int main() { int x[2][3]; int *y = x; *(y+2) = 2; return *(*(x+0)+2); }'
assert 3 'int main() { int x[2][3]; int *y = x; *(y+3) = 3; return **(x+1); }'
assert 4 'int main() { int x[2][3]; int *y = x; *(y+4) = 4; return *(*(x+1)+1); }'
assert 5 'int main() { int x[2][3]; int *y = x; *(y+5) = 5; return *(*(x+1)+2); }'
# 配列の最後の要素+1はアドレスの演算まではできるが、dereferenceは未定義動作になる
assert 1 'int main() { int x[2][3]; return **(x+1) == *(*(x+0)+3); }'
assert 1 'int main() { int x[2][3]; int *y = x; return *(y+3) == *(*(x+0)+3); }'
assert 1 'int main() { int x[2][3]; return *(*(x+1)+3) == **(x+2); }'
assert 1 'int main() { int x[2][3]; int *y = x; return *(y+6) == **(x+2); }'
assert 1 'int main() { int x[2][3]; int *y = x; return *(y+6) == *(*(x+1)+3); }'
# one dimensional arrays
assert 3 'int main() { int x[2]; int *y = x; *y = 3; return *x; }'
assert 3 'int main() { int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; return *x; }'
assert 4 'int main() { int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; return *(x+2); }'
assert 1 'int main() { int x[3]; int *y = x; return *(y+2) == *(x+2); }'
# "&"/"*"単項子の実装
assert 3 'int main() { int foo = 3; return *&foo; }'
assert 8 'int main() { int foo = 3; int bar = 5; return (*&foo)+(*&bar); }'
assert 0 'int main() { int x = 3; int y = &x; return &x+1 == y+1; }'
assert 1 'int main() { int x = 3; int y = &x; return &x+1 == y+(1*8); }'
assert 7 'int main() { int x = 3; int y = 5; *(&y-1) = 7; return x; }'
assert 8 'int main() { int x = 3; int y = 5; return foo(&x, y); } int foo(int *x, int y) { return *x + y; }'
## assert 3 'int main() { int x = 3; int y = &x; int z = &y; return **z; }'　このままだとinvalid pointer dereferenceのエラーになる
## ポインタを代入する場合は*を変数名の前に付与する
assert 3 'int main() { int x = 3; int *y = &x; int **z = &y; return **z; }' # **はポインタのポインタ
## アドレスの演算は、実際には(x*8)している
assert 5 'int main() { int x = 3; int y = 5; return *(&x + 1); }'
assert 3 'int main() { int x = 3; int y = 5; return *(&y - 1); }'
assert 22 'int main() { int x = 3; int y = 5; int z = 22; return *(&x + 2); }'
assert 3 'int main() { int x = 3; int y = 5; int z = 22; return *(&z - 2); }'
assert 5 'int main() { int x = 3; int *y = &x; *y = 5; return x; }'
assert 7 'int main() { int x = 3; int y = 5; *(&x+1) = 7; return y; }'
assert 7 'int main() { int x = 3; int y = 5; *(&y-1) = 7; return x; }'
# 引数ありの関数定義
assert 3 'int main() { return add2(1, 2); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4, 3); } int sub2(int x, int y) { return x-y; }'
assert 7 'int main() { int a = 1; int b = 2; int c = 3; return a + b + c + sub2(4, 3); } int sub2(int x, int y) { return x-y; }'
assert 8 'int main() { return fib(5); } int fib(int x) { if(x<=1) return 1; return fib(x-1) + fib(x-2); }'
# 引数なしの関数定義
assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
# 関数呼び出し(引数6つまで)
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
# 関数呼び出し(引数なし)
assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
# block {...}
assert 3 'int main() { 1; {2;} return 3; }'
assert 55 'int main() { int i=0; int j=0; while(i <= 10) { j=i+j; i=i+1;} return j; }'
assert 58 'int main() { int i=0; int j=0; int a; int b; for(; i <= 10; i=i+1) { a = 1; b = 2; j=j+i; } return j+a+b; }'
# for文
assert 55 'int main() { int i=0; int j=0; for(; i <= 10; i=i+1) j=j+i; return j; }'
assert 3 'int main() { for(;;) return 3; return 5; }'
# while文
assert 10 'int main() { int i = 0; while(i < 10) i=i+1; return i; }'
# if文
assert 5 'int main() { if(0) if(0) return 10; else return 3; else return 5; }'
assert 5 'int main() { if(0) if(1) return 10; else return 3; else return 5; }'
assert 3 'int main() { if(1) if(0) return 10; else return 3; else return 5; }'
assert 3 'int main() { if(0) return 2; else return 3; }'
assert 2 'int main() { if(1) return 2; else return 3; }'
assert 3 'int main() { if(0) return 2; return 3; }'
assert 2 'int main() { if(1) return 2; return 3; }'
assert 3 'int main() { if(1-1) return 2; return 3; }'
assert 3 'int main() { int a = 0; if(a) return 2; return 3; }'
# 複数文字のローカル変数
assert 3  'int main() { int foo=3; return foo; }'
assert 8  'int main() { int foo123=3; int bar=5; return foo123+bar; }'
assert 6  'int main() { int foo = 1; int bar = 2 + 3; return foo + bar; }'
assert 12 'int main() { int a = 2; int b = 5 * 6 - 8; return (a + b) / 2; }'
# ローカル変数の実行(アルファベット一文字)
assert 2 'int main() { int a=2; return a; }'
assert 2 'int main() { int a=2; int b=2; return a; }'
assert 2 'int main() { int a = 2; int b = 2; return b; }'
assert 8 'int main() { int a = 3; int z = 5; return a + z; }'
# 複数行に対応、return文の実装
assert 3 'int main() { {1; {2;} return 3;} }'
assert 5 'int main() { return 5; return 3; }'
assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'
# 複数のreturn文の場合、最初の行の実行結果が出力に反映される
assert 1 'int main() { return +8*-5<-8+10; }'
assert 0 'int main() { return +201<=+200; return +8*-5<-8+10; }'
assert 0 'int main() { return +201<=+200; return +8*-5<-8+10; return 42; }'
# ture: 1, false: 0
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'
assert 0 'int main() { return +8-(3+5); }'
assert 1 'int main() { return +8==+8; }'
assert 0 'int main() { return +8!=+8; }'
assert 1 'int main() { return +8!=-8; }'
assert 0 'int main() { return +200<+200; }'
assert 1 'int main() { return +200<=+200; }'
assert 1 'int main() { return +200<+201; }'
assert 1 'int main() { return +200<=+201; }'
assert 1 'int main() { return +201>+200; }'
assert 1 'int main() { return +201>=+200; }'
assert 0 'int main() { return +201<+200; }'
assert 0 'int main() { return +201<=+200; }'
# 数値を入力してそれを返す/四則演算
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'
assert 22 'int main() { return +3*-5+37; }'
assert 100 'int main() { return -100+200; }'
assert 4   'int main() { return (3+5)/2; }'
assert 15 'int main() { return 5*(9-6); }'
assert 47 'int main() { return 5+6*7; }'
assert 26 'int main() { return 2*3+4*5; }'
assert 41 'int main() { return  12 + 34 - 5 ; }'
assert 21 'int main() { return 5+20-4; }'
assert 42 'int main() { return 42; }'
assert 0  'int main() { return 0; }'
assert 15  'int main() { return 15; }'

echo OK
