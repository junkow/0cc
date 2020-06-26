#include "9cc.h"

int align_to(int n, int align) {
    // TODO: 後で確認する
    // ~(align-1): alignの値の符号反転
    return (n + align + 1) & ~(align-1);
}

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // トークナイズする
    user_input = argv[1];
    token = tokenize();
    // トークナイズしたものをパースする(抽象構文木の形にする)
    Program *prog = program(); // functionの連結リストが作成され、その先頭アドレス
    // それぞれのfunctionインスタンスごとにnodeやローカル変数のリストがメンバとして含まれている

    for(Function *fn = prog->fns; fn; fn = fn->next) {
        // ローカル変数にオフセットを割り当て
        int offset = 0;
        // 関数内のローカル変数
        for(VarList *vl = fn->locals; vl; vl = vl->next) {
            Var *var = vl->var;
            offset += var->ty->size;
            var->offset = offset;
        }
        fn->stack_size = align_to(offset, 8);
    }
    
    // アセンブリコード生成
    // Traverse the AST to emit assembly.
    codegen(prog);

    return 0;
}
