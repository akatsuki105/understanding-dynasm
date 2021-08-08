#include <stdio.h>
#include <stdlib.h>
#include "luajit-2.0/dynasm/dasm_proto.h"
#include "luajit-2.0/dynasm/dasm_x86.h"
#include <sys/mman.h>

static void* link_and_encode(dasm_State** d) {
  size_t sz;
  void* buf;
  dasm_link(d, &sz);
  buf = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  dasm_encode(d, buf);
  mprotect(buf, sz, PROT_READ | PROT_EXEC);
  return buf;
}

#define TAPE_SIZE 30000
#define MAX_NESTING 100

typedef struct bf_state {
  unsigned char* tape;
  unsigned char (*get_ch)(struct bf_state*);
  void (*put_ch)(struct bf_state*, unsigned char);
} bf_state_t;

#define bad_program(s) exit(fprintf(stderr, "bad program near %.16s: %s\n", program, s))

// func bf_compile(program []byte) (bf_main func(*bf_state_t)) {}
static void (* bf_compile(const char* program) )(bf_state_t*) {
  unsigned loops[MAX_NESTING];
  int nloops = 0;
  int n;

  dasm_State* d;  // DynASMステート
  unsigned npc = 8;
  unsigned nextpc = 0;

  // アーキテクチャの指定(今回はx64)
  |.arch x64

  // dasm_Stateに実際にメモリを割り当てる(初期化処理)
  // 今回はcodeセクションのみなのでdasm_initの第2引数は1
  |.section code
  dasm_init(&d, DASM_MAXSECTION);

  // dasm_Stateの初期化処理の続き
  // おまじない？
  |.globals lbl_
  void* labels[lbl__MAX];
  dasm_setupglobal(&d, labels, lbl__MAX);

  // dasm_Stateの初期化処理の続き
  // おまじない？
  |.actionlist bf_actions
  dasm_setup(&d, bf_actions);

  dasm_growpc(&d, npc);

  // マクロによって可読性をあげておく
  |.define aPtr, rbx        // テープの現在位置
  |.define aState, r12      // bfランタイムのステート
  |.define aTapeBegin, r13
  |.define aTapeEnd, r14
  |.define rArg1, rdi
  |.define rArg2, rsi

  // 引数を1つとる関数の呼び出し準備
  |.macro prepcall1, arg1
    | mov rArg1, arg1
  |.endmacro

  // 引数を2つとる関数の呼び出し準備
  |.macro prepcall2, arg1, arg2
    | mov rArg1, arg1
    | mov rArg2, arg2
  |.endmacro

  // 
  |.define postcall, .nop

  // スタックフレームの初期化
  // 引数はaStateに格納される
  |.macro prologue
    | push aPtr
    | push aState
    | push aTapeBegin
    | push aTapeEnd
    | push rax
    | mov aState, rArg1
  |.endmacro

  // スタックフレームの解体
  |.macro epilogue
    | pop rax
    | pop aTapeEnd
    | pop aTapeBegin
    | pop aState
    | pop aPtr
    | ret
  |.endmacro

  // このディレクティブのおかげで
  // [aState + offsetof(bf_state_t,tape)] を 
  // state->tape とかける
  |.type state, bf_state_t, aState
  
  dasm_State** Dst = &d;

  |.code

  // グローバルラベル`->bf_main`を定義します。マシンコードの出力が終了したら、このグローバルラベルのアドレスを取得して、関数ポインタにします。
  |->bf_main:
  | prologue

  // マシンコード
  | mov aPtr, state->tape
  | lea aTapeBegin, [aPtr-1]
  | lea aTapeEnd, [aPtr+TAPE_SIZE-1]
  for (;;) {
    switch (*program++) {
    case '<': // ポインタのデクリメント
      for (n = 1; *program == '<'; ++n, ++program);

      // if(!nskip) {
      //  ptr -= n;
      //  while (ptr <= tape_begin) ptr += TAPE_SIZE;
      // }
      | sub aPtr, n%TAPE_SIZE
      | cmp aPtr, aTapeBegin
      | ja >1
      | add aPtr, TAPE_SIZE
      |1:

      break;

    case '>': // ポインタのインクリメント
      for (n = 1; *program == '>'; ++n, ++program);
      | add aPtr, n%TAPE_SIZE
      | cmp aPtr, aTapeEnd
      | jbe >1
      | sub aPtr, TAPE_SIZE
      |1:
      break;

    case '+': // ポインタの指す値をインクリメント
      for (n = 1; *program == '+'; ++n, ++program);
      | add byte [aPtr], n
      break;

    case '-': // ポインタの指す値をデクリメント
      for (n = 1; *program == '-'; ++n, ++program);
      | sub byte [aPtr], n
      break;

    case ',': // 入力から1バイト読み込んで、ポインタが指す値に代入
      | prepcall1 aState
      | call aword state->get_ch
      | postcall 1
      | mov byte [aPtr], al
      break;

    case '.': // ポインタの値を出力
      | movzx r0, byte [aPtr]
      | prepcall2 aState, r0
      | call aword state->put_ch
      | postcall 2
      break;

    case '[': // ポインタの指す値が0なら、後の]までジャンプ(要するにwhile)
      if (nloops == MAX_NESTING) {
        bad_program("Nesting too deep");
      }
      if (program[0] == '-' && program[1] == ']') {
        program += 2;
        | xor eax, eax
        | mov byte [aPtr], al
      } else {
        if (nextpc == npc) {
          npc *= 2;
          dasm_growpc(&d, npc);
        }
        | cmp byte [aPtr], 0
        | jz =>nextpc+1
        |=>nextpc:
        loops[nloops++] = nextpc;
        nextpc += 2;
      }
      break;

    case ']': // ポインタの指す値が0でなければ、前の[までジャンプ
      if (nloops == 0) {
        bad_program("] without matching [");
      }
      --nloops;
      | cmp byte [aPtr], 0
      | jnz =>loops[nloops]
      |=>loops[nloops]+1:
      break;

    case 0: // ファイル終端に到達したら終了
      if (nloops != 0) {
        program = "<EOF>", bad_program("[ without matching ]");
      }
      | epilogue
      link_and_encode(&d);
      dasm_free(&d);

      // bf_mainのアドレスを関数ポインタ`void(*)(bf_state_t*)`にキャストして返す
      return (void(*)(bf_state_t*))labels[lbl_bf_main];
    }
  }
}

static void bf_putchar(bf_state_t* s, unsigned char c) {
  putchar((int)c);
}

static unsigned char bf_getchar(bf_state_t* s) {
  return (unsigned char)getchar();
}

/**
 * brainfxxkを実行する
 * 
 * @param[in] (program) bfファイルのバイト列
 */
static void bf_run(const char* program) {
  void(*)(bf_state_t*) bf_main;
  bf_state_t state;
  unsigned char tape[TAPE_SIZE] = {0};
  state.tape = tape;
  state.get_ch = bf_getchar;
  state.put_ch = bf_putchar;

  bf_main = bf_compile(program);
  bf_main(&state);
}

int main(int argc, char** argv) {
  if (argc == 2) {
    long sz;
    char* program;
    FILE* f = fopen(argv[1], "r");
    if (!f) {
      fprintf(stderr, "Cannot open %s\n", argv[1]);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    program = (char*)malloc(sz + 1);
    fseek(f, 0, SEEK_SET);
    program[fread(program, 1, sz, f)] = 0;
    fclose(f);
    bf_run(program);
    return 0;
  } else {
    fprintf(stderr, "Usage: %s INFILE.bf\n", argv[0]);
    return 1;
  }
}
