#include "9cc.h"

// 左辺値(ローカル変数)の生成
void gen_lval(Node *node) {
    if(node->kind != ND_LVAR)
        error("代入の左辺値が変数ではありません");

    printf("    mov rax, rbp\n"); // rbpの値をraxにコピーする
    printf("    sub rax, %d\n", node->offset); // raxからオフセット分マイナスして、メモリスタックを増やす(スタックは上から下に成長する)
    printf("    push rax\n"); // raxの値をスタックにpushする
}

// 抽象構文木からアセンブリコードを生成する
void gen(Node *node) {
    switch(node->kind) {
    case ND_NUM:
        printf("    push %d\n", node->val);
        return;
    case ND_LVAR:
        gen_lval(node);
        printf("    pop rax\n"); // raxにスタックの値をロードする
        printf("    mov rax, [rax]\n"); // raxに入っている値をアドレスとみなして、そのメモリアドレスから値をロードしてraxレジスタにコピーする
        printf("    push rax\n"); // raxの値をスタックにpush
        return;
    case ND_ASSIGN: // ローカル変数(左辺値)への値(右辺値)の割り当て
        gen_lval(node); // =>最終的に計算結果を入れたraxの値がスタックにpushされる ...push rax
        gen(node->rhs); // =>最終的に計算結果を入れたraxの値がスタックにpushされる ...push rax

        printf("    pop rdi\n"); // スタックの値をrdiにロードする
        printf("    pop rax\n"); // スタックの値をraxにロードする
        printf("    mov [rax], rdi\n"); // raxに入っている値をアドレスとみなし、そのメモリアドレスにrdiに入っている値をストア
        printf("    push rdi\n"); // rdiの値をスタックにpush
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
    case ND_ASSIGN:
        printf("    mov [rdi], rax");
        break;
    case ND_LVAR:
        break;
    }

    printf("    push rax\n");
}
