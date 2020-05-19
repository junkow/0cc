#include "9cc.h"

// x86_64のABIで規定されている引数をセットするレジスタのリスト(引数の順番と同じ)
static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int labelseq = 1;
static char *funcname;

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

        printf("#----- \"If\" statement\n");
        if(node->els) {
            gen(node->cond); // expr Aをコンパイルしたコード スタックトップに値が積まれているはず
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je  .L.else.%d\n", seq);
            gen(node->then); // stmt
            printf("    jmp .L.end.%d\n", seq);
            printf(".L.else.%d:\n", seq);
            gen(node->els);  // stmt
            printf(".L.end.%d:\n", seq);
        } else {
            gen(node->cond); // expr Aをコンパイルしたコード スタックトップに値が積まれているはず
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je  .L.end.%d\n", seq);
            gen(node->then); // stmt
            printf(".L.end.%d:\n", seq);
        }
        return;
    }
    case ND_WHILE: {
        int seq = labelseq++;
        /* 
            while(A) B
            A: expression
            B: statement
        */
        printf("#----- \"While\" statement\n");
        printf(".L.begin.%d:\n", seq);
        gen(node->cond);    // Aをコンパイルしたコード
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je  .L.end.%d\n", seq);
        gen(node->then);    // Bをコンパイルしたコード
        printf("    jmp .L.begin.%d\n", seq);
        printf(".L.end.%d:\n", seq);
        return;
    }
    case ND_FOR: {
        int seq = labelseq++;
        /*
            for(A;B;C) D
            A: initialize  expression statement
            B: condition   expression
            C: increment   expression statement
            D:             statement
        */
        printf("#----- \"For\" statement\n");
        if(node->init)
            gen(node->init); // the code which compiled A
        printf(".L.begin.%d:\n", seq);
        if(node->cond) {
            gen(node->cond); // the code which compiled B
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je  .L.end.%d\n", seq);
        }
        gen(node->then); // the code which compiled D
        if(node->inc)
            gen(node->inc);  // the code which compiled C
        printf("    jmp .L.begin.%d\n", seq);
        printf(".L.end.%d:\n", seq);
        return;
    }
    case ND_BLOCK: {
        Node *n = node->body; // statementのリストの先頭
        printf("#----- Block {...}\n");
        for(; n; n = n->next) {
            gen(n);
        }
        // ひとつひとつのstatementは一つの値をスタックに残すので、毎回ポップするのをわすれないこと
        return;
    }
    case ND_FUNCALL: { // 関数呼び出し
        printf("#----- Function call 6つまで引数を取れる \n");
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next) {
            gen(arg);
            nargs++;
        }

        // 引数の数分、値をpopしてレジスタにセットする
        for(int i = nargs-1; i >= 0; i--)
            printf("    pop %s\n", argreg[i]);

        // 関数を呼ぶ前にRSPを調整して、RSPを16byte境界(16の倍数)になるようにアラインメントする
        // - push/popは8byte単位で変更するので、
        //   call命令を発行する時に必ずしもRSPが16の倍数になっているとは限らない
        // - ABIで決められている
        // - RAXは可変関数のために0にセットされている
        /*
            and: (論理積, AND, &) ビットを反転したい箇所に0を置く
            ・rax = 16の場合
            15 : 01111
            16 : 10000
                 00000 => 0

            1 => 1
            2 => 2
            3 => 3
            ...
            14 => 14
            15 => 15
            16 => 0
            17 => 1
            18 => 2
            ...
            31 => 15
            32 => 0
            33 => 1
            ...
        */
        int seq = labelseq++;
        printf("    mov rax, rsp\n");   // rspの値をraxにコピーする
        printf("    and rax, 15\n");    // and: 論理積
        printf("    jnz .L.call.%d\n", seq);  // 結果が0でない(16の倍数でない)=>RSPのアラインメント操作が必要=>.L.call.XXXラベルへジャンプ
        printf("    mov rax, 0\n");     // raxに0をコピー
        printf("    call %s\n", node->funcname);  // 関数呼び出し
        printf("    jmp .L.end.%d\n", seq);  // 関数呼び出しの結果がraxにセットされている=>.L.end.XXXラベルにジャンプ
        printf(".L.call.%d:\n", seq);   // RSPのアラインメント操作をする
        printf("    sub rsp, 8\n");     // スタックをひとつ増やす(push/popで8byteごとに操作しているので、8byte増やすことでRSPを16の倍数に調整)
        printf("    call %s\n", node->funcname);  // 関数呼び出し (RSPがアラインメントできたのでRSPが呼び出せる)
        printf("    mov rax, 0\n");     // raxの値を0にセットする
        printf("    add rsp, 8\n");     // スタックをひとつ減らす(RSP調整のために足したスタックを引いておく)
        printf(".L.end.%d:\n", seq);    // 終了処理
        printf("    push rax\n");       // raxの値(関数呼び出しの結果)をスタックにプッシュ
        return;
    }
    case ND_RETURN:
        gen(node->lhs); // returnの返り値になっている式のコードが出力される

        // 関数呼び出し元に戻る
        printf("#----- Returns to the caller address.\n");
        printf("    pop rax\n"); // スタックトップから値をpopしてraxにセットする
        printf("    jmp .L.return.%s\n", funcname); // .L.returnラベルにジャンプ
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

    for(Function *fn = prog; fn; fn = fn->next) {
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);
        funcname = fn->name;

        // Prologue
        printf("#----- Prologue\n");
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", fn->stack_size);

        // 引数をローカル変数のようにスタックにpushする
        int i = 0;
        for(Var *var = fn->params; var; var = var->next) {
            printf("mov [rbp-%d], %s\n", var->offset, argreg[i++]);
        }

        // printf("    mov [rbp-8],  rdi\n");
        // printf("    mov [rbp-16], rsi\n");

        for (Node *node = fn->node; node; node = node->next) {
            // 抽象構文木を降りながらコード生成
            gen(node);
        }

        // Epilogue
        printf("#----- Epilogue\n");
        printf(".L.return.%s:\n", funcname);     // ラベル(`.L`はファイルスコープ)
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
    }
}
