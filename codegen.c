#include "9cc.h"

// x86_64のABIで規定されている引数をセットするレジスタのリスト(引数の順番と同じ)
static char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"}; // 8bitレジスタ
static char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int labelseq = 1;
static char *funcname;

static int cur_line_no = 0;

static void println(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(output_file, fmt, ap);
    fprintf(output_file, "\n");
}

static void gen(Node *node);

// ローカル変数のアドレスの取得
static void gen_addr(Node *node) {
    switch(node->kind) {
    case ND_VAR: {
        Var *var = node->var;
        if (var->is_local) {
            println("#----- Local variable");
            println("#----- Pushes the given node's memory address to the stack.");
            // srcオペランドのメモリアドレスを計算し、distオペランドにロードする
            println("# DEBUG: var->name: %s", var->name);
            println("    lea rax, [rbp-%d]", var->offset); // lea : load effective address
            println("    push rax"); // raxの値(ローカル変数のメモリアドレス)をスタックにpushする
        } else {
            println("#----- Global variable");
            println("    push offset %s", var->name);
        }
        return;
    }
    case ND_DEREF: // 左辺値に逆参照がきた場合に処理できるようにする
        gen(node->lhs); // 左辺を展開する
        return;
    case ND_COMMA:
        // TODO: あとで確認する
        println("#----- Comma operator");
        gen(node->lhs);
        println("    add rsp, 8");
        gen_addr(node->rhs);
        return;
    case ND_MEMBER:
        gen_addr(node->lhs);
        println("    pop rax");
        println("    add rax, %d", node->member->offset);
        println("    push rax");
        return;
    }

    error_tok(node->tok, "not an lvalue");
}

static void gen_lval(Node *node) {
    if(node->ty->kind == TY_ARRAY) // arrayの形のままの場合は左辺値ではない(アドレスが取れない)のでエラー
        error_tok(node->tok, "not an lvalue");
    gen_addr(node);
}

static void load(Type *ty) {
    if(ty->kind == TY_ARRAY || ty->kind == TY_STRUCT) {
        // If it is an array, do nothing because in general we can't load 
        // an entire array to a register. 
        // (The data amount is too large to store it to an register.)
        // As a result, the result of an evalutation of
        // an array becomes not the array itself but the address of the array.
        // In other words, this is where
        // "array is automatically converted to a pointer to the first element of the array in C"
        // occurs.
        return;
    }

    println("#----- Load a value from the memory address.");
    println("    pop rax"); // スタックトップからローカル変数のアドレスをpopしてraxに保存する
    
    if( ty->size == 1 )
        // movsx命令 符号拡張が不要
        println("    movsx rax, byte ptr [rax]"); // raxの指しているアドレスから1バイトの読みこみ、raxにセット
    else
        println("    mov rax, [rax]"); // raxに入っている値をアドレスとみなして、そのメモリアドレスから値をロードしてraxレジスタにコピーする
    println("    push rax"); // raxの値をスタックにpush
}

static void store(Type *ty) {
    println("#----- Store a value to the memory address.");

    println("    pop rdi"); // スタックトップの値(右辺値)をrdiにロードする
    println("    pop rax"); // スタックトップの値(アドレス)をraxにロードする

    if(ty->kind == TY_STRUCT) {
        println("#DEBUG: ---- TY_STRUCT\n");
        // for(int i = 0; i < ty->size; i++) {
        //     /*
        //         e.g.
        //         struct t {int a; int b;} x;
        //         struct t y;
        //         y = x;
        //     */
        //     // rdi: 変数xのアドレス(assignする値を持っている)
        //     // rax: 変数yのアドレス(assignされる側)
        //     // 1バイトずつ値をコピー?
        //     // println("#    movsx rsi, byte ptr [rdi+%d]", i);
        //     // println("#    mov [rax+%d], sil", i);
        // }

        // println("    mov rdi, [rdi]");
        // println("    mov [rax], rdi");
    } else if ( ty->size == 1 ) {
        println("    mov [rax], dil"); // 1バイトの書き出し
    } else {
        println("    mov [rax], rdi"); // raxに入っている値をアドレスとみなし、そのメモリアドレスにrdiに入っている値をストア
    }

    println("    push rdi"); // rdiの値をスタックにpush
}

