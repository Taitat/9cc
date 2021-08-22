#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *user_input;


//
// Tokenizer
//

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

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool startwith(char *p, char *q);
Token *tokenize();


// 
// Parser
//

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
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

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_num(int val);
Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();


//
// Code generator
//

void gen(Node *node);

