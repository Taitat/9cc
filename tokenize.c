#include "9cc.h"

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...) { // 可変長引数
  va_list ap;                // 可変長引数の格納
  va_start(ap, fmt);         // va_list型の初期化
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap); // 標準出力ではなく標準エラー出力の時は出力先の指定をするためfprintf系を使う
  fprintf(stderr, "\n");
  exit(1);
}

bool consume(char *op){
  // 今着目しているトークンのkindが記号ではないか、
  // トークン文字列の1文字目が引数で期待した記号でない場合は偽を返す
  if (token->kind != TK_RESERVED || 
      strlen(op) != token->len ||   // 渡ってきた記号と今のトークンのlenメンバの一致。先にこちらを比較することで'>='のような複数文字かどうかを検証する
      memcmp(token->str, op, token->len))  // 渡ってきた記号と今のトークンのstrメンバのメモリブロックを、今のトークンのバイト数分比較
    return false;
  // 期待した記号なら着目するトークンを次の要素に進める
  token = token->next;
  return true;
}

// トークンが期待している記号の場合はトークンを次に進める
// そうでないならエラーを報告する
void expect(char *op){
  if (token->kind != TK_RESERVED || 
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len)) 
    error_at(token->str, "'%c'ではありません", op);
  token = token->next;
}

// トークンが数値の場合はトークンを次に進める
// そうでないならエラーを報告する
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

// トークンが終端なら真を返す
bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成して、curに繋げる
/* curやstrはなぜポインタ？ */
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));   // 新しいトークンのためのメモリ領域をゼロクリア
  tok->kind = kind;   
  tok->str = str;
  tok->len = len;
  cur->next = tok; // 今作ったトークンを、着目しているトークンのnextへ繋げる
  return tok;
}

bool startwith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// 入力文字列のトークナイズ
/* *p は1文字？リスト？ポインタ？ */
Token *tokenize() { // *p には渡された文字列の1文字目が入る
  char *p = user_input;
  Token head;          // 先頭になるダミーのトークン。最終的にこれの次をreturnする。コードを簡単にするため。
  head.next = NULL;   // まだ何も読み込んでいないのでnextはNULL
  Token *cur = &head;  // 現在のトークンをまずは先頭トークンに設定

  while (*p) {
    // 空白ならポインタ変数のインクリメントによって次の1文字へ
    if (isspace(*p)) {
      p++;
      continue;
    }

    // 複数文字から成る演算子
    if (startwith(p, "==") || startwith(p, "!=") || 
        startwith(p, "<=") || startwith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // 1文字から成る演算子
    // 記号がきたときは記号のトークンを作成して、curに繋げる
    if (strchr("+-*/()<>", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1); /* なぜpをインクリメントする？ */ 
      continue;
    }

    // 数字が来たときは、数字のトークンを作成してcurに繋げる
    // 数字と認識できるところまでを現在のトークンのvalへ格納し、
    // 残りをpのアドレスを使ってp自体に格納する
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      // pを書き換える前に一旦qへコピーしておく
      char *q = p;
      cur->val = strtol(p, &p, 10);
      // 数字をとって残ったpのアドレスとqの差分をトークンの長さとしてlenメンバへセット
      cur->len = p - q; 
      continue;
    }

    error_at(p, "invalid token");
  }
  
  new_token(TK_EOF, cur, p, 0);
  return head.next;
}