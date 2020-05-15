#include "9cc.h"

Var *locals; // 連結リストを管理する

// 連結リストから変数を名前で検索。見つからなかった場合はNULLを返す
static Var *find_var(Token *tok) {
    for(Var *var = locals; var; var=var->next) {
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
    // 新しくローカル変数(Var構造体)を作成してlocalsリストにつなげる
    var->next = locals;
    locals = var; // locals変数が常に連結リストの先頭を指すようにする

    return var;
}

// 新しいノードを作成する関数
// 以下の2種類に合わせて関数を二つ用意する
// - 左辺と右辺を受け取る2項演算子
// - 数値
static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node *lhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    return node;
}

static Node *new_node_var(Var *var) {
    Node *node = new_node(ND_VAR);
    node->var = var;
    return node;
}

static Node *new_node_num(long value) {
    Node *node = new_node(ND_NUM);
    node->val = value;
    return node;
}

// 左結合の演算子をパーズする関数
// 返されるノードの左側の枝のほうが深くなる
static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// program = stmt*
Function *program(void) {
    locals = NULL;

    Node head = {};
    Node *cur = &head;

    while(!at_eof()) {
        cur->next = stmt();
        cur = cur->next;
    }

    Function *prog = calloc(1, sizeof(Function));
    prog->node = head.next;
    prog->locals = locals;

    return prog;
}

// statement(宣言): 値を必ずなにも残さない
// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | expr ";"
static Node *stmt(void) {
    // return文のnode
    if(consume("return")) {
        Node *node = new_unary(ND_RETURN, expr());
        expect(";");
        return node;
    }

    // if文のnode
    if(consume("if")) {
        Node *node = new_node(ND_IF);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if(consume("else"))
            node->els = stmt();

        return node;
    }

    // while文のnode
    if(consume("while")) {
        Node *node = new_node(ND_WHILE);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }

     // expression statement
    Node *node = new_unary(ND_EXPR_STMT, expr());
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

    if(consume("="))
        node = new_binary(ND_ASSIGN, node, assign());

    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality() {
    Node *node = relational();

    for(;;) {
        if(consume("=="))
            node = new_binary(ND_EQ, node, relational());
        else if(consume("!="))
            node = new_binary(ND_NE, node, relational());
        else
            return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add |  ">=" add)*
static Node *relational() {
    Node *node = add();

    for(;;) {
        if(consume("<"))
            node = new_binary(ND_LT, node, add());
        else if(consume("<="))
            node = new_binary(ND_LE, node, add());
        else if(consume(">"))
            node = new_binary(ND_LT, add(), node);
        else if(consume(">="))
            node = new_binary(ND_LE, add(), node);
        else
            return node;
    }
}

// add = mul ("+" mul | "+" mul)*
static Node *add() {
    Node *node = mul();

    for(;;) {
        if(consume("+"))
            node = new_binary(ND_ADD, node, mul());
        else if(consume("-"))
            node = new_binary(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul() {
    Node *node = unary();

    for(;;) {
        if(consume("*"))
            node = new_binary(ND_MUL, node, unary());
        else if(consume("/"))
            node = new_binary(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? unary | primary
static Node *unary() {
    if(consume("+"))
        // +xをxに置き換え
        return unary();
    if (consume("-"))
        // -xを0-xに置き換え
        return new_binary(ND_SUB, new_node_num(0), unary());
    return primary();
}

// primary = num | ident | ("(" expr ")")*
static Node *primary() {
    if(consume("(")) { // 次のトークンが"("なら、"(" expr ")"のはず
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if(tok) {
        // Node *node = new_node(ND_VAR);
        Var *var = find_var(tok); //local variableが既存かどうかをリストから調べる
        if(!var) {
            // var == NULLなので、そのまま代入できる
            var = new_lvar(strndup(tok->str, tok->len));
        }

         // nodeとvarの紐付け
        return new_node_var(var);
    }

    // それ以外なら数値
    return new_node_num(expect_number());
}