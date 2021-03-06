#include "9cc.h"

// 複合リテラル
Type *void_type = &(Type){TY_VOID, 1, 1};
Type *char_type = &(Type){TY_CHAR, 1, 1};
Type *short_type = &(Type){TY_SHORT, 2, 2};
Type *int_type = &(Type){TY_INT, 4, 4};
Type *long_type = &(Type){TY_LONG, 8, 8};

static Type *new_type(TypeKind kind, int size, int align) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = kind;
    ty->size = size;
    ty->align = align;
    return ty;
}

bool is_integer(Type *ty) {
    TypeKind k = ty->kind;
    return k == TY_CHAR || k == TY_SHORT || k == TY_INT || k == TY_LONG;
}

int align_to(int n, int align) {
    // `n`の値を最も近い`align`の倍数に丸める

    // ~(align-1): alignの値の符号反転
    // AND演算子: ビットを反転したいところは0にしておく
    //           変えたくないところは1にしておく
    // ・ align_to(5, 8)
    //     00001110: n + align + 1
    // AND 11111000: ~(align-1)
    //     00001000: returns 8
    //
    // ・ align_to(11, 8)
    //     00010100: n + align + 1
    // AND 11111000: ~(align-1)
    //     00010000: => returns 16
    // return (n + align + 1) & ~(align-1);

    // ・ align_to(5, 8)
    // 00001110: n + align + 1
    // 00000001: Shift right by 3 bits (8 = 2^3)
    // 00001000: Shift left by 3 bits
    // 00001000: => returns 8
    //
    // ・ align_to(11, 8)
    // 00010100: n + aling + 1
    // 00000010: Shift right by 3 bits
    // 00010000: Shift left by 3 bits
    // 00010000: => returns 16
    return (n + align - 1) / align * align;
}

// pointerのTypeのインスタンスを作成してそれを返す
// もとのtypeのbaseから、baseプロパティを指定する
Type *pointer_to(Type *base) {
    Type *ty = new_type(TY_PTR, 8, 8);
    ty->base = base;
    return ty;
}

// arrayのTypeのインスタンスを作成してそれを返す
// もとのtypeのbaseから、sizeとbaseを指定する
Type *array_of(Type *base, int len) {
    Type *ty = new_type(TY_ARRAY, base->size * len, base->align);
    ty->base = base;
    ty->array_len = len;
    return ty;
}

Type *func_type(Type *return_ty) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_FUNC;
    ty->return_ty = return_ty;
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
    case ND_NUM:
    case ND_FUNCALL:
        node->ty = long_type;
        return;
    case ND_PTR_ADD:   // ptr + num or num + ptr
    case ND_PTR_SUB:   // ptr - num or num - ptr
    case ND_ASSIGN:    // = : assign
        node->ty = node->lhs->ty; // 左辺値の型がnodeの型になる
        return;
    case ND_VAR:
        node->ty = node->var->ty;
        return;
    case ND_COMMA:
        node->ty = node->rhs->ty; // nodeの型は最後の式の型に指定
        return;
    case ND_MEMBER:
        node->ty = node->member->ty;
        return;
    case ND_ADDR:      // unary & (単項, アドレス)
        // array型の場合その中身の要素の型が知りたい。node->lhs->tyがarray型なので、配列の中身の要素についてはnode->lhs->ty->baseにある
        if(node->lhs->ty->kind == TY_ARRAY)
            node->ty = pointer_to(node->lhs->ty->base);
        else
            node->ty = pointer_to(node->lhs->ty);  // array以外ならnode->lhs->tyがそのbaseにあたる
        return;
    case ND_DEREF:     // unary * (単項, 逆参照)
        if(!node->lhs->ty->base) // 左辺値の型にbaseの型が指定されていなければエラー(たとえばint型にbaseは指定されていない)
            error_tok(node->tok, "invalid pointer dereference");
        if(node->lhs->ty->base->kind == TY_VOID)
            error_tok(node->tok, "dereferencing a void pointer");

        node->ty = node->lhs->ty->base;
        return;
    case ND_STMT_EXPR: {
        Node *stmt = node->body;
        while (stmt->next)
            stmt = stmt->next;
        node->ty = stmt->ty; // nodeの型は最後の式の型に指定
        return;
    }
    }
}
