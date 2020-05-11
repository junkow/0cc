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
    Function *prog = program(); // locals連結リストが作成される

    // ローカル変数にオフセットを割り当て
    int offset = 0;
    for(Var *var = prog->locals; var; var = var->next) {
        offset += 8;
        var->offset = offset;
    }
    prog->stack_size = offset;

    // アセンブリコード生成
    // Traverse the AST to emit assembly.
    codegen(prog);

    return 0;
}
