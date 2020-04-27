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
    Node *node = expr();

    // アセンブリコード生成
    codegen(node);

    return 0;
}
