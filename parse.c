#include "9cc.h"

// ローカル変数と引数を管理するリスト
// ローカル変数と引数を区別せずにひとつの連結リストで管理できる
static VarList *locals;

// 連結リストから変数を名前で検索。見つからなかった場合はNULLを返す
static Var *find_var(Token *tok) {
    for(VarList *vl = locals; vl; vl = vl->next) {
        Var *var = vl->var;
        if(strlen(var->name) == tok->len && !strncmp(tok->str, var->name, tok->len)) {
            // 変数名がリストから見つかったら、その位置のvar構造体のポインタを返す
            return var;
        }
    }
    return NULL;
}

static Var *new_lvar(char *name) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;

    // ローカル変数と関数の引数を両方含んだ変数のリストを作成
    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;
    vl->next = locals; // 関数内のローカル変数(または引数)のインスタンス(VarList構造体)を作成して今のlocalsリストにつなげる
    locals = vl; // locals変数が常にVarListの連結リストの先頭を指すようにする

    return var;
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

// 左結合の演算子をパーズする関数
// 返されるノードの左側の枝のほうが深くなる
static VarList *read_func_params(void);
static Function *function(void);
static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// program = function*
Function *program(void) {
    Function head = {};
    Function *cur = &head;

    while(!at_eof()) {
        cur->next = function();
        cur = cur->next;
    }

    return head.next;
}

static VarList *read_func_params(void) {
    if(consume(")"))
        return NULL;

    // ここでは関数の引数のみのリストが作成される
    VarList *head = calloc(1, sizeof(VarList));
    head->var = new_lvar(expect_ident()); // localsリストを更新しつつ、新しいVarインスタンスを返す
    VarList *cur = head;

    while(!consume(")")) {
        expect(",");
        cur->next = calloc(1, sizeof(VarList));
        cur->next->var = new_lvar(expect_ident());
        cur = cur->next;
    }

    return head;
}

/* "関数定義"は {...} の外側 `foo() {...}` のfoo()の部分の解析 */
// function = ident "(" params? ")" "{" stmt* "}"
// params   = ident ("," ident)*
static Function *function() {
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
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

static Node *read_expr_stmt(void) {
    Token *tok = token; // global変数:token(各tokenの連結リスト)のアドレス
    return new_unary(ND_EXPR_STMT, expr(), tok);
}

// statement(宣言): 値を必ずなにも残さない
// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | expr ";"
static Node *stmt(void) {
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
            // TODO: あとで考える
            node->init = read_expr_stmt(); // 式宣言
            expect(";");
        }
        if(!consume(";")) {
            node->cond = expr(); // 式
            expect(";");
        }
        if(!consume(")")) {
            // TODO: あとで考える
            node->inc = read_expr_stmt(); // 式宣言
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

// add = mul ("+" mul | "+" mul)*
static Node *add(void) {
    Node *node = mul();
    Token *tok;

    for(;;) {
        if(tok = consume("+"))
            node = new_binary(ND_ADD, node, mul(), tok);
        else if(tok = consume("-"))
            node = new_binary(ND_SUB, node, mul(), tok);
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
// unary = ("+" | "-")? unary | primary
static Node *unary(void) {
    Token *tok;
    if(consume("+"))
        // +xをxに置き換え
        return unary();
    if (tok = consume("-"))
        // -xを0-xに置き換え
        return new_binary(ND_SUB, new_node_num(0, tok), unary(), tok);
    return primary();
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

// primary = ("(" expr ")")* | ident func-args | num 
static Node *primary(void) {
    if(consume("(")) { // 次のトークンが"("なら、"(" expr ")"のはず
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok;
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
            // var == NULLなので、そのまま代入できる
            var = new_lvar(strndup(tok->str, tok->len));
        }

        // nodeとvarの紐付け
        return new_node_var(var, tok);
    }

    // それ以外なら数値のはず
    tok = token;
    if(tok->kind != TK_NUM)
        error_tok(tok, "expected expression");

    return new_node_num(expect_number(), tok);
}