#include "9cc.h"

// 抽象構文木からアセンブリコードを生成する
static void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch(node->kind) {
    case ND_ADD:
        printf("    add rax, rdi\n");
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        // raxに入っている64ビットの値を128ビットに伸ばしてRDXとRAXにセットする
        printf("    cqo\n");
        // idiv: 符号あり除算命令
        printf("    idiv rax, rdi\n");
        break;
    case ND_EQ:
        // popしてきた値をcompareする
        printf("    cmp rax, rdi\n");
        // alはraxの下位8ビット
        printf("    sete al\n");
        // movzb: rax全体を0か1にするために上位56ビットをゼロクリアする
        printf("    movzb rax, al\n");
        break;
    case ND_NE:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LT:
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LE:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    }

    printf("    push rax\n");
}

void codegen(Node *node) {
	// アセンブリの前半部分
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 抽象構文木を降りながらコード生成
    gen(node);

    // スタックトップに式全体の値が残っているはずなので、
    // それをRAXにロードして関数からの返り値とする
    printf("    pop rax\n");
    printf("    ret\n");
}