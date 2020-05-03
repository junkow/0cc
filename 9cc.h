#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenizer.c
//

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークン型
typedef struct Token Token;
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン(連結リストのためのアドレス) ここでTokenを使っているから上で宣言してる?
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークン文字列の長さ
};

// 変数を連結リストで表す
// ローカル変数の型
typedef struct LVar LVar;
struct LVar {
    LVar *next;     // 次の変数かNULL
    char *name;     // 変数の名前
    int len;        // 変数の名前の長さ
    int offset;     // RBPからのオフセット
};

// ローカル変数
extern struct LVar *locals;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
Token *consume_ident(void);
void expect(char *op);
int expect_number(void);
bool at_eof(void);

Token *tokenize(void);

// global variables

// 現在着目しているトークン
extern Token *token;
// エラーを報告するための関数
// 入力された文字列全体を受け取る変数
extern char *user_input;

//
// parse.c
//

// トークン列を抽象構文木に変換する
// 抽象構文木(AST)のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // == : equal
    ND_NE,  // != : not equal
    ND_LT,  // <  : less than
    ND_LE,  // <= : less equal
    ND_NUM, // 整数
    ND_ASSIGN, // = : assign
    ND_LVAR,   // ローカル変数: local variable
} NodeKind;

// 抽象構文木のノードの型
typedef struct Node Node;
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs;     // 左辺 left-hand side
    Node *rhs;     // 右辺 right-hand side
    LVar *var;     // kind == ND_LVAR
    int offset;    // kind == ND_LVAR
    int val;       // kind == ND_NUM
};

Node *program(void);
extern Node *code[100];

//
// codegen.c
//

void gen(Node *node);
