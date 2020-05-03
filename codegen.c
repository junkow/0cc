#include "9cc.h"

// 与えられたアドレスをスタックにpushする
// static void get_addr(Node *node) {
//     if(node->kind == ND_LVAR) {
//         // ローカル変数ならば、オフセット値を取り出してそこから計算したアドレスをstackにpush
//         printf("    lea rax, [rbp-%d]\n", node->var->offset); // rax <= RBP(関数フレームの基準位置)-offset : stackは下に成長するので値を増やす場合はマイナス
//         printf("    push rax\n"); // ローカル変数の指すアドレス(raxに保存されている)をスタックにpush
//         return;
//     }
// }

//     // ローカル変数ではない場合
//     error("not an lvalue."); 
// }

// static void load(void) {
//     pritnf("    pop rax\n"); // スタックからpopして、その値をraxに保存
//     printf("    mov rax, [rax]\n"); // raxに保存してある値をメモリアドレスとみなして、そのメモリアドレスから値をロードしてraxにコピーする
//     printf("    push rax\n"); // raxに保存した値をスタックにpush
// }

// static void store(void) {
//     printf("    pop rdi\n"); // スタックからpopして、その値をrdiに保存
//     printf("    pop rax\n"); // スタックからpopして、その値をraxに保存
//     printf("    mov [rax], rdi\n"); // raxに保存してある値をメモリアドレスとみなして、そのメモリアドレスにrdiに保存してある値をコピーする
//     printf("    push rdi\n"); // rdiの値をスタックにpushする
// }

// 左辺値(ローカル変数)の生成
void gen_lval(Node *node) {
    if(node->kind != ND_LVAR)
        error("代入の左辺値が変数ではありません");

    printf("    mov rax, rbp\n"); // rbpの値をraxにコピーする
    printf("    sub rax, %d\n", node->var->offset); // rax-offset: ローカル変数のメモリアドレスを計算して、raxに保存する
    printf("    push rax\n"); // raxの値(ローカル変数のメモリアドレス)をスタックにpushする
}

// 抽象構文木からアセンブリコードを生成する
void gen(Node *node) {
    switch(node->kind) {
    case ND_NUM:
        printf("    push %d\n", node->val);
        return;
    case ND_LVAR: // ローカル変数の値の参照
        gen_lval(node);

        // メモリアドレスからのデータのload
        // 上の関数で計算したローカル変数のメモリアドレスをraxから取り出して、そのメモリアドレスから値をレジスタにロードする
        printf("    pop rax\n"); // スタックからローカル変数のアドレスをpopしてraxに保存する
        printf("    mov rax, [rax]\n"); // raxに入っている値をアドレスとみなして、そのメモリアドレスから値をロードしてraxレジスタにコピーする
        printf("    push rax\n"); // raxの値をスタックにpush
        return;
    case ND_ASSIGN: // ローカル変数(左辺値)への値(右辺値)の割り当て
        // 左側が優先の計算順序なのでノードの左辺から先に生成する
        gen_lval(node->lhs); // =>最終的に計算結果を入れたraxの値がスタックにpushされる ...push rax
        gen(node->rhs); // =>最終的に計算結果を入れたraxの値がスタックにpushされる ...push rax

        // メモリアドレスへのデータのstore
        printf("    pop rdi\n"); // スタックの値をrdiにロードする
        printf("    pop rax\n"); // スタックの値をraxにロードする
        printf("    mov [rax], rdi\n"); // raxに入っている値をアドレスとみなし、そのメモリアドレスにrdiに入っている値をストア
        printf("    push rdi\n"); // rdiの値をスタックにpush
        return;
    }

    // 左側が優先の計算順序なのでノードの左辺から先に生成する
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
