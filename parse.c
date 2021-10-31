#include "9cc.h"

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
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
Node *assign();
Node *stmt();
void program();


Node *code[100];

// assign = equality ( "=" assign)?
Node *assign(){
  Node *node = equality();
  if (consume("="))
    node = new_binary(ND_ASSIGN, node, assign());
  return node;
}

// expr = assign
Node *expr() {
  return assign();
}

Node *stmt() {
  Node *node = expr();
  expect(";");
  return node;
}

//equality = relational ("==" relational | "!=" relational)*
Node *equality(){
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_binary(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_binary(ND_NE, node, relational());
    else
      return node;
  }
}

// expr ";"
void program() {
  int i = 0;
  // 終端を示すトークンが来るまで、ステートメントごとにグローバル変数 code に格納していく
  // 終端が来ると、目印にNULLを格納する
  while (!at_eof())
    code[i++] = stmt();
  code[i] = NULL;
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_binary(ND_LT, node, add());
    else if (consume("<="))
      node = new_binary(ND_LE, node, add());
    else if (consume(">"))
      node = new_binary(ND_LT, add(), node);
    else if (consume(">="))
      node = new_binary(ND_LE, add(), node);
    else
      return node;
  }
}

Node *add() {
  // ①mul()の返り値を得る
  Node *node = mul();

  for(;;){
    // ②次のトークンが'+' or '-'ならトークンを次に進める。
    if (consume("+"))
      // ③二項演算子のノードを作る
      // 左辺が①の結果、右辺が②によって進んだ次のトークンをmul()にかけたもの。
      node = new_binary(ND_ADD, node, mul());
    else if (consume("-"))
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
    if (consume("*"))
      node = new_binary(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-")? unary | primary
// 単項演算子用
Node *unary() {
  // '+'は意味がなく右辺そのものと同義なので、ただトークンを進める
  if (consume("+"))
    return unary();
  // '-'は右辺の反転の意味なので、次のトークン（右辺）を０から引いて反転させる式のノードを作る
  if (consume("-"))
    return new_binary(ND_SUB, new_num(0), unary());
  return primary();
}


LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}
// primary = "(" expr ")" | num
// primaryはexprを再帰的に呼び出すか、数値をそのまま返す。
Node *primary() {
  if (consume("(")) {
    // '(' が来たということは、その次に式が来るはずなので、再びexpr()を呼び出して部分木を構築する
    Node *node = expr();
    // 式が消費できなくなった時点でexpr()はreturnされる。
    // その直後に')'が来ていれば式の終わりなので、トークンを進めて、ここまで括弧内の式で構築した部分木を返す。
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
  
    LVar *lvar = find_lvar(tok);
    if (lvar) {
      node->offset = lvar->offset;
    } else {
      lvar = calloc(1, sizeof(LVar));
      lvar->next = locals;
      lvar->name = tok->str;
      lvar->len = tok->len;
      lvar->offset = 8;
      node->offset = lvar->offset;
      locals = lvar;
    }
    return node;
}

  // 現在のトークンが数値の場合はここまで降りてきて、そのまま数値のノードを作って返す
  // (さらにexpect_number によってトークンが進む)
  return new_num(expect_number());
}
