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
    Node *node = program(); // locals連結リストが作成される

    // ローカル変数にオフセットを割り当て
    // int offset = 0;
    // for(LVar *v = locals; v; v->next) {
    //     offset += 8;
    //     v->offset = offset; // ここへの代入ができない
    // }

    // アセンブリコード生成
    // Traverse the AST to emit assembly.
    codegen(node);

    return 0;
}
