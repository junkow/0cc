#include "9cc.h"

Token *token;

// Input filename
static char *current_input;

// Input string
static char *current_filename;

// エラーを報告してexitする
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Reports an error message in the following format.
//
// foo.c:10: x = y + 1;
//               ^ <error message here>
static void verror_at(int line_no, char *loc, char *fmt, va_list ap) {
    // `loc`を含んでいる行を見つける
    char *line = loc;
    while(current_input < line && line[-1] != '\n')
        line--;

    char *end = loc;
    while (*end != '\n')
        end++;

    // その行を表示する
    int indent = fprintf(stderr, "%s:%d: ", current_filename, line_no);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // エラーメッセージを表示
    int pos = loc - line + indent;

    fprintf(stderr, "%*s", pos, ""); // pos個の空白を入力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

// エラー箇所を報告し、exitする
// 第2引数以下はprintfと同じ引数をとる
void error_at(char *loc, char *fmt, ...) {
    int line_no = 1;
    for (char *p = current_input; p < loc; p++)
        if(*p == '\n')
            line_no++;

    va_list ap;
    va_start(ap, fmt);
    verror_at(line_no, loc, fmt, ap);
    exit(1);
}

// エラー箇所を報告し、exitする
void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    verror_at(tok->line_no, tok->str, fmt, ap);
    exit(1);
}

void warn_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->line_no, tok->str, fmt, ap);
}

// parseの中で呼び出すことでtokenがnodeに変換される
// トークンはconsume/expect/expect_number関数の呼び出しの中で副作用としてひとつずつ読み進めている
// 次のトークンが期待している記号の時には、トークンを一つ読み進める
// 現在のトークンを返す
Token *consume(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        strncmp(token->str, op, token->len))
        return NULL;
    Token *t = token;
    token = token->next; // 副作用で一つトークンを進める
    return t;
}

// トークンが変数(識別子)の場合
Token *consume_ident(void) {
    if (token->kind != TK_IDENT)
        return NULL;
    Token *t = token; // アドレスを進める前に、現在のtokenのアドレスを変数に保存しておく
    token = token->next; // 副作用としてグローバル変数のtokenのアドレスを一つ進める
    return t; // アドレスが進む前のtokenのアドレスを返すことで、呼び出し先で現在注目しているtokenのプロパティ(token->strなど)を参照することができる
}

// 現在のtokenが与えられたstringに一致していたら、そのままTokenインスタンスを、そうでなければNULLを返す
// トークンは進めない
Token *peek(char *s) {
    if(token->kind != TK_RESERVED || strlen(s) != token->len || 
        strncmp(token->str, s, token->len))
        return NULL;
    return token;
}

// 次のトークンが期待している記号の時には、トークンを一つ進めて真を返す
// それ以外の場合にはエラーを返す
void expect(char *s) {
    if (!peek(s))
        error_tok(token, "expected \"%s\"", s);
    token = token->next; // 副作用で一つトークンを進める
}

// 次のトークンが数値の場合には、トークンを一つ進めて、その数値を返す
// それ以外の場合にはエラーを返す
long expect_number(void) {
    if(token->kind != TK_NUM)
        error_tok(token, "数ではありません");
    long val = token->val;
    token = token->next;
    return val;
}

// トークンが識別子かどうか
// 識別子の場合はその識別子の文字列を返す、トークンを一つすすめる
// それ以外はエラーを出力してexit
char *expect_ident(void) {
    if(token->kind != TK_IDENT)
        error_tok(token, "expected an identifier");
    char *s = strndup(token->str, token->len);
    token = token->next;
    return s;
}

bool at_eof(void) {
    return token->kind == TK_EOF;
}

//　新しいトークンを作成して、curにつなげる
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok; // currentのtokenのアドレスの次に、新しく作成したtokenを指定する
    return tok;
}

static bool startswith(char *p, char *q) {
    return strncmp(p, q, strlen(q)) == 0;
}

// Is alphabet or not
static bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           (c == '_');
}

// Is alphabet or number or not
static bool is_alnum(char c) {
    return is_alpha(c) || ('0' <= c && c <= '9');
}

// If the string includes the reserved keywords returns the keyword.
static char *is_keyword(char *p) {
    // Keywords
    static char *kw[] = {
        "return", "if", "else", "while", "for", "int", "char", "sizeof",
        "struct", "union", "short", "long", "void"
    };

    for(int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = strlen(kw[i]);
        // 文字列がkeywordを含んでいて、かつ次のひと文字が_またはalphabetまたは数値ではないこと
        if(startswith(p, kw[i]) && !is_alnum(p[len]))
            return kw[i];
    }

    return NULL;
}

static bool is_hex(char c) {
    return ('0' <= c && c <= '9') ||
           ('a' <= c && c <= 'f') ||
           ('A' <= c && c <= 'F');
}

