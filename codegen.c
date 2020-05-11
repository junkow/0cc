#include "9cc.h"

// ローカル変数のアドレスの取得
static void gen_addr(Node *node) {
    if(node->kind == ND_VAR) {
        int offset = (node->name - 'a' + 1) * 8;
        printf("    lea rax, [rbp-%d]\n", offset); // lea : load effective address
        printf("    push rax\n"); // raxの値(ローカル変数のメモリアドレス)をスタックにpushする
        return;
    }

    error("代入の左辺値が変数ではありません");
}

static void load(void) {
    printf("    pop rax\n"); // スタックからローカル変数のアドレスをpopしてraxに保存する
    printf("    mov rax, [rax]\n"); // raxに入っている値をアドレスとみなして、そのメモリアドレスから値をロードしてraxレジスタにコピーする
    printf("    push rax\n"); // raxの値をスタックにpush
}

static void store(void) {
    printf("    pop rdi\n"); // スタックの値をrdiにロードする
    printf("    pop rax\n"); // スタックの値をraxにロードする
    printf("    mov [rax], rdi\n"); // raxに入っている値をアドレスとみなし、そのメモリアドレスにrdiに入っている値をストア
    printf("    push rdi\n"); // rdiの値をスタックにpush
}

// 抽象構文木からアセンブリコードを生成する
static void gen(Node *node) {
    switch(node->kind) {
    case ND_NUM:
        printf("    push %ld\n", node->val);
        return;
    case ND_VAR: // 変数の値の参照
        gen_addr(node);

        // メモリアドレスからデータをレジスタにload
        load();
        return;
    case ND_ASSIGN: // ローカル変数(左辺値)への値(右辺値)の割り当て
        gen_addr(node->lhs); // =>最終的に計算結果を入れたraxの値がスタックにpushされる ...push rax
        gen(node->rhs); // =>最終的に計算結果を入れたraxの値がスタックにpushされる ...push rax

        // メモリアドレスへのデータのstore
        store();
        return;
    case ND_EXPR_STMT:
        gen(node->lhs);
        printf("    add rsp, 8\n");
        return;
    case ND_RETURN:
        gen(node->lhs); // returnの返り値になっている式のコードが出力される

        // 関数呼び出し元に戻る
        printf("    pop rax\n"); // スタックから値をpopしてraxにセットする
        printf("    jmp .L.return\n"); // リターンアドレスをpopして、そのアドレスにジャンプする
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

    // prologue
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", 208); // 予めa-zまでの変数のスペースを確保しておく(8byte * 26文字)

    // 先頭の式から順にコードを生成
    for (Node *n = node; n; n = n->next) {
        // 抽象構文木を降りながらコード生成
        gen(n);
    }

    // epilogue
    // 最後の式の結果がRAXに残っているので、それをRBPにセットして関数からの返り値とする
    printf(".L.return:\n");
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
}
