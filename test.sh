#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi  
}

# while文
assert 10 "i = 0; while(i < 10) i=i+1; return i;"
# if文
assert 3 'if(0) return 2; else return 3;'
assert 2 'if(1) return 2; else return 3;'
assert 3 'if(0) return 2; return 3;'
assert 2 'if(1) return 2; return 3;'
assert 3 'if(1-1) return 2; return 3;'
assert 3 'a = 0; if(a) return 2; return 3;'
# 複数文字のローカル変数
# 変数は定義なしに使えるものとする
assert 3 'foo=3; return foo;'
assert 8 'foo123=3; bar=5; return foo123+bar;'
assert 6 'foo = 1; bar = 2 + 3; return foo + bar;'
assert 12 'a = 2; b = 5 * 6 - 8; return (a + b) / 2;'
# ローカル変数の実行(アルファベット一文字)
assert 2 'a=2; return a;'
assert 2 'a=b=2; a;'
assert 2 'a = b = 2; b;'
assert 8 'a = 3; z = 5; a + z;'
# 複数行に対応、return文の実装
assert 5 'return 5; return 3;'
assert 1 'return 1; 2; 3;'
assert 2 '1; return 2; 3;'
assert 3 '1; 2; return 3;'
# 複数のreturn文の場合、最初の行の実行結果が出力に反映される
assert 1 'return +8*-5<-8+10;'
assert 0 'return +201<=+200; return +8*-5<-8+10;'
assert 0 'return +201<=+200; return +8*-5<-8+10; return 42;'
# ture: 1, false: 0
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'
assert 0 'return +8-(3+5);'
assert 1 'return +8==+8;'
assert 0 'return +8!=+8;'
assert 1 'return +8!=-8;'
assert 0 'return +200<+200;'
assert 1 'return +200<=+200;'
assert 1 'return +200<+201;'
assert 1 'return +200<=+201;'
assert 1 'return +201>+200;'
assert 1 'return +201>=+200;'
assert 0 'return +201<+200;'
assert 0 'return +201<=+200;'
# 数値を入力してそれを返す/四則演算
assert 10 'return - -10;'
assert 10 'return - - +10;'
assert 22 'return +3*-5+37;'
assert 100 'return -100+200;'
assert 4 'return (3+5)/2;'
assert 15 'return 5*(9-6);'
assert 47 'return 5+6*7;'
assert 26 'return 2*3+4*5;'
assert 41 'return  12 + 34 - 5 ;'
assert 21 'return 5+20-4;'
assert 42 'return 42;'
assert 0 'return 0;'

echo OK
