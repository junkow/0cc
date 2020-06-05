#include "9cc.h"

Type *int_type = &(Type){TY_INT};
// kindにTY_INTを指定したType型インスタンスのアドレス?

bool is_integer(Type *ty) {
    return ty->kind == TY_INT;
}

// pointer型のインスタンスを作成してそれを返す
Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    return ty;
}

void add_type(Node *node) {
    if( !node || node->ty )
        return;
    
    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->init);
    add_type(node->inc);

    for (Node *n = node->body; n; n = n->next)
        add_type(n);
    for (Node *n = node->args; n; n = n->next)
        add_type(n);

    switch(node->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_PTR_DIFF:
    case ND_MUL:
    case ND_DIV:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_FUNCALL:
    case ND_NUM:
        node->ty = int_type;
        return;
    case ND_PTR_ADD:   // ptr + num or num + ptr
    case ND_PTR_SUB:   // ptr - num or num - ptr
    case ND_ASSIGN:    // = : assign
        node->ty = node->lhs->ty;
        return;
    case ND_VAR:
        node->ty = node->var->ty;
        return;
    case ND_ADDR:      // unary & (単項, アドレス)
        node->ty = pointer_to(node->lhs->ty);
        return;
    case ND_DEREF:     // unary * (単項, 間接参照)
        if(node->lhs->ty->kind != TY_PTR) // 左辺値のtypeの種類がpointerでない場合はエラー
            error_tok(node->tok, "invalid pointer dereference");
        node->ty = node->lhs->ty->base;
        return;
    }
}