#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類 Token型で使う
typedef enum {
  TK_RESERVED, // 記号
  TK_NUM,      // 整数トークン 
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

/* ？ */
typedef struct Token Token;

// Token型 ポインタnextによって自己参照構造体になり、連結リストの形を取る
struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
};

// Token型のポインタをグローバル変数として定義
// 連結リストになっているtokenを辿っていくことで入力を読み進める
// consume, expect関数でしか使わない
Token *token;

char *user_input;

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

bool consume(char op){
  // 今着目しているトークンのkindが記号ではないか、
  // トークン文字列の1文字目が引数で期待した記号でない場合は偽を返す
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  // 期待した記号なら着目するトークンを次の要素に進める
  token = token->next;
  return true;
}

// トークンが期待している記号の場合はトークンを次に進める
// そうでないならエラーを報告する
void expect(char op){
  if (token->kind != TK_RESERVED || token->str[0] != op)
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
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));   // 新しいトークンのためのメモリ領域をゼロクリア
  tok->kind = kind;   
  tok->str = str;
  cur->next = tok; // 今作ったトークンを、着目しているトークンのnextへ繋げる
  return tok;
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

    // 記号がきたときは記号のトークンを作成して、curに繋げる
    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++); /* なぜpをインクリメントする？ */ 
      continue;
    }

    // 数字が来たときは、数字のトークンを作成してcurに繋げる
    // 数字と認識できるところまでを現在のトークンのvalへ格納し、
    // 残りをpのアドレスを使ってp自体に格納する
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10); 
      continue;
    }

    error_at(p, "トークナイズできません");
  }
  
  new_token(TK_EOF, cur, p);
  return head.next;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: 引数の数が正しくありません", argv[1]);
    return 1;
  }

  user_input = argv[1];
  token = tokenize();

  printf(".intel_syntax noprefix\n");
  printf(".globl _main\n");
  printf("_main:\n");

  // 式の最初が数であるかをチェックして、
  // 最初のmov命令を出力
  printf("  mov rax, %d\n", expect_number());

  // 着目しているトークンが終端になるまで
  // `+ <数>`あるいは`- <数>`というトークンの並びを消費していく
  while (!at_eof()) {
    // 着目しているトークンが＋なら、一つ進める
    // さらに、次のトークンが数字ならadd命令を出力
    if (consume('+')) {
      printf("  add rax, %d\n", expect_number());
      continue;
    }
    // ーでも同じ
    expect('-');
    printf("  sub rax, %d\n", expect_number());
  }

  printf("  ret\n");
  return 0;
}
