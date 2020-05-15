#include "9cc.h"

Token *token;
char *user_input;
Var *locals; // ローカル変数

// エラーを報告してexitする
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー箇所を報告
// 第2引数以下はprintfと同じ引数をとる
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を入力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// parseの中で呼び出すことでtokenがnodeに変換される
// トークンはconsume/expect/expect_number関数の呼び出しの中で副作用としてひとつずつ読み進めている
// 次のトークンが期待している記号の時には、トークンを一つ読み進めて真を返す
// それ以外の場合には偽を返す
bool consume(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        strncmp(token->str, op, token->len))
        return false;
    token = token->next; // 副作用で一つトークンを進める
    return true;
}

// トークンが変数(識別子)の場合
Token *consume_ident() {
    if (token->kind != TK_IDENT)
        return NULL;
    Token *t = token; // アドレスを進める前に、現在のtokenのアドレスを変数に保存しておく
    token = token->next; // 副作用としてグローバル変数のtokenのアドレスを一つ進める
    return t; // アドレスが進む前のtokenのアドレスを返すことで、呼び出し先で現在注目しているtokenのプロパティ(token->strなど)を参照することができる
}

// 次のトークンが期待している記号の時には、トークンを一つ進めて真を返す
// それ以外の場合にはエラーを返す
void expect(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        strncmp(token->str, op, token->len))
        error_at(token->str, "expected \"%s\"", op);
    token = token->next; // 副作用で一つトークンを進める
}

// 次のトークンが数値の場合には、トークンを一つ進めて、その数値を返す
// それ以外の場合にはエラーを返す
long expect_number() {
    if(token->kind != TK_NUM)
        error_at(token->str, "数ではありません");
    long val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
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
    static char *kw[] = {"return", "if", "else", "while"};

    for(int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = strlen(kw[i]);
        // 文字列がkeywordを含んでいて、かつ次のひと文字が_またはalphabetまたは数値ではないこと
        if(startswith(p, kw[i]) && !is_alnum(p[len]))
            return kw[i];
    }

    return NULL;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(void) { // グローバル変数を使うので引数はvoidに変更
    char *p = user_input;
    Token head = {}; // ダミーの要素
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
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

        // Identifier: 識別子
        if(is_alpha(*p)) {
            char *q = p++;
            while(is_alnum(*p)) {
                p++;
            }
            cur = new_token(TK_IDENT, cur, q, p-q);
            continue;
        }

        // Multi-letter punctuators
        // 複数文字の方を先に書く
        if(startswith(p, "==") || startswith(p, "!=") ||
           startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2); // pの値を入力後pを2つ進める
            p += 2;
            continue;
        }

        // Single-letter punctuators
        if(strchr("+-*/()<>;=", *p)) {
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
    return head.next; // 先頭のダミーの次のアドレスなので、目的である先頭のトークンのアドレスを返す
}