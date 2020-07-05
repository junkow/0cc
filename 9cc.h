#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

//
// tokenizer.c
//

// トークンの種類
typedef enum {
    TK_RESERVED, // Keywords or punctuators 記号
    TK_IDENT,    // Itendefiers 識別子
    TK_STR,      // string literals 文字列リテラル
    TK_NUM,      // Integer literals 整数リテラル
    TK_EOF,      // End-of-file markers 入力の終わりを表すマーカー
} TokenKind;

// トークン型
typedef struct Token Token;
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン(連結リストのためのアドレス) ここでTokenを使っているから上で宣言してる?
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークン文字列の長さ

    char *contents; // 文字列リテラルのコンテンツ
    int cont_len;   // 文字列リテラルの長さ
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *peek(char *s);
Token *consume(char *op);
Token *consume_ident(void);
void expect(char *op);
long expect_number(void);
char *expect_ident(void);
bool at_eof(void);

Token *tokenize(void);

extern char *filename;
// 現在着目しているトークン
extern Token *token;
// 入力された文字列全体を受け取る変数
extern char *user_input;

//
// parse.c
//

// 変数を連結リストで表す
// Variable
typedef struct Var Var;
struct Var {
    char *name;     // 変数の名前
    Type *ty;       // Type
    bool is_local;  // local or global

    // for local variable
    int offset;     // RBPからのオフセット

    // Global variable
    char *contents;
    int cont_len;
};

// 変数のリストを表す構造体
typedef struct VarList VarList;
struct VarList {
    VarList *next;
    Var *var;
};

// トークン列を抽象構文木に変換する
// 抽象構文木(AST)のノードの種類
typedef enum {
    ND_ADD,       // num + num
    ND_PTR_ADD,   // ptr + num or num + ptr
    ND_SUB,       // num - num
    ND_PTR_SUB,   // ptr - num or num - ptr
    ND_PTR_DIFF,  // ptr - ptr
    ND_MUL,       // *
    ND_DIV,       // /
    ND_EQ,        // == : equal
    ND_NE,        // != : not equal
    ND_LT,        // <  : less than
    ND_LE,        // <= : less equal
    ND_ASSIGN,    // = : assign
    ND_ADDR,      // unary & (単項, アドレス)
    ND_DEREF,     // unary * (単項, 間接参照)
    ND_RETURN,    // "return"
    ND_IF,        // "if"
    ND_WHILE,     // "while"
    ND_FOR,       // "for"
    ND_BLOCK,     // { ... } compound-statement
    ND_FUNCALL,   // Function call
    ND_EXPR_STMT, // Expression statement
    ND_STMT_EXPR, // Statement expression
    ND_VAR,       // local variable
    ND_NUM,       // integer
    ND_NULL,      // Empty statement
} NodeKind;

// 抽象構文木(AST)のノードの型
typedef struct Node Node;
struct Node {
    NodeKind kind; // ノードの型
    Node *next;    // next node
    Type *ty;      // Type, e.g. int or pointer to int
    Token *tok;    // Representative token

    Node *lhs;     // 左辺 left-hand side
    Node *rhs;     // 右辺 right-hand side

    // "if" or "while" or "for" statement
    Node *cond;    // condition(条件)
    Node *then;
    Node *els;     // else
    Node *init;    // initialization(初期化式)
    Node *inc;     // increment

    // Block or statement expression
    // 複数のstatementをまとめてひとつのstatementにする
    // stmt()で展開した複数のnodeを連結リストで表して、その先頭のアドレス
    Node *body;

    // Function call
    char *funcname;
    Node *args; // 引数を連結リストで管理。そのリストの先頭のアドレス

    Var *var;      // kind == ND_VAR
    long val;      // kind == ND_NUM
};

typedef struct Function Function;
struct Function {
    Function *next;  // 次の関数
    char *name;      // 関数名
    VarList *params; // 関数の引数の連結リストの先頭アドレス

    Node *node;      // 関数内のNode
    VarList *locals; // (関数内のローカル変数+関数の引数)の連結リストの先頭のアドレス
    int stack_size;  // スタックサイズ
};

// トップレベルのitemについての型
typedef struct {
    VarList *globals; // List of global variables
    Function *fns;    // function
} Program;

Program *program(void);

//
// type.c
//

typedef enum { 
    TY_CHAR,
    TY_INT,
    TY_PTR,
    TY_ARRAY,
} TypeKind;

struct Type {
    TypeKind kind;
    int size;       // sizeof() value
    Type *base;
    int array_len;
};
// e.g.
// int a[2];と宣言した場合
// ty->kind = TY_ARRAY
// ty->base->kind = TY_INT

extern Type *char_type;
extern Type *int_type;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int len);
void add_type(Node *node);

//
// codegen.c
//

void codegen(Program *prog);

//
// main.c
//

extern FILE *output_file;
