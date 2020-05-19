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
    Function *prog = program(); // functionの連結リストが作成され、その先頭アドレス
    // それぞれのfunctionインスタンスごとにnodeやローカル変数のリストがメンバとして含まれている

    for(Function *fn = prog; fn; fn = fn->next) {
        // ローカル変数にオフセットを割り当て
        int offset = 0;
        Var *var = NULL;
        // 関数定義の引数
        for(var = fn->params; var; var = var->next) {
            offset += 8;
            var->offset = offset;
        }
        // 関数内のローカル変数
        for(var = fn->locals; var; var = var->next) {
            offset += 8;
            var->offset = offset;
        }
        fn->stack_size = offset;
    }
    
    // アセンブリコード生成
    // Traverse the AST to emit assembly.
    codegen(prog);

    return 0;
}
