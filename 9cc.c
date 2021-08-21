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
  Token *next; // nextはToken型のポインタ。次のトークンのメモリ位置が入っている。
  int val;
  char *str;
  int len;
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

// 
// Parser
//

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val;
};

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// 左辺と右辺の情報持つ二項演算子のノード
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 数値のノード
Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

Node *expr();
Node *mul();
Node *unary();
Node *primary();

// expr = mul("+" mul | "-" mul)*
Node *expr() {
  // ①mul()の返り値を得る
  Node *node = mul();

  for(;;){
    // ②次のトークンが'+' or '-'ならトークンを次に進める。
    if (consume('+'))
      // ③二項演算子のノードを作る
      // 左辺が①の結果、右辺が②によって進んだ次のトークンをmul()にかけたもの。
      node = new_binary(ND_ADD, node, mul());
    else if (consume('-'))
      node = new_binary(ND_SUB, node, mul());
    else
      // 何かしらの式が来た時には、exprかmulのfor文によって構文木のトップは二項演算子ノードになる
      // ただの数字が来た時のみ、数字のノードとして返却される
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume('*'))
      node = new_binary(ND_MUL, node, unary());
    else if (consume('/'))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-")? unary | primary
// 単項演算子用
Node *unary() {
  // '+'は意味がなく右辺そのものと同義なので、ただトークンを進める
  if (consume('+'))
    return unary();
  // '-'は右辺の反転の意味なので、次のトークン（右辺）を０から引いて反転させる式のノードを作る
  if (consume('-'))
    return new_binary(ND_SUB, new_num(0), unary());
  return primary();
}

// primary = "(" expr ")" | num
// primaryはexprを再帰的に呼び出すか、数値をそのまま返す。
Node *primary() {
  if (consume('(')) {
    // '(' が来たということは、その次に式が来るはずなので、再びexpr()を呼び出して部分木を構築する
    Node *node = expr();
    // 式が消費できなくなった時点でexpr()はreturnされる。
    // その直後に')'が来ていれば式の終わりなので、トークンを進めて、ここまで括弧内の式で構築した部分木を返す。
    expect(')');
    return node;
  }

  // 現在のトークンが数値の場合はここまで降りてきて、そのまま数値のノードを作って返す
  // (さらにexpect_number によってトークンが進む)
  return new_num(expect_number());
}

//
// Code generator
//

// pushやpopはx86-64のRSPレジスタを暗黙的にスタックポインタとして使用し、
// その値を変更しつつ、RSPが指しているメモリにアクセスする
void gen(Node *node) {
  // 数値が来たときはそのままスタックマシンへpush
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  // 数値以外の時は二項演算子ノードのはずなので、左辺と右辺をそれぞれ再帰的にgen()する
  gen(node->lhs);
  gen(node->rhs);

  // スタックトップから二つ取り出してレジスタにロード
  printf("  pop rdi\n");
  printf("  pop rax\n");

  // それぞれの演算子に対応する命令を書き込む
  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n"); // RAXに入っている64bitの値を128bitに伸ばして、RDXとRAXにセット
    // idivは、RDXとRAXを合わせたものを128bit整数と見做して、それを引数のレジスタ(RDI)の64bitの値で割り、
    // 商をRAXへ、余りをRDXへセットする。その下準備に上記のcqoが必要。
    printf("  idiv rdi\n");
    break;
  }

  printf("  push rax\n");

}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: 引数の数が正しくありません", argv[0]);
  }

  // Tokenize and parse.
  user_input = argv[1];
  token = tokenize();
  Node *node = expr();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".globl _main\n");
  printf("_main:\n");

  // 抽象構文木を降りながらコード生成
  gen(node);

  // スタックトップに式全体の値が残っているはずなので、
  // それをRAXにロードして関数からの返り値とする。
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