static int from_hex(char c) {
    if('0' <= c && c <= '9')
        return c - '0';
    if('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return c - 'A' + 10;
}

// エスケープシーケンス'\'に対応
static char get_escape_char(char **new_pos, char *p) {
    // 呼び出し先のloopでpのアドレスを進めるか、この中でアドレスを進める必要がある
    // 引数new_posを使って呼び出し先の文字列pのアドレスを進める
    // - 呼び出し先の場合、文字だけの時はアドレスを1だけ進めれば良かったので、p++で対応できた。
    // - 8進数を読んだときはその桁数によって進める数が変わってくるので、この関数内でその分を進める方がわかりやすい

    // 8進数を読む
    // "\<octal-sequence>"
    if('0' <= *p && *p <= '7') {
        // まず一の位を計算
        int c = *p++ - '0'; // 計算をしたいからcharではなくてintで処理している?
        if('0' <= *p && *p <= '7') {
            // 桁上がりなので、前の結果cを8倍してから、一の位を計算して足す
            c = (c*8) + (*p++ - '0');
            if('0' <= *p && *p <= '7') {
                // 桁上がりなので、前の結果cを8倍してから、一の位を計算して足す
                c = (c*8) + (*p++ - '0');
            }
        }

        // 8進数を読んだあとの位置にpのアドレスが進んでいる
        *new_pos = p;
        return c;
    }

    // 16進数を読む
    // "\x<hexadecimal-sequence>"
    if(*p == 'x') {
        p++;

        if(!is_hex(*p))
            error_at(p, "invalid hex escape sequence");

        int c = 0;
        for(; is_hex(*p); p++) {
            c = (c * 16) + from_hex(*p);
            if(c > 255)
                error_at(p, "hex escape sequence out of range");
        }

        *new_pos = p;
        return c;
    }

    // pのアドレスを1つ(これから読む文字1文字分)だけ進める
    *new_pos = p+1;

    switch(*p) {
    case 'a': return '\a'; // 7 ベル(警告音)
    case 'b': return '\b'; // 8 バックスペース
    case 't': return '\t'; // 9 水平タブ
    case 'n': return '\n'; // 10 ラインフィード(改行)
    case 'v': return '\v'; // 11 垂直タブ
    case 'f': return '\f'; // 12 フォームフィード
    case 'r': return '\r'; // 13 キャリッジリターン
    case 'e': return 27;
    default: return *p;
    }
}

static Token *read_string_literal(Token *cur, char *start) {
    char *p = start + 1; // 先頭の'"'分を進める
    char *end = p;

    // 文字列の終端の'"'を見つける
    for(; *end != '"'; end++) { 
        if(*end == '\0')
            error_at(start, "unclosed string literal");
        if(*end == '\\') // もし'\'ならアドレスを進める
            end++;
    }

    // 文字列全体を含めるのに十分なバッファをallocateする
    char *buf = calloc(1, end - p + 1); // 文字の長さ分 + 1(後で追加する'\0'の分)
    int len = 0;     // 文字数をカウント

    while(*p != '"') {
        if (*p == '\\') { // '\'は92
            // pを'\'の分、1だけを進める
            buf[len++] = get_escape_char(&p, p + 1); // エスケープ文字としてバッファに追加
            // get_escape_char関数で副作用として、pを進めている
        } else {
            buf[len++] = *p++;  // 通常の文字ならそのままバッファに追加
        }
    }

    // ここでpは末尾の'"'を指している
    // lenは文字の個数を指している

    buf[len] = '\0'; // bufの最後に終端文字'\0'をセット

    Token *tok = new_token(TK_STR, cur, start, p - start + 1); // ""も含めた文字列の長さ。"abc"ならlen = 5
    tok->contents = buf;
    tok->cont_len = len+1;  // 文字数 + '\0'(終端文字)
    return tok;
}

static void add_line_numbers(Token *tok) {
    char *p = current_input;
    int n = 1;

    do {
        if (p == tok->str) {
            tok->line_no = n;
            tok = tok->next;
        }

        if (*p == '\n') {
            n++;
        }
    } while(*p++);
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *filename, char *p) {
    current_filename = filename;
    current_input = p;

    Token head = {}; // ダミーの要素
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Skip line comments
        // 一行コメントを読み飛ばす
        if (startswith(p, "//")) {
            p += 2;
            while(*p != '\n')
                p++;
            continue;
        }

        // Skip block comments
        // ブロックコメントを読み飛ばす
        if (startswith(p, "/*")) {
            char *q = strstr(p+2, "*/");
            if(!q)
                error_at(p, "unclosed block comment");
            p = q+2;
            continue;
        }

        // 文字列リテラル
        if (*p == '"') {
            cur = read_string_literal(cur, p);
            p += cur->len;
            continue;
        }

        // Keywords
        char *kw = is_keyword(p);
        if(kw) {
            int len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        // Multi-letter punctuators
        // 複数文字の方を先に書く
        if(startswith(p, "==") || startswith(p, "!=") ||
           startswith(p, "<=") || startswith(p, ">=") ||
           startswith(p, "->")) {
            cur = new_token(TK_RESERVED, cur, p, 2); // pの値を入力後pを2つ進める
            p += 2;
            continue;
        }

        // Identifier: 識別子
        if(is_alpha(*p)) {
            char *q = p++;
            while(is_alnum(*p)) {
                p++;
            }
            cur = new_token(TK_IDENT, cur, q, p-q);
            continue;
        }

        // Single-letter punctuators
        if(ispunct(*p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1); // pの値を入力後pをひとつ進める
            continue;
        }

        if(isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10); // ここでpのアドレスがひとつ進む
            cur->len = p - q;
            continue;
        }

        error_at(p, "invalid token");
    }

    new_token(TK_EOF, cur, p, 0);
    add_line_numbers(head.next);
    return head.next; // 先頭のダミーの次のアドレスなので、目的である先頭のトークンのアドレスを返す
}
