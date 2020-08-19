// line comments and block comments
// This is a line comment.

/*
 * This is a block comment.
 */

// function declaration
int printf();
int exit();

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

int sub_short(short a, short b, short c) {
    return a - b - c;
}

int sub_long(long a, long b, long c) {
    return a - b - c;
}

int *g1_ptr() {
    return &g1;
}

int main() {
    // add void type
    { void *x; }
    // a return type to a function
    assert(0, g1, "g1");
    g1=3;
    assert(3, *g1_ptr(), "g1_ptr()");
    // nested type declarators
    assert(8, ({ int (*x)[3][4]; sizeof(x); }), "({ int (*x)[3][4]; sizeof(x); })");
    assert(96, ({ int *x[3][4]; sizeof(x); }), "({ int *x[3][4]; sizeof(x); })");
    assert(8, ({ int (*x)[3]; sizeof(x); }), "({ int (*x)[3]; sizeof(x); })");
    assert(24, ({ int *x[3]; sizeof(x); }), "({ int *x[3]; sizeof(x); })");
    assert(3, ({ int *x[3]; int y; x[0] = &y; y=3; x[0][0]; }), "({ int *x[3]; int y; x[0] = &y; y=3; x[0][0]; })");
    assert(4, ({ int x[3]; int (*y)[3]=x; y[0][0]=4; y[0][0]; }), "({ int x[3]; int (*y)[3]=x; y[0][0]=4; y[0][0]; })");

    // add short and long types
    assert(2, ({ short c; sizeof(c); }), "({ short c; sizeof(c); })");
    assert(4, ({ struct { char a; short b; } x; sizeof(x); }), "({ struct { char a; short b; } x; sizeof(x); })");

    assert(8, ({ long c; sizeof(c); }), "({ long c; sizeof(c); })");
    assert(16, ({ struct { char a; long b; } x; sizeof(x); }), "({ struct { char a; short b; } x; sizeof(x); })");

    assert(1, ({ short a = 5; short b = 3; short c = 1; sub_short(a, b, c); }), "({ short a = 5; short b = 3; short c = 1; sub_short(a, b, c); })");
    assert(1, ({ long a = 5; long b = 3; long c = 1; sub_long(a, b, c); }), "({ long a = 5; long b = 3; long c = 1; sub_long(a, b, c); })");
    // change the size of `int` from 8-byte to 4-byte
    // Struct assignment
    assert(3, ({ struct t {int a; int b;} x; struct t y; x.a=3; y=x; y.a; }), "({ struct t {int a; int b;} x; struct t y; x.a=5; y=x; y.a; })");
    assert(5, ({ struct t {int a; int b;} x; struct t y; x.b=5; y=x; y.b; }), "({ struct t {int a; int b;} x; struct t y; x.b=5; y=x; y.b; })");
    assert(7, ({ struct t {int a; int b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; }), "({ struct t {int a; int b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; })");
    assert(7, ({ struct t {int a; int b;}; struct t x; x.a=7; struct t y; struct t *p=&x; struct t *q=&y; *q=*p; y.a; }), "({ struct t {int a; int b;}; struct t x; x.a=7; struct t y; struct t *p=&x; struct t *q=&y; *q=*p; y.a; })");
    assert(5, ({ struct t {char a; char b;} x; struct t y; x.a=5; y=x; y.a; }), "({ struct t {char a; char b;} x; struct t y; x.a=5; y=x; y.a; })");
    assert(3, ({ struct t {int a; char b;} x; struct t y; x.b=3; y=x; y.b; }), "({ struct t {int a; char b;} x; struct t y; x.b=3; y=x; y.b; })");
    assert(3, ({ struct t {char a; int b;} x; struct t y; x.b=3; y=x; y.b; }), "({ struct t {int a; char b;} x; struct t y; x.b=3; y=x; y.b; })");
    // Union
    assert(8, ({ union {int a; char b[6];} x; sizeof(x); }), "({ union {int a; char b[6];} x; sizeof(x); })");
    assert(3, ({ union {int a; char b[4];} x; x.a = 515; x.b[0]; }), "({ union {int a; char b[4];} x; x.a = 515; x.b[0]; })");
    assert(2, ({ union {int a; char b[4];} x; x.a = 515; x.b[1]; }), "({ union {int a; char b[4];} x; x.a = 515; x.b[1]; })");
    assert(0, ({ union {int a; char b[4];} x; x.a = 515; x.b[2]; }), "({ union {int a; char b[4];} x; x.a = 515; x.b[1]; })");
    assert(0, ({ union {int a; char b[4];} x; x.a = 515; x.b[3]; }), "({ union {int a; char b[4];} x; x.a = 515; x.b[1]; })");
    // add "->" operator
    assert(3, ({ struct t {char a;} x; struct t *y = &x; x.a = 3; y->a; }), "({ struct t {char a;} x; struct t *y = &x; x.a = 3; y->a; })");
    assert(3, ({ struct t {char a;} x; struct t *y = &x; y->a=3; x.a; }), "({ struct t {char a;} x; struct t *y = &x; y->a=3; x.a; })");
    // add struct tag
    assert(8, ({ struct t {int a; int b;} x; struct t y; sizeof(y); }), "({ struct t {int a; int b;} x; struct t y; sizeof(y); })");
    assert(8, ({ struct t {int a; int b;}; struct t y; sizeof(y); }), "({ struct t {int a; int b;}; struct t y; sizeof(y); })");
    assert(2, ({ struct t {char a[2];}; { struct t {char a[4];}; } struct t y; sizeof(y); }), "({ struct t {char a[2];}; { struct t {char a[4];}; } struct t y; sizeof(y); })");
    assert(3, ({ struct t {int x;}; int t=1; struct t y; y.x=2; t + y.x; }), "({ struct t {int x;}; int t=1; struct t y; y.x=2; t + y.x; })");
    // align local variables
    assert(7, ({int x; char y; int a = &x; int b = &y; b-a;}), "({int x; char y; int a = &x; int b = &y; b-a;})");
    assert(1, ({char x; int y; int a = &x; int b = &y; b-a;}), "({int x; char y; int a = &x; int b = &y; b-a;})");
    // Struct(align members)
    assert(2, ({int x[5]; int *y = x+2; y-x;}), "({int x[5]; int *y = x+2; y-x;})");
 
    assert(1, ({struct {int a; int b;} x; x.a = 1; x.b = 2; x.a; }), "({struct {int a; int b;} x; x.a = 1; x.b = 2; x.a; })");
    assert(2, ({struct {int a; int b;} x; x.a = 1; x.b = 2; x.b; }), "({struct {int a; int b;} x; x.a = 1; x.b = 2; x.b; })");
    assert(1, ({struct {char a; int b; char c;} x; x.a = 1; x.b = 2; x.c = 3; x.a;}), "({struct {char a; int b; char c;} x; x.a = 1; x.b = 2; x.c = 3; x.a;})");
    assert(2, ({struct {char a; int b; char c;} x; x.a = 1; x.b = 2; x.c = 3; x.b;}), "({struct {char a; int b; char c;} x; x.a = 1; x.b = 2; x.c = 3; x.b;})");
    assert(3, ({struct {char a; int b; char c;} x; x.a = 1; x.b = 2; x.c = 3; x.c;}), "({struct {char a; int b; char c;} x; x.a = 1; x.b = 2; x.c = 3; x.c;})");

    assert(0, ({struct {char a; char b;} x[3]; char *p=x; p[0] = 0; x[0].a;}), "({struct {char a; char b;} x[3]; char *p=x; p[0] = 0; x[0].a;})");
    assert(1, ({struct {char a; char b;} x[3]; char *p=x; p[1] = 1; x[0].b;}), "({struct {char a; char b;} x[3]; char *p=x; p[1] = 1; x[0].b;})");
    assert(2, ({struct {char a; char b;} x[3]; char *p=x; p[2] = 2; x[1].a;}), "({struct {char a; char b;} x[3]; char *p=x; p[2] = 2; x[1].a;})");
    assert(3, ({struct {char a; char b;} x[3]; char *p=x; p[3] = 3; x[1].b;}), "({struct {char a; char b;} x[3]; char *p=x; p[3] = 3; x[1].b;})");

    assert(6, ({struct {char a[3]; char b[5];} x; char *p = &x; x.a[0]=6; p[0];}), "({struct {char a[3]; char b[5];} x; char *p = &x; x.a[0]=6; p[0];})");
    assert(7, ({struct {char a[3]; char b[5];} x; char *p = &x; x.b[0]=7; p[3];}), "({struct {char a[3]; char b[5];} x; char *p = &x; x.b[0]=7; p[3];})");

    assert(6, ({struct {struct {char b;} a;} x; x.a.b=6; x.a.b; }), "({struct {struct {char b;} a;} x; x.a.b=6; x.a.b; })");

    assert(4, ({struct {int a;} x; sizeof(x);}), "({struct {int a;} x; sizeof(x);})");
    assert(8, ({struct {int a; int b;} x; sizeof(x);}), "({struct {int a; int b;} x; sizeof(x);})");
    assert(12, ({struct {int a[3];} x; sizeof(x);}), "({struct {int a[3];} x; sizeof(x);})");
    assert(16, ({struct {int a;} x[4]; sizeof(x);}), "({struct {int a;} x[4]; sizeof(x);})");
    assert(24, ({struct {int a[3];} x[2]; sizeof(x);}), "({struct {int a[3];} x[2]; sizeof(x);})");
    assert(2, ({struct {char a; char b;} x; sizeof(x);}), "({struct {char a; char b;} x; sizeof(x);})");
    // 4の倍数でalignmentされる
    assert(8, ({struct {char a; int b;} x; sizeof(x);}), "({struct {int a; char b;} x; sizeof(x);})");
    assert(8, ({struct {int a; char b;} x; sizeof(x);}), "({struct {int a; char b;} x; sizeof(x);})");
    // Comma operator
    assert(3, (1,2,3), "(1,2,3)");
    assert(5, ({int i=2; int j=3; (i=5,j)=6; i; }), "({int i=2, j=3; (i=5,j)=6; i; })");
    assert(6, ({int i=2; int j=3; (i=5,j)=6; j; }), "({int i=2, j=3; (i=5,j)=6; j; })");
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
    assert(3, g1, "g1");

    g2[0] = 0; g2[1] = 1; g2[2] = 2; g2[3] = 3;
    assert(0, g2[0], "g2[0]");
    assert(1, g2[1], "g2[1]");
    assert(2, g2[2], "g2[2]");
    assert(3, g2[3], "g2[3]");

    assert(4, sizeof(g1), "sizeof(g1)");
    assert(16, sizeof(g2), "sizeof(g2)");

    assert(0, ({int x; x; }), "({int x; x; })");
    assert(0, ({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[0]; }), "({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[0]; })");
    assert(1, ({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[1]; }), "({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[1]; })");
    assert(2, ({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[2]; }), "({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[2]; })");
    assert(3, ({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[3]; }), "({ int x[4]; { x[0] = 0; x[1] = 1; x[2] = 2; x[3] = 3; } x[3]; })");

    assert(4, ({ int x;  sizeof(x); }), "({ int x;  sizeof(x); })");
    assert(16, ({ int x[4];  sizeof(x); }), "({ int x[4];  sizeof(x); })");

    // sizeof
    assert(4, ({ int x; sizeof(x); }), "({ int x; sizeof(x); })");
    assert(4, ({ int x; sizeof x; }), "({ int x; sizeof x; })");
    assert(8, ({ int *x; sizeof(x); }), "({ int *x; sizeof(x); })");
    assert(16, ({ int x[4]; sizeof(x); }), "({ int x[4]; sizeof(x); })");
    assert(48, ({ int x[3][4]; sizeof(x); }), "({ int x[3][4]; sizeof(x); })"); // 4*3*4
    assert(16, ({ int x[3][4]; sizeof(*x); }), "({ int x[3][4]; sizeof(*x); })"); // 4*4
    assert(4, ({ int x[3][4]; sizeof(**x); }), "({ int x[3][4]; sizeof(**x); })"); // 4*1
    assert(5, ({ int x[3][4]; sizeof(**x) + 1; }), "({ int x[3][4]; sizeof(**x) + 1; })");
    assert(5, ({ int x[3][4]; sizeof **x + 1; }), "({ int x[3][4]; sizeof **x + 1; })");
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
    assert(1, ({ int x = 3; int y = &x; &x+1 == &y; }),  "({ int x = 3; int y = &x; &x+1 == &y; })");
    // TODO: あとで確認
    // assert(1, ({ int x = 3; int y = &x; &x+1 == y+(1*4); }),  "({ int x = 3; int y = &x; &x+1 == y+(1*4); })");
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
