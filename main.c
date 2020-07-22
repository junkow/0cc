#include "9cc.h"

FILE *output_file;

static char *input_path;
static char *output_path = "-";

static char *filename;
// 入力された文字列全体を受け取る変数
static char *user_input;

// 与えられたファイルのコンテンツを返す
static char *read_file(char *path) {
    FILE *fp;

    // Open and read the file.
    if (strcmp(path, "-") == 0) {
        // 慣例として、与えられたfilenameが"-"の場合はstdinから読み込む
        fp = stdin;
    } else {
        fp = fopen(path, "r");
        if(!fp)
            error("cannnot open %s: %s", path, strerror(errno));
    }

    int buflen = 4096; // 4 * 1024
    int nread = 0;
    char *buf = calloc(1, buflen);

    // Read the entire file
    for(;;) {
        int end = buflen - 2; // 末尾の"\n\0"のために、追加で2bytes用意する
        int n = fread(buf + nread, 1, end - nread, fp);
        if(n == 0)
            break;
        nread += n;
        if(nread == end) {
            buflen *= 2;
            buf = realloc(buf, buflen);
        }
    }

    if(fp != stdin)
        fclose(fp);

    // Canonicalize the last line by appending "\n\0"
    // if it does not end with a newline.
    // コンパイラの実装の都合上、全ての行が改行文字で終わっている方が、改行文字かEOFで
    // 終わっているデータよりも扱いやすいので、ファイルの最後のバイトが\nではない場合、
    // 自動的に\nを追加して正規化する
    if(nread == 0 || buf[nread-1] != '\n')
        buf[nread++] = '\n';
    buf[nread] = '\0';

    return buf;
}

int align_to(int n, int align) {
    // `n`の値を最も近い`align`の倍数に丸める

    // ~(align-1): alignの値の符号反転
    // AND演算子: ビットを反転したいところは0にしておく
    //           変えたくないところは1にしておく
    // ・ align_to(5, 8)
    //     00001110: n + align + 1
    // AND 11111000: ~(align-1)
    //     00001000: returns 8
    //
    // ・ align_to(11, 8)
    //     00010100: n + align + 1
    // AND 11111000: ~(align-1)
    //     00010000: => returns 16
    // return (n + align + 1) & ~(align-1);

    // ・ align_to(5, 8)
    // 00001110: n + align + 1
    // 00000001: Shift right by 3 bits (8 = 2^3)
    // 00001000: Shift left by 3 bits
    // 00001000: => returns 8
    //
    // ・ align_to(11, 8)
    // 00010100: n + aling + 1
    // 00000010: Shift right by 3 bits
    // 00010000: Shift left by 3 bits
    // 00010000: => returns 16
    return (n + align + 1) / align * align;
}

static void usage(int status) {
    fprintf(stderr, "9cc [ -o <path>] <file>\n");
    exit(status);
}

static void parse_args(int argc, char **argv) {
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "--help"))
            usage(0);

        if(!strcmp(argv[i], "-o")) {
            if(!argv[++i])
                usage(1);
            output_path = argv[i];
            continue;
        }

        if(!strncmp(argv[i], "-o", 2)) {
            output_path = argv[i] + 2;
            printf("debug: strncmp output_path: %s\n", output_path);
            continue;
        }

        if(argv[i][0] == '-' && argv[i][1] != '\0')
            error("unknown argument: %s", argv[i]);

        input_path = argv[i];
    }

    if(!input_path)
        error("no input files");
}

int main(int argc, char **argv) {
    // if(argc != 2) {
    //     fprintf(stderr, "引数の個数が正しくありません\n");
    //     return 1;
    // }
    parse_args(argc, argv);

    // Open the output file
    if(strcmp(output_path, "-") == 0) {
        output_file = stdout;
    } else {
        output_file = fopen(output_path, "w");
        if(!output_file)
            error("cannot open output file: %s: %s", output_path, strerror(errno));
    }

    // トークナイズする
    filename = input_path;
    user_input = read_file(input_path);
    token = tokenize(filename, user_input);
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

    // Emit a .file directice for the assembler.
    fprintf(output_file, ".file 1 \"%s\"\n", argv[1]);

    // アセンブリコード生成
    // Traverse the AST to emit assembly.
    codegen(prog);

    return 0;
}
