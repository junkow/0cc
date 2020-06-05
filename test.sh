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

    ./9cc "$input" > tmp.s
    # cc -o tmp tmp.s
    cc -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi  
}

# "&"/"*"単項子の実装
assert 3 'main() { foo = 3; return *&foo; }'
assert 8 'main() { foo = 3; bar = 5; return (*&foo)+(*&bar); }'
assert 0 'main() { x = 3; y = &x; return &x+1 == y+1; }'
assert 1 'main() { x = 3; y = &x; return &x+1 == y+(1*8); }'
assert 3 'main() { x = 3; y = &x; z = &y; return **z; }'
# アドレスの演算は、実際には(x*8)している
assert 5 'main() { x = 3; y = 5; return *(&x + 1); }'
assert 3 'main() { x = 3; y = 5; return *(&y - 1); }'
assert 22 'main() { x = 3; y = 5; z = 22; return *(&x + 2); }'
assert 3 'main() { x = 3; y = 5; z = 22; return *(&z - 2); }'
assert 5 'main() { x = 3; y = &x; *y = 5; return x; }'
assert 7 'main() { x = 3; y = 5; *(&x+1) = 7; return y; }'
assert 7 'main() { x = 3; y = 5; *(&y-1) = 7; return x; }'
# 引数ありの関数定義
assert 3 'main() { return add2(1, 2); } add2(x, y) { return x+y; }'
assert 1 'main() { return sub2(4, 3); } sub2(x, y) { return x-y; }'
assert 7 'main() { a = 1; b = 2; c = 3; return a + b + c + sub2(4, 3); } sub2(x, y) { return x-y; }'
assert 8 'main() { return fib(5); } fib(x) { if(x<=1) return 1; return fib(x-1) + fib(x-2); }'
# 引数なしの関数定義
assert 32 'main() { return ret32(); } ret32() { return 32; }'
# 関数呼び出し(引数6つまで)
assert 21 'main() { return add6(1,2,3,4,5,6); }'
assert 8 'main() { return add(3, 5); }'
assert 2 'main() { return sub(5, 3); }'
# 関数呼び出し(引数なし)
assert 3 'main() { return ret3(); }'
assert 5 'main() { return ret5(); }'
# # block {...}
assert 3 'main() { 1; {2;} return 3; }'
assert 55 'main() { i=0; j=0; while(i <= 10) { j=i+j; i=i+1;} return j; }'
assert 58 'main() { i=0; j=0; for(; i <= 10; i=i+1) { a = 1; b = 2; j=j+i; } return j+a+b; }'
# for文
assert 55 'main() { i=0; j=0; for(; i <= 10; i=i+1) j=j+i; return j; }'
assert 3 'main() { for(;;) return 3; return 5; }'
# while文
assert 10 'main() { i = 0; while(i < 10) i=i+1; return i; }'
# # if文
assert 5 'main() { if(0) if(0) return 10; else return 3; else return 5; }'
assert 5 'main() { if(0) if(1) return 10; else return 3; else return 5; }'
assert 3 'main() { if(1) if(0) return 10; else return 3; else return 5; }'
assert 3 'main() { if(0) return 2; else return 3; }'
assert 2 'main() { if(1) return 2; else return 3; }'
assert 3 'main() { if(0) return 2; return 3; }'
assert 2 'main() { if(1) return 2; return 3; }'
assert 3 'main() { if(1-1) return 2; return 3; }'
assert 3 'main() { a = 0; if(a) return 2; return 3; }'
# # 複数文字のローカル変数
# # 変数は定義なしに使えるものとする
assert 3  'main() { foo=3; return foo; }'
assert 8  'main() { foo123=3; bar=5; return foo123+bar; }'
assert 6  'main() { foo = 1; bar = 2 + 3; return foo + bar; }'
assert 12 'main() { a = 2; b = 5 * 6 - 8; return (a + b) / 2; }'
# # ローカル変数の実行(アルファベット一文字)
assert 2 'main() { a=2; return a; }'
assert 2 'main() { a=b=2; a; }'
assert 2 'main() { a = b = 2; b; }'
assert 8 'main() { a = 3; z = 5; a + z; }'
# # 複数行に対応、return文の実装
assert 5 'main() { return 5; return 3; }'
assert 1 'main() { return 1; 2; 3; }'
assert 2 'main() { 1; return 2; 3; }'
assert 3 'main() { 1; 2; return 3; }'
# # 複数のreturn文の場合、最初の行の実行結果が出力に反映される
assert 1 'main() { return +8*-5<-8+10; }'
assert 0 'main() { return +201<=+200; return +8*-5<-8+10; }'
assert 0 'main() { return +201<=+200; return +8*-5<-8+10; return 42; }'
# # ture: 1, false: 0
assert 1 'main() { return 0!=1; }'
assert 0 'main() { return 42!=42; }'
assert 0 'main() { return +8-(3+5); }'
assert 1 'main() { return +8==+8; }'
assert 0 'main() { return +8!=+8; }'
assert 1 'main() { return +8!=-8; }'
assert 0 'main() { return +200<+200; }'
assert 1 'main() { return +200<=+200; }'
assert 1 'main() { return +200<+201; }'
assert 1 'main() { return +200<=+201; }'
assert 1 'main() { return +201>+200; }'
assert 1 'main() { return +201>=+200; }'
assert 0 'main() { return +201<+200; }'
assert 0 'main() { return +201<=+200; }'
# # 数値を入力してそれを返す/四則演算
assert 10 'main() { return - -10; }'
assert 10 'main() { return - - +10; }'
assert 22 'main() { return +3*-5+37; }'
assert 100 'main() { return -100+200; }'
assert 4   'main() { return (3+5)/2; }'
assert 15 'main() { return 5*(9-6); }'
assert 47 'main() { return 5+6*7; }'
assert 26 'main() { return 2*3+4*5; }'
assert 41 'main() { return  12 + 34 - 5 ; }'
assert 21 'main() { return 5+20-4; }'
assert 42 'main() { return 42; }'
assert 0  'main() { return 0; }'

echo OK
