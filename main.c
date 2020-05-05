#include "9cc.h"

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // トークナイズする
    user_input = argv[1];
    token = tokenize();
    // トークナイズしたものをパースする(抽象構文木の形にする)
    // 結果はToken *code[100];で定義されているcode変数に保存される
    program(); // locals連結リストが作成される

    // ローカル変数にオフセットを割り当て
    int offset = 0;
    for(LVar *var = locals; var; var->next) {
        offset += 8;
        var->offset = offset;
    }

    // アセンブリコード生成
    // アセンブリの前半部分
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // prologue
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", offset); // 予めa-zまでの変数のスペースを確保しておく

    // 先頭の式から順にコードを生成
    for (int i = 0; code[i]; i++) {
        // 抽象構文木を降りながらコード生成
        gen(code[i]);

        // 式の評価結果としてスタックに一つの値が残っているはずなので(gen関数の最後の一行)
        // スタックが溢れないようにpopしておく
        printf("    pop rax\n"); // スタックから値をpopしてraxレジスタにロードする
    }

    // epilogue
    // 最後の式の結果がRAXに残っているので、それをRBPにロードして関数からの返り値とする
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");

    return 0;
}
