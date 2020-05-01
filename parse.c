#include "9cc.h"

Node *code[100];

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

static Node *new_node_var(char name) {
    Node *node = new_node(ND_LVAR);
    node->name = name;
    return node;
}

static Node *new_node_num(int value) {
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
Node *program(void) {
    int i = 0;
    while(!at_eof())
        code[i++] = stmt();

    code[i] = NULL; // 最後のノードはNULLで埋めておくと、どこが末尾かわかるようになる
}

// stmt = expr ";"
static Node *stmt(void) {
    Node *node = expr();
    expect(";");
    return node;
}

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
    Token *tok = consume_ident();
    if(tok) {
        Node *node = new_node_var(*tok->str);
        // ASCIIコードでは0~127までの数に対して文字が割り当てられている
        // 0-9、a-zは文字コード上で連続している
        // Cでは文字は整数型の単なる小さな値。'a'は97、'0'は48と等価
        // str[0] - 'a'でaから何文字離れているのかがわかる
        node->offset = (tok->str[0] - 'a' + 1) * 8;
        return node;
    }

    if(consume("(")) { // 次のトークンが"("なら、"(" expr ")"のはず
        Node *node = expr();
        expect(")");
        return node;
    }

    // それ以外なら数値
    return new_node_num(expect_number());
}