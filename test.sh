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

assert 0 0
assert 42 42
assert 21 '5+20-4'
assert 41 ' 12 + 34 - 5 '
assert 26 '2*3+4*5'
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 100 '-100+200'
assert 22 '+3*-5+37'
# ture: 1, false: 0
assert 0 '+8-(3+5)'
assert 1 '+8==+8'
assert 0 '+8!=+8'
assert 1 '+8!=-8'
assert 0 '+200<+200'
assert 1 '+200<=+200'
assert 1 '+200<+201'
assert 1 '+200<=+201'
assert 1 '+201>+200'
assert 1 '+201>=+200'
assert 0 '+201<+200'
assert 0 '+201<=+200'
assert 1 '+8*-5<-8+10'

echo OK
