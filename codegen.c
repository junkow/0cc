#include "9cc.h"

// x86_64のABIで規定されている引数をセットするレジスタのリスト(引数の順番と同じ)
static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int labelseq = 1;
static char *funcname;

static void gen(Node *node);

// ローカル変数のアドレスの取得
static void gen_addr(Node *node) {
    switch(node->kind) {
    case ND_VAR: {
        Var *var = node->var;
        if (var->is_local) {
            printf("#----- Local variable");
            printf("#----- Pushes the given node's memory address to the stack.\n");
            // srcオペランドのメモリアドレスを計算し、distオペランドにロードする
            printf("    lea rax, [rbp-%d]\n", var->offset); // lea : load effective address
            printf("    push rax\n"); // raxの値(ローカル変数のメモリアドレス)をスタックにpushする
        } else {
            printf("#----- Global variable");
            printf("    push offset %s\n", var->name);
        }
        return;
    }
    case ND_DEREF: // 左辺値に逆参照がきた場合に処理できるようにする
        gen(node->lhs); // 左辺を展開する
        return;
    }

    error_tok(node->tok, "not an lvalue");
}

static void gen_lval(Node *node) {
    if(node->ty->kind == TY_ARRAY) // arrayの形のままの場合は左辺値ではない(アドレスが取れない)のでエラー
        error_tok(node->tok, "not an lvalue");
    gen_addr(node);
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
    case ND_NULL: // Empty statement
        return;
    case ND_NUM:
        printf("    push %ld\n", node->val);
        return;
    case ND_EXPR_STMT:
        // expression (式):  値を一つ必ず残す
        // statement (文):  値を必ず何も残さない
        gen(node->lhs);
        printf("#----- Expression statement\n");
        printf("    add rsp, 8\n");
        return;
    case ND_VAR: // 変数の値の参照
        gen_addr(node);

        if(node->ty->kind != TY_ARRAY)
            load(); // メモリアドレスからデータをレジスタにload
        return;
    case ND_ASSIGN: // ローカル変数(左辺値)への値(右辺値)の割り当て
        // 左辺のkindがTY_ARRAYではない場合のみ、左辺値として処理できる(arrayの形のままではどのアドレスに値を割り当てるかわからない)
        gen_lval(node->lhs); // =>最終的に計算結果を入れたraxの値(アドレス)がスタックにpushされる ...push rax
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
            expression statement: 文 => 値を残してはいけない
            - 式を評価して、結果を捨てる役割
            - 必ず値が残ってしまう「式」という存在を、値を残さない「文」という存在に変換する

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
        for(; n; n = n->next)
            gen(n);

        // ひとつひとつのstatementは一つの値をスタックに残すので、毎回ポップするのをわすれないこと
        return;
    }
    case ND_FUNCALL: { // 関数呼び出し
        printf("#----- Function call with up to 6 parameters. \n");
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next) {
            gen(arg);
            nargs++;
        }

        /*
        ・foo(a, b, c)
        スタック
            | ... |
            |  a  | => rdi
            |  b  | => rsi
            |  c  | => rdx
        */

        // 引数の数分、値をpopしてレジスタにセットする
        printf("#-- 引数をレジスタにセット \n");
        for(int i = nargs-1; i >= 0; i--)
            printf("    pop %s\n", argreg[i]);

        // 関数を呼ぶ前にRSPを調整して、RSPを16byte境界(16の倍数)になるようにアラインメントする
        // - push/popは8byte単位で変更するので、
        //   call命令を発行する時に必ずしもRSPが16の倍数になっているとは限らない
        // - ABI(System V)で決められている
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
        printf("#-- スタックフレームが16の倍数になっているかどうかチェック\n");
        printf("    and rax, 15\n");    // and: 論理積
        printf("#-- もし16の倍数でないならアライン操作するラベルにジャンプ\n");
        printf("    jnz .L.call.%d\n", seq);  // 結果が0でない(16の倍数でない)=>RSPのアラインメント操作が必要=>.L.call.XXXラベルへジャンプ
        printf("    mov rax, 0\n");     // raxに0をコピー
        printf("    call %s\n", node->funcname);  // 関数呼び出し
        printf("#-- 関数の実行終了のラベルにジャンプ\n");
        printf("    jmp .L.end.%d\n", seq);  // 関数呼び出しの結果がraxにセットされている=>.L.end.XXXラベルにジャンプ
        
        printf("#----- スタックフレームを16の倍数にアラインする\n");
        printf(".L.call.%d:\n", seq);   // RSPのアラインメント操作をする
        printf("#-- スタックフレームを8増やす\n");
        printf("    sub rsp, 8\n");     // スタックをひとつ増やす(push/popで8byteごとに操作しているので、8byte増やすことでRSPを16の倍数に調整)
        printf("    mov rax, 0\n");     // raxの値を0にセットする
        printf("    call %s\n", node->funcname);  // 関数呼び出し (RSPがアラインメントできたのでRSPが呼び出せる)
        printf("#-- アラインのために8増やしたスタックを減らして元に戻す\n");
        printf("    add rsp, 8\n");     // スタックをひとつ減らす(RSP調整のために足したスタックを引いておく)
        
        printf("#----- 関数の実行終了\n");
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
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        if(node->ty->kind != TY_ARRAY)
            load();
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
    case ND_PTR_ADD:
        printf("    imul rdi, %d\n", node->ty->base->size);  // この数値(rdi)はアドレスなので、8倍する
        printf("    add rax, rdi\n"); // num + num の形
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_PTR_SUB:
        printf("    imul rdi, %d\n", node->ty->base->size);  // この数値(rdi)はアドレスなので、8倍する
        printf("    sub rax, rdi\n"); // num - num の形
        break;
    case ND_PTR_DIFF:
        /*
            rax: dividend
            rdi: divisor

            cqo: raxに入っている64ビットの値を128ビットに伸ばしてRDXとRAXにセットする(RDX:RAX)
            idiv: 符号あり除算命令(Signed division)
            ・RDXとRAXを取ってそれを合わせたものを128ビット整数とみなし(RDX:RAX)それを引数レジスタの64ビットの値(SRC)で割る
                tmp = (RDX:RAX) / SRC
            ・商をRAXへ、あまりをRDXにセットする
                RAX = tmp
                RDX = RDE:RAX SignedModulus SRC
        */
        // 被除数(この場合はraxの値)をセット
        printf("    sub rax, rdi\n"); // rax = rax - rdi
        printf("    cqo\n");          // rax => (RDX:RAX)
        printf("    mov rdi, %d\n", node->lhs->ty->base->size);   // 8をrdiにコピーする
        printf("    idiv rdi\n");     // divide rax by rdi(=8)(引き算の結果は欲しい結果の8倍の値なので)
        // 欲しい結果はraxにセットされている
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

static void emit_data(Program *prog) {
    printf(".data\n");

    for(VarList *vl = prog->globals; vl; vl = vl->next) {
        Var *var = vl->var;
        printf("%s:\n", var->name);
        printf("    .zero %d\n", var->ty->size);
    }
}

static void emit_text(Program *prog) {
    printf(".text\n");

    for(Function *fn = prog->fns; fn; fn = fn->next) {
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
        for(VarList *vl = fn->params; vl; vl = vl->next) {
            Var *var = vl->var;
            printf("    mov [rbp-%d], %s\n", var->offset, argreg[i++]);
        }

        // Emit code
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

void codegen(Program *prog) {
    // アセンブリの前半部分
    printf(".intel_syntax noprefix\n");
    emit_data(prog);
    emit_text(prog);
}
