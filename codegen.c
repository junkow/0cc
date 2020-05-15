#include "9cc.h"

static int labelseq = 1;

// ローカル変数のアドレスの取得
static void gen_addr(Node *node) {
    if(node->kind == ND_VAR) {
        printf("#----- Pushes the given node's memory address to the stack.\n");
        printf("    lea rax, [rbp-%d]\n", node->var->offset); // lea : load effective address
        printf("    push rax\n"); // raxの値(ローカル変数のメモリアドレス)をスタックにpushする
        return;
    }

    error("not an lvalue");
}

static void load(void) {
    printf("#----- Load a value from the memory address.\n");
    printf("    pop rax\n"); // スタックトップからローカル変数のアドレスをpopしてraxに保存する
    printf("    mov rax, [rax]\n"); // raxに入っている値をアドレスとみなして、そのメモリアドレスから値をロードしてraxレジスタにコピーする
    printf("    push rax\n"); // raxの値をスタックにpush
}

static void store(void) {
    printf("#----- Store a value to the memory address.\n");
    printf("    pop rdi\n"); // スタックトップの値(右辺値)をrdiにロードする
    printf("    pop rax\n"); // スタックトップの値(アドレス)をraxにロードする
    printf("    mov [rax], rdi\n"); // raxに入っている値をアドレスとみなし、そのメモリアドレスにrdiに入っている値をストア
    printf("    push rdi\n"); // rdiの値をスタックにpush
}

// 抽象構文木からアセンブリコードを生成する
static void gen(Node *node) {
    switch(node->kind) {
    case ND_NUM:
        printf("    push %ld\n", node->val);
        return;
    case ND_EXPR_STMT:
        // expression (式):  値を一つ必ず残す
        // statement (宣言):  値を必ず何も残さない
        gen(node->lhs);
        printf("#----- Expression statement\n");
        printf("    add rsp, 8\n");
        return;
    case ND_VAR: // 変数の値の参照
        gen_addr(node);

        // メモリアドレスからデータをレジスタにload
        load();
        return;
    case ND_ASSIGN: // ローカル変数(左辺値)への値(右辺値)の割り当て
        gen_addr(node->lhs); // =>最終的に計算結果を入れたraxの値(アドレス)がスタックにpushされる ...push rax
        gen(node->rhs); // =>最終的に計算結果を入れたraxの値(右辺値)がスタックにpushされる ...push rax

        // メモリアドレスへのデータのstore
        store();
        return;
    case ND_IF: {
        int seq = labelseq++;
        printf("#----- If statement\n");
        if(node->els) {
            gen(node->cond); // Aをコンパイルしたコード スタックトップに値が積まれているはず
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L.else.%d\n", seq);
            gen(node->then);
            printf("    jmp .L.end.%d\n", seq);
            printf(".L.else.%d:\n", seq);
            gen(node->els);
            printf(".L.end.%d:\n", seq);
        } else {
            gen(node->cond); // Aをコンパイルしたコード スタックトップに値が積まれているはず
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L.end.%d\n", seq);
            gen(node->then);
            printf(".L.end.%d:\n", seq);
        }
        return;
    }
    case ND_RETURN:
        gen(node->lhs); // returnの返り値になっている式のコードが出力される

        // 関数呼び出し元に戻る
        printf("#----- Returns to the caller address.\n");
        printf("    pop rax\n"); // スタックトップから値をpopしてraxにセットする
        printf("    jmp .L.return\n"); // .L.returnラベルにジャンプ
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

void codegen(Function *prog) {
    // アセンブリの前半部分
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // prologue
    printf("#----- Prologue\n");
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", prog->stack_size); // 予めa-zまでの変数のスペースを確保しておく(8byte * 26文字)

    // 先頭の式から順にコードを生成
    for (Node *node = prog->node; node; node = node->next) {
        // 抽象構文木を降りながらコード生成
        gen(node);
    }

    // epilogue
    printf("#----- Epilogue\n");
    printf(".L.return:\n");     // ラベル(`.L`はファイルスコープ)
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
}
