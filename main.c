#include "9cc.h"

// 与えられたファイルのコンテンツを返す
static char *read_file(char *path) {
    // Open and read the file.
    FILE *fp = fopen(path, "r");
    if(!fp)
        error("cannnot open %s: %s", path, strerror(errno));

    int filemax = 10 * 1024 * 1024; // 10MiB
    char *buf = malloc(filemax);
    int size = fread(buf, 1, filemax - 2, fp);

    if(!feof(fp))
        error("%s: file too large");

    // Make sure that the string ends with "\n\0"
    // コンパイラの実装の都合上、全ての行が改行文字で終わっている方が、改行文字かEOFで
    // 終わっているデータよりも扱いやすいので、ファイルの最後のバイトが\nではない場合、
    // 自動的に\nを追加する
    if(size == 0 || buf[size-1] != '\n')
        buf[size++] = '\n';
    buf[size] = '\0';
    return buf;
}

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
    filename = argv[1];
    user_input = read_file(argv[1]);
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
