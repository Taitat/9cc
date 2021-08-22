#include "9cc.h"

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
