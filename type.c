#include "9cc.h"

// 複合リテラル
Type *char_type = &(Type){TY_CHAR, 1};
Type *int_type = &(Type){TY_INT, 8};

bool is_integer(Type *ty) {
    return ty->kind == TY_CHAR || ty->kind == TY_INT;
}

// pointerのTypeのインスタンスを作成してそれを返す
// もとのtypeのbaseから、baseプロパティを指定する
Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    ty->size = 8;
    return ty;
}

// arrayのTypeのインスタンスを作成してそれを返す
// もとのtypeのbaseから、sizeとbaseを指定する
Type *array_of(Type *base, int len) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_ARRAY;
    ty->size = base->size * len;
    ty->base = base;
    ty->array_len = len;
    return ty;
}

// nodeに型を付与する
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
        node->ty = node->lhs->ty; // 左辺値の型がnodeの型になる
        return;
    case ND_VAR:
        node->ty = node->var->ty;
        return;
    case ND_ADDR:      // unary & (単項, アドレス)
        // array型の場合その中身の要素の型が知りたい。node->lhs->tyがarray型なので、配列の中身の要素についてはnode->lhs->ty->baseにある
        if(node->lhs->ty->kind == TY_ARRAY)
            node->ty = pointer_to(node->lhs->ty->base);
        else
            node->ty = pointer_to(node->lhs->ty);  // array以外ならnode->lhs->tyがそのbaseにあたる
        return;
    case ND_DEREF:     // unary * (単項, 逆参照)
        // if (node->lhs->ty->kind != TY_PTR) // 左辺値の型のkindがTY_PTRでない場合はエラー
        if(!node->lhs->ty->base) // 左辺値の型にbaseの型が指定されていなければエラー たとえばint型にbaseは指定されていない
            error_tok(node->tok, "invalid pointer dereference");
        node->ty = node->lhs->ty->base;
        return;
    case ND_STMT_EXPR: {
        Node *last = node->body;
        while (last->next)
            last = last->next;
        node->ty = last->ty;
        return;
    }
    }
}
