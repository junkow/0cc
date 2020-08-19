#include "9cc.h"

typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    char *name;
    int depth;
    Var *var;
};

// Scope for struct or union tags
typedef struct TagScope TagScope;
struct TagScope {
    TagScope *next;
    char *name;
    int depth;
    Type *ty;
};

// 変数をまとめるリスト
// ローカル変数と引数をまとめるリスト
// パーズ中に作成されたすべてのlocal variableインスタンスをまとめる
static VarList *locals;
// 同様にglobal変数をまとめるリスト
static VarList *globals;

// C has two block scopes; one is for variables and
// the other is for struct tags.
static VarScope *var_scope;
static TagScope *tag_scope;

// blockの始まりに、1だけincrementされる
// block scopeの終わりに、1だけdecrementされる
static int scope_depth;

static void enter_scope(void) {
    scope_depth++;
}

static void leave_scope(void) {
    scope_depth--;
    while(var_scope && var_scope->depth > scope_depth)
        var_scope = var_scope->next;

    while(tag_scope && tag_scope->depth > scope_depth)
        tag_scope = tag_scope->next;
}

// File a variable by name
// 連結リストから変数を名前で検索。見つからなかった場合はNULLを返す
// scopeの内側から外側へ変数を辿る
static Var *find_var(Token *tok) {
    // 内側のscopeから変数を辿り、なければ、その外側のscopeというように変数を辿る
    for(VarScope *sc = var_scope; sc; sc = sc->next) {
        if(strlen(sc->name) == tok->len && !strncmp(tok->str, sc->name, tok->len)) {
            // 変数名がリストから見つかったら、その位置のvar構造体のポインタを返す
            return sc->var;
        }
    }

    return NULL;
}

static TagScope *find_tag(Token *tok) {
    for(TagScope *tsc = tag_scope; tsc; tsc = tsc->next) {
        if(strlen(tsc->name) == tok->len && !strncmp(tok->str, tsc->name, tok->len))
            return tsc;
    }

    return NULL;
}

// 新しいノードを作成する関数
// 以下の2種類に合わせて関数を二つ用意する
// - 左辺と右辺を受け取る2項演算子
// - 数値
static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

static Node *new_node_var(Var *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

static Node *new_node_num(long value, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = value;

    return node;
}

// scopeインスタンスを作成して、リストにつなげる
static VarScope *push_scope(char *name, Var *var) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->var = var;
    sc->depth = scope_depth;
    sc->next = var_scope;
    // var_scope変数はリストの先頭を指している
    var_scope = sc;

    return sc;
}

static TagScope *push_tag_scope(Token *tok, Type *ty) {
    TagScope *tsc = calloc(1, sizeof(TagScope));
    tsc->next = tag_scope;
    tsc->name = strndup(tok->str, tok->len);
    tsc->ty = ty;
    tsc->depth = scope_depth;
    tag_scope = tsc;

    return tsc;
}

// 変数を作成
static Var *new_var(char *name, Type *ty, bool is_local) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    var->is_local = is_local;

    push_scope(name, var);

    return var;
}

// local変数を作成
static Var *new_lvar(char *name, Type *ty) {
    // 変数インスタンスを作成
    // scopeにvarメンバに作成した変数インスタンスを指定して、リストを連結
    Var *var = new_var(name, ty, true);

    // ローカル変数と関数の引数を両方含んだ変数のリストを作成
    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;
    vl->next = locals; // 関数内のローカル変数(または引数)のインスタンス(VarList構造体)を作成して今のlocalsリストにつなげる
    locals = vl; // locals変数が常にVarListの連結リストの先頭を指すようにする

    return var;
}

// global変数を作成
static Var *new_gvar(char *name, Type *ty) {
    Var *var = new_var(name, ty, false); // varはscopeに関連付けられ、リストに連結されていく

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;
    vl->next = globals;
    globals = vl;

    return var;
}