// 抽象構文木からアセンブリコードを生成する
static void gen(Node *node) {
    if (node->tok->line_no != cur_line_no) {
        println("    .loc 1 %d", node->tok->line_no);
        cur_line_no = node->tok->line_no;
    }

    switch(node->kind) {
    case ND_NULL: // Empty statement
        return;
    case ND_NUM:
        println("    push %ld", node->val);
        return;
    case ND_EXPR_STMT:
        // expression (式):  値を一つ必ず残す
        // statement (文):  値を必ず何も残さない
        println("#----- Expression statement");
        gen(node->lhs);
        println("    add rsp, 8");
        return;
    case ND_VAR: // 変数の値の参照
    case ND_MEMBER: // structのmemberへのアクセス
        gen_addr(node);

        load(node->ty); // メモリアドレスからデータをレジスタにload
        return;
    case ND_ASSIGN: // ローカル変数(左辺値)への値(右辺値)の割り当て
        // gen_lvalでTY_ARRAYの場合はエラーを出力
        // 左辺のkindがTY_ARRAYではない場合のみ、左辺値として処理できる(arrayの形のままではどのアドレスに値を割り当てるかわからない)
        gen_lval(node->lhs); // =>最終的に計算結果を入れたraxの値(アドレス)がスタックにpushされる ...push rax
        gen(node->rhs); // =>最終的に計算結果を入れたraxの値(右辺値)がスタックにpushされる ...push rax

        // メモリアドレスへのデータのstore
        store(node->ty);
        return;
    case ND_IF: {
        int seq = labelseq++;

        println("#----- \"If\" statement");
        if(node->els) {
            gen(node->cond); // expr Aをコンパイルしたコード スタックトップに値が積まれているはず
            println("    pop rax");
            println("    cmp rax, 0");
            println("    je  .L.else.%d", seq);
            gen(node->then); // stmt
            println("    jmp .L.end.%d", seq);
            println(".L.else.%d:", seq);
            gen(node->els);  // stmt
            println(".L.end.%d:", seq);
        } else {
            gen(node->cond); // expr Aをコンパイルしたコード スタックトップに値が積まれているはず
            println("    pop rax");
            println("    cmp rax, 0");
            println("    je  .L.end.%d", seq);
            gen(node->then); // stmt
            println(".L.end.%d:", seq);
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
        println("#----- \"While\" statement");
        println(".L.begin.%d:", seq);
        gen(node->cond);    // Aをコンパイルしたコード
        println("    pop rax");
        println("    cmp rax, 0");
        println("    je  .L.end.%d", seq);
        gen(node->then);    // Bをコンパイルしたコード
        println("    jmp .L.begin.%d", seq);
        println(".L.end.%d:", seq);
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
        println("#----- \"For\" statement");
        if(node->init)
            gen(node->init); // the code which compiled A
        println(".L.begin.%d:", seq);
        if(node->cond) {
            gen(node->cond); // the code which compiled B
            println("    pop rax");
            println("    cmp rax, 0");
            println("    je  .L.end.%d", seq);
        }
        gen(node->then); // the code which compiled D
        if(node->inc)
            gen(node->inc);  // the code which compiled C
        println("    jmp .L.begin.%d", seq);
        println(".L.end.%d:", seq);
        return;
    }
    case ND_BLOCK:
    case ND_STMT_EXPR: {
        if(node->body) {
            Node *n = node->body; // statementのリストの先頭
            println("#----- Block {...} or Statement expression");
            for(; n; n = n->next)
                gen(n);
        }
        // ひとつひとつのstatementは一つの値をスタックに残すので、毎回ポップするのをわすれないこと
        // TODO: bodyが空の時にエラーを表示する
        return;
    }
    case ND_COMMA:
        // TODO: あとで確認
        println("#----- Comma operator. ");
        gen(node->lhs);
        println("    add rsp, 8"); // 最後の式の結果以外を捨てる
        gen(node->rhs);
        return;
    case ND_FUNCALL: { // 関数呼び出し
        println("#----- Function call with up to 6 parameters. ");
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
        println("#-- 引数をレジスタにセット ");
        for(int i = nargs-1; i >= 0; i--)
            println("    pop %s", argreg8[i]);

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
        println("    mov rax, rsp");   // rspの値をraxにコピーする
        println("#-- スタックフレームが16の倍数になっているかどうかチェック");
        println("    and rax, 15");    // and: 論理積
        println("#-- もし16の倍数でないならアライン操作するラベルにジャンプ");
        println("    jnz .L.call.%d", seq);  // 結果が0でない(16の倍数でない)=>RSPのアラインメント操作が必要=>.L.call.XXXラベルへジャンプ
        println("    mov rax, 0");     // raxに0をコピー
        println("    call %s", node->funcname);  // 関数呼び出し
        println("#-- 関数の実行終了のラベルにジャンプ");
        println("    jmp .L.end.%d", seq);  // 関数呼び出しの結果がraxにセットされている=>.L.end.XXXラベルにジャンプ
        
        println("#----- スタックフレームを16の倍数にアラインする");
        println(".L.call.%d:", seq);   // RSPのアラインメント操作をする
        println("#-- スタックフレームを8増やす");
        println("    sub rsp, 8");     // スタックをひとつ増やす(push/popで8byteごとに操作しているので、8byte増やすことでRSPを16の倍数に調整)
        println("    mov rax, 0");     // raxの値を0にセットする
        println("    call %s", node->funcname);  // 関数呼び出し (RSPがアラインメントできたのでRSPが呼び出せる)
        println("#-- アラインのために8増やしたスタックを減らして元に戻す");
        println("    add rsp, 8");     // スタックをひとつ減らす(RSP調整のために足したスタックを引いておく)
        
        println("#----- 関数の実行終了");
        println(".L.end.%d:", seq);    // 終了処理
        println("    push rax");       // raxの値(関数呼び出しの結果)をスタックにプッシュ
        return;
    }
    case ND_RETURN:
        gen(node->lhs); // returnの返り値になっている式のコードが出力される

        // 関数呼び出し元に戻る
        println("#----- Returns to the caller address.");
        println("    pop rax"); // スタックトップから値をpopしてraxにセットする
        println("    jmp .L.return.%s", funcname); // .L.returnラベルにジャンプ
        return;
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        load(node->ty);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    println("    pop rdi");
    println("    pop rax");

    switch(node->kind) {
    case ND_ADD:
        println("    add rax, rdi");
        break;
    case ND_PTR_ADD:
        println("    imul rdi, %d", node->ty->base->size);  // この数値(rdi)はアドレスなので、basetypeのsizeにscaleを合わせる
        println("    add rax, rdi"); // num + num の形
        break;
    case ND_SUB:
        println("    sub rax, rdi");
        break;
    case ND_PTR_SUB:
        println("    imul rdi, %d", node->ty->base->size);  // この数値(rdi)はアドレスなので、basetypeのsizeにscaleを合わせる
        println("    sub rax, rdi"); // num - num の形
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
        println("    sub rax, rdi"); // rax = rax - rdi
        println("    cqo");          // rax => (RDX:RAX)
        println("    mov rdi, %d", node->lhs->ty->base->size);   // 8をrdiにコピーする
        println("    idiv rdi");     // divide rax by rdi(=8)(引き算の結果は欲しい結果の8倍の値なので)
        // 欲しい結果はraxにセットされている
        break;
    case ND_MUL:
        println("    imul rax, rdi");
        break;
    case ND_DIV:
        // raxに入っている64ビットの値を128ビットに伸ばしてRDXとRAXにセットする
        println("    cqo");
        // idiv: 符号あり除算命令
        println("    idiv rax, rdi");
        break;
    case ND_EQ:
        // popしてきた値をcompareする
        println("    cmp rax, rdi");
        // alはraxの下位8ビット
        println("    sete al");
        // movzb: rax全体を0か1にするために上位56ビットをゼロクリアする
        println("    movzb rax, al");
        break;
    case ND_NE:
        println("    cmp rax, rdi");
        println("    setne al");
        println("    movzb rax, al");
        break;
    case ND_LT:
        println("    cmp rax, rdi");
        println("    setl al");
        println("    movzb rax, al");
        break;
    case ND_LE:
        println("    cmp rax, rdi");
        println("    setle al");
        println("    movzb rax, al");
        break;
    }

    println("    push rax");
}

// リテラルの文字列はスタック上に存在している値ではなく、メモリ上の固定の位置に存在している
// なので、文字列リテラルを表すためにはグローバル変数を使用する
static void emit_data(Program *prog) {
    println(".data");

    for(VarList *vl = prog->globals; vl; vl = vl->next) {
        Var *var = vl->var;
        println("%s:", var->name);

        // global変数の場合
        if (!var->contents) {
            println("    .zero %d", var->ty->size);
            continue;
        }

        // 文字列リテラルの場合
        for( int i = 0; i < var->cont_len; i++ ) {
            println("    .byte %d", var->contents[i]);
        }
    }
}

static void load_arg(Var *var, int idx) {
    int sz = var->ty->size;
    if(sz == 1) {
        println("    mov [rbp-%d], %s", var->offset, argreg1[idx]);
    } else {
        assert(sz == 8);
        println("    mov [rbp-%d], %s", var->offset, argreg8[idx]);
    }
}

static void emit_text(Program *prog) {
    println(".text");

    for(Function *fn = prog->fns; fn; fn = fn->next) {
        println(".global %s", fn->name);
        println("%s:", fn->name);
        funcname = fn->name;

        // Prologue
        println("#----- Prologue");
        println("    push rbp");
        println("    mov rbp, rsp");
        println("    sub rsp, %d", fn->stack_size);

        // 関数の引数をローカル変数のようにスタックにpushする
        int i = 0;
        for(VarList *vl = fn->params; vl; vl = vl->next)
            load_arg(vl->var, i++);

        // Emit code
        for (Node *node = fn->node; node; node = node->next) {
            // 抽象構文木を降りながらコード生成
            gen(node);
        }

        // Epilogue
        println("#----- Epilogue");
        println(".L.return.%s:", funcname);     // ラベル(`.L`はファイルスコープ)
        println("    mov rsp, rbp");
        println("    pop rbp");
        println("    ret");
    }
}

void codegen(Program *prog) {
    // アセンブリの前半部分
    println(".intel_syntax noprefix");
    emit_data(prog);
    emit_text(prog);
}
