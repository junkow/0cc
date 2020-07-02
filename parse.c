#include "9cc.h"

// 変数をまとめるリスト
// ローカル変数と引数をまとめるリスト
// パーズ中に作成されたすべてのlocal variableインスタンスをまとめる
static VarList *locals;
// global変数をまとめるリスト
static VarList *globals;

// 変数を見つける
// 連結リストから変数を名前で検索。見つからなかった場合はNULLを返す
// local変数=>global変数の順で検索することで、local変数が同名のglobal変数を隠すという動きを実装できる
static Var *find_var(Token *tok) {
    // local変数から名前でlookup
    for(VarList *vl = locals; vl; vl = vl->next) {
        Var *var = vl->var;
        if(strlen(var->name) == tok->len && !strncmp(tok->str, var->name, tok->len)) {
            // 変数名がリストから見つかったら、その位置のvar構造体のポインタを返す
            return var;
        }
    }

    // local変数のリストになければglobal変数のリストから名前でlookup
    for(VarList *vl = globals; vl; vl = vl->next) {
        Var *var = vl->var;
        if(strlen(var->name) == tok->len && !strncmp(tok->str, var->name, tok->len)) {
            // 変数名がリストから見つかったら、その位置のvar構造体のポインタを返す
            return var;
        }
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

static Node *new_unary(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
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

// 変数を作成
static Var *new_var(char *name, Type *ty, bool is_local) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    var->is_local = is_local;

    return var;
}

// local変数を作成
static Var *new_lvar(char *name, Type *ty) {
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
    Var *var = new_var(name, ty, false);

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
static Type *read_type_suffix(Type *base);
static Type *basetype(void);
static void global_var(void);
static VarList *read_func_params(void);

static Node *declaration(void);
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
// トップレベルのitemについて、トークンの一つ先を読んでfunctionかglobal変数かを決定する
// e.g.
// int *foo : global variable
// int foo[10] : global variable
// int *foo () {} : function
// int foo() {} : function
static bool is_function(void) {
    Token *tok = token;
    basetype();
    
    bool isfunc = consume_ident() && consume("(");
    token = tok; // 読み進めたトークンを元に戻す
    return isfunc;
}

// program = function*
Program *program(void) {
    Function head = {};
    Function *cur = &head;
    globals = NULL; // globals変数を初期化

    while(!at_eof()) {
        if ( is_function() ) {
            cur->next = function();
            cur = cur->next;
        } else {
            // global変数を作成
            global_var();
        }
    }

    Program *prog = calloc(1, sizeof(Program));
    prog->globals = globals;
    prog->fns =  head.next;

    return prog;
}

// global変数
// global-var = basetype ident ("[" num "]")* ";"
static void global_var(void) {
    Type *ty =  basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);
    expect(";");
    new_gvar(name, ty);
}

// baseType(Type構造体のbase propertyにあたる)を返す
// basetype = "int" "*"*
// "int"と"*"が0以上の組み合わせ
static Type *basetype(void) {
    Type *ty;

    if (consume("char")) {
        ty = char_type;
    } else {
        expect("int");
        ty = int_type;
    }

    while(consume("*"))
        // もしderefの記号`*`があったら、kindにTY_PTRを設定したTypeになる
        ty = pointer_to(ty);

    return ty;
}

// 配列ならbaseを設定して、arrayのTypeインスタンスを返す、それ以外ならbaseをそのまま返す
static Type *read_type_suffix(Type *base) {
    if(!consume("["))
        return base;
    int sz = expect_number();
    expect("]");
    
    base = read_type_suffix(base); // 再起的に関数を呼ぶだけで配列の配列を実装できる!

    return array_of(base, sz);
}

static VarList *read_func_param(void) {
    Type *ty = basetype(); // 型を作成
    char *name = expect_ident();
    ty = read_type_suffix(ty);

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

/* "関数定義"は {...} の外側 `foo() {...}` のfoo()の部分の解析 */
// function = basetype ident "(" params? ")" "{" stmt* "}"
// params   = param ("," param)*
// param    = basetype ident
// e.g. int foo (int bar, int foobar)
static Function *function() {
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    basetype(); // basetypeを作成(関数の返り値の型) tokenがintでない場合のエラーチェック?
    fn->name = expect_ident();

    expect("(");
    fn->params = read_func_params(); // 関数の引数だけを管理しているVarList
    expect("{");

    Node head = {};
    Node *cur = &head;

    while(!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    fn->node = head.next;
    fn->locals = locals; // ローカル変数と引数を合わせて管理している
    return fn;
}

// 文
// declaration = basetype ident ("[" num "]")* ("=" expr)? ";"
/*
    e.g.
    int a;
    int a = 5;
    int a = 1+3;
    int a = (1 <= 3);
    int a[3];
    int a[3][2];
*/
static Node *declaration(void) {
    Token *tok = token;
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);
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
    return ( peek("char") || peek("int") );
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

        while(!consume("}")) { // consume("}")の結果がNULLでなければ
            cur->next = stmt();
            cur = cur->next;
        }

        Node *node = new_node(ND_BLOCK, tok);
        node->body = head.next;
        return node;
    }

    if(is_typename()) {
        // declaration()内のbasetype関数でtokenの"int"をチェックしたいので、トークンを進めないpeekを使う
        return declaration();
    }

    // expression statement
    Node *node = read_expr_stmt();
    expect(";");
    return node;
}

// expression(式): 値を一つ必ず残す
// expr = assign
static Node *expr(void) {
    return assign();
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

static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
    // 数値どうしか、アドレスの入った計算か判断するためにtypeを付与して判別
    add_type(lhs);
    add_type(rhs);

    if(is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_ADD, lhs, rhs, tok);
    if(lhs->ty->base && is_integer(rhs->ty))
        return new_binary(ND_PTR_ADD, lhs, rhs, tok);
    if(is_integer(lhs->ty) && rhs->ty->base)
        return new_binary(ND_PTR_ADD, rhs, lhs, tok);

    error_tok(tok, "invalid operands");
}

static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
    // 数値どうしか、アドレスの入った計算か判断するためにtypeを付与して判別
    add_type(lhs);
    add_type(rhs);

    if(is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_SUB, lhs, rhs, tok);
    if(lhs->ty->base && is_integer(rhs->ty))
        return new_binary(ND_PTR_SUB, lhs, rhs, tok);
    if(is_integer(lhs->ty) && rhs->ty->base)
        return new_binary(ND_PTR_DIFF, lhs, lhs, tok);

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

// 配列の表現, []演算子を追加
// postfix = primary ("[" expr "]")*
static Node *postfix(void) {
    Node *node = primary();
    Token *tok;

    while(tok = consume("[")) {
        // x[y] is short for *(x+y)
        Node *exp = new_add(node, expr(), tok); // アドレスの足し算になるので、new_addの方を使う
        expect("]");
        node = new_unary(ND_DEREF, exp, tok);
    }

    return node;
}

// stmt-expr = "(" "{" stmt stmt* "}" ")"
// Stament expression is GNU C extension
static Node *stmt_expr(Token *tok) {
    Node *node = new_node(ND_STMT_EXPR, tok);
    node->body = stmt();
    Node *cur = node->body;

    while(!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    expect(")");

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

    memcpy(cur, cur->lhs, sizeof(Node));
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

    if (consume("(")) {
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

    tok = token;

    // トークンの種類が文字列リテラルの場合
    if(tok->kind == TK_STR) {
        token = token->next;

        Type *ty = array_of(char_type, tok->cont_len); // base type はchar型, 長さは文字列の長さ分
        Var *var = new_gvar(new_label(), ty); // nameは型はarray

        var->contents = tok->contents;
        var->cont_len = tok->cont_len;

        return new_node_var(var, tok);
    }

    // それ以外なら数値のはず
    if(tok->kind != TK_NUM)
        error_tok(tok, "expected expression");

    return new_node_num(expect_number(), tok);
}
