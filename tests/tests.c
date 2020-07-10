// -*- c -*-

// line comments and block comments
// This is a line comment.

/*
 * This is a block comment.
 */

int g1;
int g2[4];

int assert(int expected, int actual, char *code) {
    if(expected == actual) {
        printf("%s => %d\n", code, actual);
    } else {
        printf("%s => %d expected but got %d\n", code, expected, actual);
        exit(1);
    }
}

// 複数のreturn文の場合、最初の行の実行結果が出力に反映される
int ret3() { 
    return 3;
    return 5;
}

int ret5() {
    return 5;
}

int add2(int x, int y) {
    return x+y;
}

int sub2(int x, int y) {
    return x-y;
}

int add6(int a, int b, int c, int d, int e, int f) { 
    return a+b+c+d+e+f;
}

// 複数行に対応、return文の実装
int fib(int x) {
    if (x <= 1) {
        return 1;
    }
    return fib(x-1) + fib(x-2);
}

int foo(int *x, int y) {
    return *x + y;
}

int main() {
    // Handle block scope
    assert(2, ({ int x = 2; { int x = 3; } x; }), "({ int x = 2; { int x = 3; } x; })");
    assert(2, ({ int x = 2; { int x = 3; } int y = 4; x; }), "({ int x = 2; { int x = 3; } int y = 4; x; })");
    assert(3, ({ int x = 2; { x = 3; } x; }), "({ int x = 2; { x = 3; } x; })");
    // statement-expression: GNU C extension
    assert(2, ({ 0; 1; 2; }), "({ 0; 1; 2; })");
    assert(2, ({ 0; ({1;}); 2; }), "({ 0; ({1;}); 2; })");
    assert(2, ({ 0; ({1-1;}); 2; }), "({ 0; ({1-1;}); 2; })");
    assert(3, ({ int x=3; x; }), "({ int x=3; x; })");
    assert(1, ({1;}), "({1;})");
    assert(6, ({1;}) + ({2;}) + ({3;}), "({1;}) + ({2;}) + ({3;})");
    assert(119, ({"\x77"[0];}), "({\"\\x77\"[0];})");
    // \x<hexadecimal-sequence>
    assert(0, "\x00"[0], "\"\\x00\"[0]");
    assert(119, "\x77"[0], "\"\\x77\"[0]");
    assert(127, "\x7F"[0], "\"\\x7F\"[0]");
    // 以下3つは負の値になってしまう
    // assert(143, "\x8F"[0], "\"\\x8F\"[0]"); // \x8F = 10001111
    // assert(165, "\xA5"[0], "\"\\xA5\"[0]"); // \xA5 = 10100101
    // assert(255, "\x00ff"[0], "\"\\x00ff\"[0]"); // \x00ff = 11111111
    // \<octal-sequence>
    assert(0, "\0"[0], "\"\\0\"[0]");
    assert(16, "\20"[0], "\"\\20\"[0]");
    assert(65, "\101"[0], "\"\\101\"[0]");
    assert(104, "\1500"[0], "\"\\1500\"[0]");
    // string literal + escape sequence
    assert(7, "\ax\ny"[0], "\"\\ax\\ny\"[0]");
    assert(120, "\ax\ny"[1], "\"\\ax\\ny\"[1]");
    assert(10, "\ax\ny"[2], "\"\\ax\\ny\"[2]");
    assert(121, "\ax\ny"[3], "\"\\ax\\ny\"[3]");

    assert(7, "\a"[0], "\"\\a\"[0]");
    assert(8, "\b"[0], "\"\\b\"[0]");
    assert(9, "\t"[0], "\"\\t\"[0]");
    assert(10, "\n"[0], "\"\\n\"[0]");
    assert(11, "\v"[0], "\"\\v\"[0]");
    assert(12, "\f"[0], "\"\\f\"[0]");
    assert(13, "\r"[0], "\"\\r\"[0]");
    assert(27, "\e"[0], "\"\\e\"[0]");
    assert(0, "\0"[0], "\"\\0\"[0]");

    assert(106, "\j"[0], "\"\\j\"[0]");
    assert(107, "\k"[0], "\"\\k\"[0]");
    assert(108, "\l"[0], "\"\\l\"[0]");
    // string literal
    assert(92, queenssol8(), "queenssol8()");
    assert(97, "abc"[0], "\"abc\"[0]");
    assert(98, "abc"[1], "\"abc\"[1]");
    assert(99, "abc"[2], "\"abc\"[2]");
    assert(0,  "abc"[3], "\"abc\"[3]"); // "\0"
    assert(102, ({ char *a = "foo"; char *b = "bar"; a[0]; }), "({ char *a = \"foo\"; char *b = \"bar\"; a[0]; })");
    assert(4, sizeof("abc"), "sizeof(\"abc\")");
    assert(97, ({ char *g; { g = "a"; char *g = "b"; } g[0]; }), "({ char *g; { g = \"a\"; char *g = \"b\"; g[0]; } })");
    assert(3, ({ char g[3]; sizeof(g); }), "({ char g[3]; sizeof(g); })");
    assert(8, ({ char *g; sizeof(g); }), "({ char *g; sizeof(g); })");
    // char
    assert(1, ({ char x=1; x; }), "({ char x=1; x; })");
    // global variable
    assert(0, g1, "g1");
    g1=3;
    assert(3, g1, "g1");

    g2[0] = 0; g2[1] = 1; g2[2] = 2; g2[3] = 3;
    assert(0, g2[0], "g2[0]");
    assert(1, g2[1], "g2[1]");
    assert(2, g2[2], "g2[2]");
    assert(3, g2[3], "g2[3]");

    assert(8, sizeof(g1), "sizeof(g1)");
    assert(32, sizeof(g2), "sizeof(g2)");

    assert(0, ({int x; x; }), "({int x; x; })");
    assert(0, ({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[0]; }), "({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[0]; })");
    assert(1, ({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[1]; }), "({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[1]; })");
    assert(2, ({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[2]; }), "({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[2]; })");
    assert(3, ({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[3]; }), "({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[3]; })");

    assert(8, ({ int x;  sizeof(x); }), "({ int x;  sizeof(x); })");
    assert(32, ({ int x[4];  sizeof(x); }), "({ int x[4];  sizeof(x); })");

    // sizeof
    assert(8, ({ int x; sizeof(x); }), "({ int x; sizeof(x); })");
    assert(8, ({ int x; sizeof x; }), "({ int x; sizeof x; })");
    assert(8, ({ int *x; sizeof(x); }), "({ int *x; sizeof(x); })");
    assert(32, ({ int x[4]; sizeof(x); }), "({ int x[4]; sizeof(x); })");
    assert(96, ({ int x[3][4]; sizeof(x); }), "({ int x[3][4]; sizeof(x); })"); // 8*3*4
    assert(32, ({ int x[3][4]; sizeof(*x); }), "({ int x[3][4]; sizeof(*x); })"); // 8*4
    assert(8, ({ int x[3][4]; sizeof(**x); }), "({ int x[3][4]; sizeof(**x); })"); // 8*1
    assert(9, ({ int x[3][4]; sizeof(**x) + 1; }), "({ int x[3][4]; sizeof(**x) + 1; })");
    assert(9, ({ int x[3][4]; sizeof **x + 1; }), "({ int x[3][4]; sizeof **x + 1; })");
    assert(8, ({ int x[3][4]; sizeof(**x + 1); }), "({ int x[3][4]; sizeof(**x + 1); })");
    // []operator
    assert(0, ({ int x[2][3]; int *y = x; y[0] = 0; x[0][0]; }), "({ int x[2][3]; int *y = x; y[0] = 0; x[0][0]; })");
    assert(1, ({ int x[2][3]; int *y = x; y[1] = 1; x[0][1]; }), "({ int x[2][3]; int *y = x; y[1] = 1; x[0][1]; })");
    assert(2, ({ int x[2][3]; int *y = x; y[2] = 2; x[0][2]; }),  "({ int x[2][3]; int *y = x; y[2] = 2; x[0][2]; })");
    assert(3, ({ int x[2][3]; int *y = x; y[3] = 3; x[1][0]; }),  "({ int x[2][3]; int *y = x; y[3] = 3; x[1][0]; })");
    assert(4, ({ int x[2][3]; int *y = x; y[4] = 4; x[1][1]; }),  "({ int x[2][3]; int *y = x; y[4] = 4; x[1][1]; })");
    assert(5, ({ int x[2][3]; int *y = x; y[5] = 5; x[1][2]; }), "({ int x[2][3]; int *y = x; y[5] = 5; x[1][2]; })");
    // arrays of arrays
    assert(0, ({ int x[2][3]; int *y = x; *y = 0; **x; }), "({ int x[2][3]; int *y = x; *y = 0; **x; })");
    assert(1, ({ int x[2][3]; int *y = x; *(y+1) = 1; *(*(x+0)+1); }), "({ int x[2][3]; int *y = x; *(y+1) = 1; *(*(x+0)+1); })");
    assert(2, ({ int x[2][3]; int *y = x; *(y+2) = 2; *(*(x+0)+2); }), "({ int x[2][3]; int *y = x; *(y+2) = 2; *(*(x+0)+2); })");
    assert(3, ({ int x[2][3]; int *y = x; *(y+3) = 3; **(x+1); }), "({ int x[2][3]; int *y = x; *(y+3) = 3; **(x+1); })");
    assert(4, ({ int x[2][3]; int *y = x; *(y+4) = 4; *(*(x+1)+1); }), "({ int x[2][3]; int *y = x; *(y+4) = 4; *(*(x+1)+1); })");
    assert(5, ({ int x[2][3]; int *y = x; *(y+5) = 5; *(*(x+1)+2); }), "({ int x[2][3]; int *y = x; *(y+5) = 5; *(*(x+1)+2); })");
    // 配列の最後の要素+1はアドレスの演算まではできるが、dereferenceは未定義動作になる
    assert(1, ({ int x[2][3]; **(x+1) == *(*(x+0)+3); }), "({ int x[2][3]; **(x+1) == *(*(x+0)+3); })");
    assert(1, ({ int x[2][3]; int *y = x; *(y+3) == *(*(x+0)+3); }), "({ int x[2][3]; int *y = x; *(y+3) == *(*(x+0)+3); })");
    assert(1, ({ int x[2][3]; *(*(x+1)+3) == **(x+2); }), "({ int x[2][3]; *(*(x+1)+3) == **(x+2); })");
    assert(1, ({ int x[2][3]; int *y = x; *(y+6) == **(x+2); }), "({ int x[2][3]; int *y = x; *(y+6) == **(x+2); })");
    assert(1, ({ int x[2][3]; int *y = x; *(y+6) == *(*(x+1)+3); }), "({ int x[2][3]; int *y = x; *(y+6) == *(*(x+1)+3); })");
    // one dimensional arrays
    assert(3, ({ int x[2]; int *y = x; *y = 3; *x; }), "({ int x[2]; int *y = x; *y = 3; *x; })");
    assert(3, ({ int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; *x; }), "({ int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; *x; })");
    assert(4, ({ int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; *(x+1); }), "({ int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; *(x+1); })");
    assert(5, ({ int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; *(x+2); }), "({ int x[3]; *x = 3; *(x+1) = 4; *(x+2) = 5; *(x+2); })");
    assert(1, ({ int x[3]; int *y = x; *(y+2) == *(x+2); }), "({ int x[3]; int *y = x; *(y+2) == *(x+2); })");
    // "&"/"*"単項子の実装
    assert(3, ({ int foo = 3; *&foo; }), "({ int foo = 3; *&foo; })");
    assert(8, ({ int foo = 3; int bar = 5; (*&foo)+(*&bar); }), "({ int foo = 3; int bar = 5; (*&foo)+(*&bar); })");
    assert(0, ({ int x = 3; int y = &x; &x+1 == y+1; }), "({ int x = 3; int y = &x; &x+1 == y+1; })");
    assert(1, ({ int x = 3; int y = &x; &x+1 == y+(1*8); }),  "({ int x = 3; int y = &x; &x+1 == y+(1*8); })");
    assert(7, ({ int x = 3; int y = 5; *(&y-1) = 7; x; }), "({ int x = 3; int y = 5; *(&y-1) = 7; x; })");
    assert(8, ({ int x = 3; int y = 5; foo(&x, y); }), "({ int x = 3; int y = 5; foo(&x, y); })");
    // assert(3, ({ int x = 3; int y = &x; int z = &y; **z; }), "({ int x = 3; int y = &x; int z = &y; **z; })");　// このままだとinvalid pointer dereferenceのエラーになる
    // ポインタを代入する場合は*を変数名の前に付与する
    assert(3, ({ int x = 3; int *y = &x; int **z = &y; **z; }), "({ int x = 3; int *y = &x; int **z = &y; **z; })"); // **はポインタのポインタ
    // アドレスの演算は、実際には(x*8)している
    assert(5, ({ int x = 3; int y = 5; *(&x + 1); }), "({ int x = 3; int y = 5; *(&x + 1); })");
    assert(3, ({ int x = 3; int y = 5; *(&y - 1); }), "({ int x = 3; int y = 5; *(&y - 1); })");
    assert(22, ({ int x = 3; int y = 5; int z = 22; *(&x + 2); }), "({ int x = 3; int y = 5; int z = 22; *(&x + 2); })");
    assert(3, ({ int x = 3; int y = 5; int z = 22; *(&z - 2); }), "({ int x = 3; int y = 5; int z = 22; *(&z - 2); })");
    assert(5, ({ int x = 3; int *y = &x; *y = 5; x; }), "({ int x = 3; int *y = &x; *y = 5; x; })");
    assert(7, ({ int x = 3; int y = 5; *(&x+1) = 7; y; }), "({ int x = 3; int y = 5; *(&x+1) = 7; y; })");
    assert(7, ({ int x = 3; int y = 5; *(&y-1) = 7; x; }), "({ int x = 3; int y = 5; *(&y-1) = 7; x; })");
    // 引数ありの関数定義
    assert(3, add2(1, 2), "add2(1, 2)");
    assert(1, sub2(4, 3), "sub2(4, 3)");
    assert(7, ({ int a = 1; int b = 2; int c = 3; a + b + c + sub2(4, 3); }), "({ int a = 1; int b = 2; int c = 3; a + b + c + sub2(4, 3); })");
    assert(8, fib(5), "fib(5)");
    assert(55, fib(9), "fib(9)");
    // 関数呼び出し(引数6つまで)
    assert(21, add6(1,2,3,4,5,6), "add6(1,2,3,4,5,6)");
    assert(8, add2(3, 5), "add(3, 5)");
    assert(2, sub2(5, 3), "sub(5, 3)");
    // 関数呼び出し(引数なし)
    assert(3, ret3(), "ret3()");
    assert(5, ret5(), "ret5()");
    // block {...}
    assert(3, ({ 1; {2;} 3; }), "({ 1; {2;} 3; })");
    assert(0, ({ 0; }), "({ 0; })");
    // assert(0, ({ ; 0; }), "({ 0; })"); // expected expression
    // assert(0, ({}), "({})"); // expected expression
    // TODO: 後で確認
    // assert(5, ({ ;;; 5; }), "({ ;;; 5; })"); // expected expression
    /* 
       parse.c
       stmt2 = | "return" expr ";" | ... | "{" stmt* "}" | declaration | expr ";"
    */
    assert(55, ({ int i=0; int j=0; while(i <= 10) { j=i+j; i=i+1;} j; }), "({ int i=0; int j=0; while(i <= 10) { j=i+j; i=i+1;} j; })");
    assert(58, ({ int i=0; int j=0; int a; int b; for(; i <= 10; i=i+1) { a = 1; b = 2; j=j+i; } j+a+b; }), "({ int i=0; int j=0; int a; int b; for(; i <= 10; i=i+1) { a = 1; b = 2; j=j+i; } j+a+b; })");
    // for文
    assert(55, ({ int i=0; int j=0; for(; i <= 10; i=i+1) j=j+i; j; }), "({ int i=0; int j=0; for(; i <= 10; i=i+1) j=j+i; j; })");
    // while文
    assert(10, ({ int i = 0; while(i < 10) i=i+1; i; }), "({ int i = 0; while(i < 10) i=i+1; i; })");
    // if文
    assert(3, ({ int x; if(0) x = 2; else x = 3; x; }), "({ int x; if(0) x = 2; else x = 3; x; })");
    assert(3, ({ int x; if(1-1) x = 2; else x = 3; x; }), "({ int x; if(1-1) x = 2; else x = 3; x; })");
    assert(2, ({ int x; if(1) x = 2; else x = 3; x; }), "({ int x; if(1) x = 2; else x = 3; x; })");
    assert(2, ({ int x; if(2-1) x = 2; else x = 3; x; }), "({ int x; if(2-1) x = 2; else x = 3; x; })");
    // 複数文字のローカル変数
    assert(3, ({ int foo=3; foo; }), "({ int foo=3; foo; })");
    assert(8, ({ int foo123=3; int bar=5; foo123+bar; }), "({ int foo123=3; int bar=5; foo123+bar; })");
    assert(6, ({ int foo = 1; int bar = 2 + 3; foo + bar; }), "({ int foo = 1; int bar = 2 + 3; foo + bar; })");
    assert(12, ({ int a = 2; int b = 5 * 6 - 8; (a + b) / 2; }), "({ int a = 2; int b = 5 * 6 - 8; (a + b) / 2; })");
    // ローカル変数の実行(アルファベット一文字)
    assert(2, ({ int a=2; a; }), "({ int a=2; a; })");
    assert(2, ({ int a=2; int b=2; a; }), "({ int a=2; int b=2; a; })");
    assert(2, ({ int a = 2; int b = 2; b; }), "({ int a = 2; int b = 2; b; })");
    assert(8, ({ int a = 3; int z = 5; a + z; }), "({ int a = 3; int z = 5; a + z; })");

    assert(3, ({ 3; }), "({ 3; })");
    assert(5, ({ 5; }), "({ 5; })");
    assert(3, ({1; 2; 3;}), "({1; 2; 3;})");
    // 以下3個はエラー
    // assert(2, ({1; return 2; 3;}), "({1; return 2; 3;})"); // Error 2
    // assert(1, ({return 1; 2; 3;}), "({return 1; 2; 3;})"); // Error 1
    // assert(3, ({1; 2; return 3;}), "({1; 2; return 3;})"); // stmt-expr returning void is not supported
    // ture: 1, false: 0
    assert(1, 0!=1,  "0!=1");
    assert(0, 42!=42, "42!=42");
    assert(0, +8-(3+5), "+8-(3+5)");
    assert(1, +8==+8, "+8==+8");
    assert(0, +8!=+8, "+8!=+8");
    assert(1, +8!=-8, "+8!=-8");
    assert(0, +200<+200, "+200<+200");
    assert(1, +200<=+200, "+200<=+200");
    assert(1, +200<+201, "+200<+201");
    assert(1, +200<=+201, "+200<=+201");
    assert(1, +201>+200, "+201>+200");
    assert(1, +201>=+200, "+201>=+200");
    assert(0, +201<+200, "+201<+200");
    assert(0, +201<=+200, "+201<=+200");
    // 数値を入力してそれを返す/四則演算
    assert(10, - -10, "- -10");
    assert(10, - - +10, "- - +10");
    assert(22, +3*-5+37, "+3*-5+37");
    assert(100, -100+200, "-100+200");
    assert(4, (3+5)/2, "(3+5)/2");
    assert(15, 5*(9-6), "5*(9-6)");
    assert(47, 5+6*7, "5+6*7");
    assert(26, 2*3+4*5, "2*3+4*5");
    assert(41, 12 + 34 - 5 , "12 + 34 - 5 ");
    assert(21, 5+20-4, "5+20-4");
    assert(0, 0, "0");
    assert(15, 15, "15");

    printf("echo OK\n");
    return 0;
}