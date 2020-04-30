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
    program();

    // アセンブリコード生成
    // アセンブリの前半部分
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // prologue
    // 変数26個分の領域を確保する 26*8(バイト) = 208バイト
    // rbp: ベースレジスタ(値はベースポインタ)
    // rsp: スタックポインタ
    // スタックは下に成長するのでsub(減算)になる
    // call命令によってリターンアドレスがスタックに詰まれている状態
    printf("    push rbp\n");     // rbpの値(ベースポインタ)をメモリスタックにpush
    // スタックには関数呼び出し時点での関数フレーム開始位置が詰まれる
    printf("    mov rbp, rsp\n"); // rbpにrspの値をコピー(rbpはスタックトップに移動)
    printf("    sub rsp, 208\n"); // この時、rspはベースポインタ(rbp)に等しいので、ここから必要なメモリの領域を確保する
    // 関数フレームができる。RSPはスタックトップ(関数フレームの終端位置)を指す

    // 先頭の式から順にコードを生成
    for (int i = 0; code[i]; i++) {
        // 抽象構文木を降りながらコード生成
        gen(code[i]);

        // 式の評価結果としてスタックに一つの値が残っているはずなので
        // スタックが溢れないようにpopしておく
        printf("    pop rax\n"); // スタックから値をpopしてraxレジスタにロードする
    }

    // epilogue
    // 最後の式の結果がRAXに残っているので、それをRBPにロードして関数からの返り値とする
    printf("    mov rsp, rbp\n"); // rspにrbpの値をコピー(RSPの指す位置をRBPと同じ位置に戻す)
    printf("    pop rbp\n");      // スタックの値をrbpにロードする=>rbpは一つ前の関数呼び出し時点の関数フレーム開始位置に移動する
    printf("    ret\n");          // 現在の関数のリターンアドレスをポップして、そのアドレスにジャンプする

    return 0;
}
