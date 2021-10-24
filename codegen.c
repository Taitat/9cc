#include "9cc.h"
//
// Code generator
//


void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error("代入の左辺値が変数ではありません");
  printf("  mov rax, rbp\n");
  printf("  sub rax %d\n", node->offset);
}

// pushやpopはx86-64のRSPレジスタを暗黙的にスタックポインタとして使用し、
// その値を変更しつつ、RSPが指しているメモリにアクセスする
void gen(Node *node) {
  switch (node->kind){
  // 数値が来たときはそのままスタックマシンへpush
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_LVAR:
      gen_lval(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen(node->rhs);

      printf("  pop rdi\n");
      printf("  pop rax\n");
      printf("  mov [rax], rdi\n");
      printf("  push rdi\n");

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
  case ND_EQ:
    printf("  cmp rax, rdi\n");  // スタックトップからポップしてきたレジスタの値二つを比較する
    printf("  sete al\n");       // 上記のcmp命令で比較した結果が同じだった場合は、ALレジスタに1をセットする。そうでなければ0。
    // ALはRAXの下位8ビットのこと。ALにセットするということはRAXを変更することになるが、
    // 残りの上位56ビットは変更前の値のままになるので、ここをゼロクリアする必要がある。それを行うのがmovzx命令。
    // （オリジナルだとmovzbだがエラーになるので、movzxを使用する）
    printf("  movzx rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzx rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzx rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzx rax, al\n");
    break;
  }

  printf("  push rax\n");

}