// 今まで見た文字列リテラルがすべて入っているベクタ
static char *new_label(void) {
    static int cnt = 0;
    char buf[20];
    sprintf(buf, ".L.data.%d", cnt++);

    return strndup(buf, 20);
}


// 左結合の演算子をパーズする関数
// 返されるノードの左側の枝のほうが深くなる
static Function *function(void);
static Type *type_suffix(Type *ty);
static Type *basetype(void);
static Type *struct_decl(void);
static Type *union_decl(void);
static Member *struct_member(void);
static void global_var(void);
static VarList *read_func_params(void);
static Type *declarator(Type *ty, char **name);
static Node *declaration(void);
static bool is_typename(void);
static Node *stmt(void);
static Node *stmt2(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *postfix(void);
static Node *primary(void);

// Determine whether the next top-level item is a function
// or a global variable by looking ahead inpit tokens.
// 次のトップレベルのitemについて、トークンの一つ先を読んでfunctionかglobal変数かを決定する
// e.g.
// int *foo : global variable
// int foo[10] : global variable
// int *foo () {} : function
// int foo() {} : function
static bool is_function(void) {
    Token *tok = token;

    Type *ty = basetype();
    char *name = NULL;
    declarator(ty, &name);
    bool isfunc = name && consume("(");

    token = tok; // 読み進めたトークンを元に戻す
    return isfunc;
}

// global変数
// global-var = basetype ident ("[" num "]")* ";"
// TODO: あとで実装したい
// global-var = gvar ( "," gvar )* ";"
// gvar = basetype ident ("[" num "]")* ";"
static void global_var(void) {
    Type *basety = basetype();
    Type *ty;
    char *name = expect_ident();
    ty = type_suffix(basety);

    expect(";");
    new_gvar(name, ty); // varはscopeに関連づけられ、リストに連結されていく
} 

// program = (function | global-var)*
Program *program(void) {
    Function head = {};
    Function *cur = &head;
    globals = NULL; // globals変数を初期化

    while(!at_eof()) {
        // Function
        if ( is_function() ) {
            Function *fn = function();
            if(!fn)
                continue;
            cur->next = fn;
            cur = cur->next;
            continue;
        }

        // Global variable
        global_var();
    }

    Program *prog = calloc(1, sizeof(Program));
    prog->globals = globals;
    prog->fns = head.next;

    return prog;
}

// baseType(Type構造体のbase propertyにあたる)を返す
// basetype = "char" | "short" | "int" | "long" | struct_decl | union-decl
static Type *basetype(void) {

    if (consume("char")) {
        return char_type;
    }
    else if (consume("short")) {
        return short_type;
    }
    else if (consume("int")) {
        return int_type;
    }
    else if (consume("long")) {
        return long_type;
    }
    else if (peek("struct")) {
        return struct_decl();
    }
    else if (peek("union")) {
        return union_decl();
    }

    error_tok(token, "typename expected");
}

// declarator = "*"* ("(" declarator ")" | ident) type-suffix
// e.g.
// nested type declarator
// int (*x)[3];
static Type *declarator(Type *ty, char **name) {
    while(consume("*"))
        // もしderefの記号`*`があったら、kindにTY_PTRを設定したTypeになる
        ty = pointer_to(ty);

    if(consume("(")) {
        Type *placeholder = calloc(1, sizeof(Type));
        Type *new_ty = declarator(placeholder, name);
        expect(")");
        memcpy(placeholder, type_suffix(ty), sizeof(Type));
        return new_ty;
    }

    *name = expect_ident(); // ここでname要素を設定できる

    return type_suffix(ty);
}

// type-suffix = "[" num "]" type-suffix | ε
// 配列ならbaseを設定して、arrayのTypeインスタンスを返す、それ以外ならbaseをそのまま返す
static Type *type_suffix(Type *ty) {
    if(!consume("["))
        return ty;
    int sz = expect_number();
    expect("]");

    ty = type_suffix(ty); // 再起的に関数を呼ぶだけで配列の配列を実装できる!

    return array_of(ty, sz);
}

// struct-decl = "struct" ident
//             | "struct" ident? "{" struct-member "}"
static Type *struct_decl(void) {
    expect("struct");

    // Read a struct tag.
    Token *tag = consume_ident();
    // struct型そのものの定義ではない場合(変数宣言などの場合のsturct宣言)
    // tagが識別子かつ、次のtokenが"{"ではない場合
    if (tag && !peek("{")) {
        TagScope *tsc = find_tag(tag);
        if(!tsc)
            error_tok(tag, "unknown struct type.");

        return tsc->ty;
    }

    // struct型の定義そのものの場合
    expect("{");

    // Read struct members.
    Member head = {};
    Member *cur = &head;

    while(!consume("}")) {
        cur->next = struct_member();
        cur = cur->next;
    }

    // Construct a struct object.
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_STRUCT;
    ty->members = head.next;

    // Assign offsets within the struct to members.
    int offset = 0;
    for (Member *mem = ty->members; mem; mem = mem->next) {
        offset = align_to(offset, mem->ty->align);
        mem->offset = offset;
        offset += mem->ty->size;

        if (ty->align < mem->ty->align)
            ty->align = mem->ty->align;
    }

    ty->size = align_to(offset, ty->align);

    // Register the struct type if a name was given.
    if(tag)
        push_tag_scope(tag, ty);

    return ty;
}

static Type *union_decl(void) {
    expect("union");
    Token *tag = consume_ident();

    if(tag && !peek("{")) {
        TagScope *tsc = find_tag(tag);
        if(!tsc)
            error_tok(tag, "unknown struct type.");
        return tsc->ty;
    }

    expect("{");

    // Read member
    Member head = {};
    Member *cur = &head;

    while(!consume("}")) {
        cur->next = struct_member();
        cur = cur->next;
    }
 
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_STRUCT;
    ty->members = head.next;

    // unionの場合、すでに0で初期化されているので、offsetを割り当てる必要はない
    // しかし、alignmentとsizeは計算する必要がある
    for(Member *mem = ty->members; mem; mem = mem->next) {
        if(ty->align < mem->ty->align)
            ty->align = mem->ty->align;
        if(ty->size < mem->ty->size)
            ty->size = mem->ty->size;
    }

    ty->size = align_to(ty->size, ty->align);

    if(tag)
        push_tag_scope(tag, ty);

    return ty;
}

// struct-member = basetype declarator ";"
// declarator = "*"* ("(" declarator ")" | ident) type-suffix
static Member *struct_member(void) {
    Type *ty = basetype();
    char *name = NULL;
    ty = declarator(ty, &name);
    ty = type_suffix(ty);
    expect(";");

    // memberインスタンスを作成
    Member *mem = calloc(1, sizeof(Member));
    mem->name = name;
    mem->ty = ty;
    return mem;
}

static VarList *read_func_param(void) {
    Type *ty = basetype(); // 型を作成
    char *name = NULL;
    ty = declarator(ty, &name);
    ty = type_suffix(ty);

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = new_lvar(name, ty); // localsリストを更新しつつ、新しいVarインスタンスを返す
    return vl;
}

static VarList *read_func_params(void) {
    if(consume(")"))
        return NULL;

    // ここでは関数の引数のみのリストが作成される
    VarList *head = read_func_param();
    VarList *cur = head;

    while(!consume(")")) {
        expect(",");
        cur->next = read_func_param();
        cur = cur->next;
    }

    return head;
}

// function = basetype declarator "(" params? ")" ("{" stmt* "}" | ";")
// params   = param ("," param)*
// param    = basetype declarator type-suffix
// e.g.
//    int foo (int bar, int foobar) { statement... } <= function definition
//    int foo (int bar, int foobar); <= function declaration
static Function *function() {
    locals = NULL;

    Type *ty = basetype(); // basetypeを作成(関数の返り値の型)
    char *name = NULL;
    ty = declarator(ty, &name);

    // 関数の戻り値の型を、scopeに繋げる
    // new_varの中のpush_scope関数でvar_scopeのリストに繋げる
    // local変数ではないので、第三引数はfalse
    new_var(name, func_type(ty), false);

    // Construct a function body
    Function *fn = calloc(1, sizeof(Function));
    fn->name = name;
    expect("(");

    enter_scope();

    fn->params = read_func_params(); // 関数の引数だけを管理しているVarList

    if(consume(";")) {
        // 関数宣言の場合
        leave_scope();
        return NULL;
    }

    // Read function body
    Node head = {};
    Node *cur = &head;
    expect("{");

    while(!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    leave_scope();

    fn->node = head.next;

    fn->locals = locals; // ローカル変数と引数を合わせて管理している
    return fn;
}

// 文
// declaration = basetype declarator type_suffix ("=" expr)? ";"
// TODO: declaratorに含まれているので、ここのtype_suffixいらない?
//             | basetype ";"
/*
    e.g.
    int a;
    int a = 5;
    int a = 1+3;
    int a = (1 <= 3);
    int a[3];
    int a[3][2];
    struct t{int a; int b;} x; // identつきのstruct宣言
    struct t{int a; int b;};   // identなしのstruct宣言
    int (*x)[3];
*/
static Node *declaration(void) {
    Token *tok = token;
    Type *ty = basetype();
    if (consume(";"))
        return new_node(ND_NULL, tok);

    char *name = NULL;
    ty = declarator(ty, &name);
    // DEBUG
    // ty = type_suffix(ty);
    Var *var = new_lvar(name, ty);

    if(consume(";"))
        return new_node(ND_NULL, tok);
    
    expect("=");
    Node *lhs = new_node_var(var, tok);
    Node *rhs = expr();
    expect(";");

    Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
    return new_unary(ND_EXPR_STMT, node, tok); // 式文の単項になる
}

// 次のtokenがtype名を表現していたら、trueを返す
static bool is_typename(void) {
    return ( peek("char") || peek("short") || peek("int") || peek("long") || peek("struct") || peek("union") );
}

static Node *read_expr_stmt(void) {
    Token *tok = token; // global変数:token(各tokenの連結リスト)のアドレス

    return new_unary(ND_EXPR_STMT, expr(), tok);
}

// statement(文): 値を必ずなにも残さない
static Node *stmt(void) {
    Node *node = stmt2();
    add_type(node);
    return node;
}

// stmt2 = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | declaration
//      | expr ";"
static Node *stmt2(void) {
    Token *tok;

    // return文のnode
    if(tok = consume("return")) {
        Node *node = new_unary(ND_RETURN, expr(), tok);
        expect(";");
        return node;
    }

    // if文のnode
    if(tok = consume("if")) {
        Node *node = new_node(ND_IF, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if(consume("else"))
            node->els = stmt();

        return node;
    }

    // while文のnode
    if(tok = consume("while")) {
        Node *node = new_node(ND_WHILE, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }

    // for文のnode
    if(tok = consume("for")) {
        Node *node = new_node(ND_FOR, tok);
        expect("(");
        if(!consume(";")) {
            node->init = read_expr_stmt(); // 式文
            expect(";");
        }
        if(!consume(";")) {
            node->cond = expr(); // 式
            expect(";");
        }
        if(!consume(")")) {
            node->inc = read_expr_stmt(); // 式文
            expect(")");
        }
        node->then = stmt();
        return node;
    }

    // block {...} (compound-statement)
    if(tok = consume("{")) {
        Node head = {};
        Node *cur = &head;

        enter_scope();

        while(!consume("}")) { // consume("}")の結果がNULLでなければ
            cur->next = stmt();
            cur = cur->next;
        }

        leave_scope();

        Node *node = new_node(ND_BLOCK, tok);
        node->body = head.next;
        return node;
    }

    if(is_typename()) {
        // declaration()内のbasetype関数でtokenのtype名をチェックしたいので、トークンを進めないpeekを使う
        return declaration();
    }

    // expression statement
    Node *node = read_expr_stmt();

    expect(";");
    return node;
}

// expression(式): 値を一つ必ず残す
// expr = assign ("," expr)?
static Node *expr(void) {
    Node *node = assign();
    Token *tok;

    if (tok = consume(",")) {
        node = new_binary(ND_COMMA, node, expr(), tok);
    }

    return node;
}

// assign = equality ("=" assign)?
static Node *assign(void) {
    Node *node = equality();
    Token *tok;

    if(tok = consume("="))
        node = new_binary(ND_ASSIGN, node, assign(), tok);

    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
    Node *node = relational();
    Token *tok;

    for(;;) {
        if(tok = consume("=="))
            node = new_binary(ND_EQ, node, relational(), tok);
        else if(tok = consume("!="))
            node = new_binary(ND_NE, node, relational(), tok);
        else
            return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add |  ">=" add)*
static Node *relational(void) {
    Node *node = add();
    Token *tok;

    for(;;) {
        if(tok = consume("<"))
            node = new_binary(ND_LT, node, add(), tok);
        else if(tok = consume("<="))
            node = new_binary(ND_LE, node, add(), tok);
        else if(tok = consume(">"))
            node = new_binary(ND_LT, add(), node, tok);
        else if(tok = consume(">="))
            node = new_binary(ND_LE, add(), node, tok);
        else
            return node;
    }
}

// In C, `+` operator is overloaded to perform the pointer arithmetric.
// If p is a pointer, p+n adds not n but sizeof(*p)*n to the value of p,
// so that p+n points to the location n elements (not bytes) ahead of p.
// In other words, we need to scale an integer value(n) before adding to a 
// pointer value. This function takes care of the scaling.
static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
    // 数値どうしか、アドレスの入った計算か判断するためにtypeを付与して判別
    add_type(lhs);
    add_type(rhs);

    // num + num
    if(is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_ADD, lhs, rhs, tok);
    // ptr + num
    if(lhs->ty->base && is_integer(rhs->ty))
        return new_binary(ND_PTR_ADD, lhs, rhs, tok);
    if(is_integer(lhs->ty) && rhs->ty->base)
        return new_binary(ND_PTR_ADD, rhs, lhs, tok);

    error_tok(tok, "invalid operands");
}

// `-`演算子を、pointer型の計算の場合はoverloadするように、値をscalingする
static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
    // 数値どうしか、アドレスの入った計算か判断するためにtypeを付与して判別
    add_type(lhs);
    add_type(rhs);

    // num - num
    if(is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_SUB, lhs, rhs, tok);
    // ptr - num
    if(lhs->ty->base && is_integer(rhs->ty))
        return new_binary(ND_PTR_SUB, lhs, rhs, tok);
    // ptr - ptr, which returns how many elements are between the two.
    if(lhs->ty->base && rhs->ty->base)
        return new_binary(ND_PTR_DIFF, lhs, rhs, tok);

    error_tok(tok, "invalid operands");
}

// add = mul ("+" mul | "+" mul)*
static Node *add(void) {
    Node *node = mul();
    Token *tok;

    for(;;) {
        if(tok = consume("+"))
            node = new_add(node, mul(), tok);
        else if(tok = consume("-"))
            node = new_sub(node, mul(), tok);
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(void) {
    Node *node = unary();
    Token *tok;

    for(;;) {
        if(tok = consume("*"))
            node = new_binary(ND_MUL, node, unary(), tok);
        else if(tok = consume("/"))
            node = new_binary(ND_DIV, node, unary(), tok);
        else
            return node;
    }
}

// unary: 単項
// unary = ("+" | "-" | "&" | "*")? unary | postfix
static Node *unary(void) {
    Token *tok;
    if(consume("+")) {
        // +xをxに置き換え
        return unary();
    }
    if (tok = consume("-")) {
        // -xを0-xに置き換え
        return new_binary(ND_SUB, new_node_num(0, tok), unary(), tok);
    }
    if(tok = consume("&"))
        return new_unary(ND_ADDR, unary(), tok);
    if(tok = consume("*"))
        return new_unary(ND_DEREF, unary(), tok);

    return postfix();
}

static Member *get_struct_member(Type *ty, char *name) {
    for(Member *mem = ty->members; mem; mem = mem->next) {
        if(!strcmp(mem->name, name))
            return mem;
    }
    return NULL;
}

static Node *struct_ref(Node *lhs) {
    add_type(lhs);

    if(lhs->ty->kind != TY_STRUCT)
        error_tok(lhs->tok, "not a struct");

    Token *tok = token;
    Member *mem = get_struct_member(lhs->ty, expect_ident());
    if(!mem)
        error_tok(tok, "no such member");

    Node *node = new_unary(ND_MEMBER, lhs, tok);
    node->member = mem;

    return node;
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident)*
// 配列の表現 []演算子
// 構造体の表現 primary . ident: 構造体のメンバへのアクセス演算子( x.y : xは構造体の実体)
// 構造体の表現 primary -> ident: x->y == (*x).y : xはアドレスなのでderefする
static Node *postfix(void) {
    Node *node = primary();
    Token *tok;

    for(;;) {
        if(tok = consume("[")) {
            // x[y] is short for *(x+y)
            Node *exp = new_add(node, expr(), tok); // アドレスの足し算になるので、new_addの方を使う
            expect("]");
            node = new_unary(ND_DEREF, exp, tok);
            continue;
        }

        if(tok = consume(".")) {
            node = struct_ref(node);
            continue;
        }

        if(tok = consume("->")) {
            // x->y is shot for (*x).y
            node = new_unary(ND_DEREF, node, tok);
            node = struct_ref(node);
            continue;
        }

        return node;
    }
}

// stmt-expr = "(" "{" stmt stmt* "}" ")"
// Stament expression is GNU C extension
static Node *stmt_expr(Token *tok) {

    enter_scope();

    // ({})のなかに変数がある可能性がある
    // scopeがどんどんリストの先頭を指すようになる
    Node *node = new_node(ND_STMT_EXPR, tok);
    node->body = stmt();
    Node *cur = node->body;

    while(!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    expect(")");

    leave_scope();

    // TODO: あとで調べる
    // 式なので、値を一つ必ず残す
    // 文は"return" expr ";", "{" stmt* "}"など
    // "(""{" "}"")" の中が stmt stmt* の形になっている必要がある
    // e.g. これらは以下のエラーになった
    // 'int main() { (1;); }'                     => 式の結果はあるので、いまのところエラーにならない?
    // 'int main() { (); }'                       => expected expression 式がない
    // 'int main() { ({}); }'                     => expected expression 式がない
    // 'int main() { return (); }'                => expected expression 式がない
    // 'int main() { return ({}); }'              => expected expression 式がない
    // 'int main() { ({return 1;}); }'            => stmt-expr returning void is not supported
    // 'int main() { ({int x = 5; return x;}); }' => stmt-expr returning void is not supported
    // 'int main() { return ({return 1;}); }'     => stmt-expr returning void is not supported
    if(cur->kind != ND_EXPR_STMT)
        error_tok(cur->tok, "stmt-expr returning void is not supported");

    memcpy(cur, cur->lhs, sizeof(Node)); // 最後のcurはアドレスはそのままで、中身はcur->lhsを指すようになる
    /*
        statementは一つしか許していないけど、複文({}:compound statement)として
        複数のstatementをまとめてひとつのstatementにする
        なので、nodeのアドレスは上記でnodeとして連結されているリストのアドレス(ND_EXPR_STMT)のものを使う
        そして、最後のND_EXPR_STMTだけ式の結果をpushしたい
        でもND_EXPR_STMTのままコード生成すると、statementの中の式の結果をpushした後に
        Expression statementの処理
        add rsp, 8
        が実行されて、pushした値が消えてしまう(statementは値を残さないため?)
        なので、そのnodeのアドレスを式として処理できるように、
        式がセットされているnode->lhsをmemcpyする
        例)
        ({ 1; 2; 3; })
        以下が順にリストで連結される
        1 => node->kind = ND_EXPR_STMT, node->lhs->kind = ND_NUM(val=1)
        2 => node->kind = ND_EXPR_STMT, node->lhs->kind = ND_NUM(val=2)
        3 => node->kind = ND_EXPR_STMT, node->lhs->kind = ND_NUM(val=3)
        最後の3のみ if(node->kind != NODE_EXPR_STMT) のチェック後に
        memcpyされるので、node(statement)のアドレスに、もともとnode->lhs(expression)だったインスタンス(ND_NUM)がセットされている
        つまりnodeのkindがND_EXPR_STMTから、式のND_NUMになっている
        curが単項のnodeで、curがND_EXPR_STMT, cur->lhsがND_NUMの場合
        curのアドレスで、kindがND_NUMになる
    */

    return node;
}

/* 
    "関数呼び出し"は {...} のなかでの関数呼び出しの文の解析
    `foo() { bar(x, y); }`の`bar(x, y)の部分`
*/
// func-args = "(" ( assign ("," assign)* )?  ")"
static Node *func_args(void) {
    if(consume(")"))
        return NULL;

    Node *head = assign();
    Node *cur = head;
    while(consume(",")) {
        cur->next = assign();
        cur = cur->next;
    }
    expect(")");
    return head;
}

// primary = "(" "{" stmt-expr-tail "}" ")"
//           | ("(" expr ")")*
//           | "sizeof" unary
//           | ident func-args?
//           | str
//           | num
static Node *primary(void) {
    Token *tok;

    if (tok = consume("(")) {
        if(consume("{")) {
            // 次のトークンが"("かつ"{"なら stmt_expr "}" ")"となるはず
            Node *node = stmt_expr(tok);
            return node;
        }

        // 次のトークンが"("なら、"(" expr ")"のはず
        Node *node = expr();
        expect(")");
        return node;
    }

    // tokenが"sizeof"の文字列から始まる場合
    if(tok = consume("sizeof")) {
        Node *node = unary();
        add_type(node);
        return new_node_num(node->ty->size, tok);
    }

    // 識別子の場合
    if(tok = consume_ident()) {
        // Function call
        if(consume("(")) {
            Node *node = new_node(ND_FUNCALL, tok);
            node->funcname = strndup(tok->str, tok->len);
            node->args = func_args();
            add_type(node);

            Var *var = find_var(tok);
            if(var) {
                if(var->ty->kind != TY_FUNC)
                    error_tok(tok, "not a function");
                node->ty = var->ty->return_ty;
            } else {
                warn_tok(node->tok, "implicit declaration of a function");
                node->ty = int_type;
            }

            return node;
        }

        // Variable
        Var *var = find_var(tok); //local variableが既存かどうかをリストから調べる
        if(!var) {
            // 関数の引数以外の変数のVarインスタンスは、declaration関数内で作成済みなので、ここで作成しなくて良い
            error_tok(tok, "undefined variable");
        }

        // nodeとvarの紐付け
        return new_node_var(var, tok);
    }

    tok = token; // tokenの位置を戻す

    // トークンの種類が文字列リテラルの場合
    if(tok->kind == TK_STR) {
        token = token->next;

        Type *ty = array_of(char_type, tok->cont_len); // base type はchar型, 長さは文字列の長さ分
        Var *var = new_gvar(new_label(), ty); // nameは型はarray
        // new_gvar()のなかで、varはvar_scopeに関連づけられ、さらにVarList globalsに連結される

        var->contents = tok->contents;
        var->cont_len = tok->cont_len;

        return new_node_var(var, tok);
    }

    // それ以外なら数値のはず
    if(tok->kind != TK_NUM)
        error_tok(tok, "expected expression");

    return new_node_num(expect_number(), tok);
}
