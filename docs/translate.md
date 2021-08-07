# DynASM

この記事は、[公式ドキュメント](https://luajit.org/dynasm.html)から部分的に抜粋して翻訳したものです。

```
! この記事では、アセンブリを機械語のニーモニックとして、アセンブラをアセンブリを機械語にコンパイルするツールという意味で扱います。
```

## 特徴

### ツールチェーンについて

- DynASM はプリプロセッサアセンブラです。
- DynASM はCとアセンブリの混ざったコードをCのみのコードにプリプロセスします。
- 命令名、オペランドモード、レジスタ、オペコード、そしてそれらをどのようにエンコードするかといった主要な知識は、プリプロセッサでのみ必要となります。
- 生成されるCのコードは非常に小さいサイズかつ高速に動作します。
- DynASMが生成するCコードに入っている組込み(環境でも実行)可能な小さなCライブラリは、マシンコードを動的にアセンブル、再配置、リンクするプロセスを支援します。
- アセンブラやリンカなどの他のツールには依存していません。
- 未定義のラベルなどがあった場合はランタイムエラーとなるので安全です。
- ツールチェーンは、クロスプラットフォームなサブセットとCPU固有のモジュールに分かれています。
- DynASM自体はLuaで書かれています。
- Lua5.1と[`Lua BitOp`](https://bitop.luajit.org/)が動く環境ならどこでも動作します。

### DynASMアセンブリについて

- Cとアセンブリを組み合わせてコードを書くことができ、可読性に優れています。
- 標準的なアセンブリに求められる、命令やオペランドモードの構文をすべて備えています。
- アセンブリからCの変数やC++の定義にアクセス可能です。
- `type mapping`を通じてCの構造体や共用体にアクセス可能です。
- C構造体へのアクセスを簡単にするシンタックスシュガーも用意しています。
- Numbered labels (e.g. for mapping bytecode instruction numbers).
- コードセクションを複数定義可能(e.g. for tailcode)
- Defines/substitutions (inline and from command line).
- Conditionals (translation time) with proper nesting.
- 引数を伴ったマクロが定義可能
- マクロではCとアセンブリの両方を使用可能
- Captures (output diversion for code reordering).
- 独自の命令定義のためのシンプルで拡張可能なテンプレートシステム

### 制約

現在、x86、x64、ARM、ARM64、PowerPC、MIPSの各命令セットをサポートしています。これには、最新のCPUで利用可能なほとんどのユーザーモード命令が含まれます。

x86/x64では、SSE、SSE2、SSE3、SSE4a、SSE4.1、SSE4.2、AVX、AVX2、BMI、ADX、AES-NI、FMA3が含まれます。PPCでは、e500命令セット拡張も含まれています。

ツールチェーン全体が、複数のCPUアーキテクチャをサポートするように設計されています。LuaJITがより多くのアーキテクチャをサポートするようになると、DynASMは新しいCPU固有のモジュールで拡張されます。

Note that runtime conditionals are not really needed, since you can just use plain C code for that (and LuaJIT does this a lot). It's not going to be more (time-) efficient if conditionals are done by the embedded C library (maybe a bit more space-efficient).

## コードによる説明

はじめに、プリプロセスを行うための簡単なコードスニペットを紹介します。

`|`（パイプのマーク）で始まる行はDynASM用です。

```c
 if (ptr != NULL) {
    |  mov eax, foo+17
    |  mov edx, [eax+esi*2+0x20]
    |  add ebx, [ecx+bar(ptr, 9)]
 }
```

DynASMでプリプロセスをした後のコードは次のようになります。

```c
  if (ptr != NULL) {
    dasm_put(Dst, 123, foo+17, bar(ptr, 9));
  }
```

ここで123は、部分的に指定されたマシンコードを保持するアクションリストバッファへのオフセットです。

詳細は省きますが、組み込みCライブラリは、アクションリストを入力とし、マシンコードを出力する小さなバイトコードエンジンを実装しています。

基本的には、アクションリストから機械コードのスニペットをコピーして、`dasm_put`に渡された引数とマージします。

引数には、あらゆる種類のC言語の式を指定できます。実際には、ほとんどが定数（構造体のオフセットなど）に評価されます。Cコンパイラはこの式から非常にコンパクトなコードを生成するはずです。

組み込みCライブラリは、ターゲットCPU用の適切なマシンコードを生成するために（例えば、可変変位サイズ、可変分岐オフセットサイズなど）絶対に必要なものだけを知っています。

x86のオペコードエンコーディングのような他の残虐行為については何も知りませんし、その必要もありません。これにより、必要な最小コードサイズは約2Kと劇的に小さくなります。

アクションリストバッファ自体も非常にコンパクトなエンコーディングになっています。例えば、LuaJITの初期バージョンでは、アクションリストバッファ全体で約3Kしか必要ありませんでした。

### 踏み込んでみよう

型マップ、マクロ、C言語の構造体へのアクセス方法など、LuaJITの高度な機能を使った実例をご紹介します。

```c
|.type L,      lua_State,  esi  // L.
|.type BASE,   TValue,     ebx  // L->base.
|.type TOP,    TValue,     edi  // L->top.
|.type CI,     CallInfo,   ecx  // L->ci.
|.type LCL,    LClosure,   eax  // L->ci->func->value.
|.type UPVAL,  UpVal

|.macro copyslot, D, S, R1, R2, R3
|  mov R1, S.value;  mov R2, S.value.na[1];  mov R3, S.tt
|  mov D.value, R1;  mov D.value.na[1], R2;  mov D.tt, R3
|.endmacro

|.macro copyslot, D, S;  copyslot D, S, ecx, edx, eax; .endmacro

|.macro getLCL, reg
||if (!J->pt->is_vararg) {
|  mov LCL:reg, BASE[-1].value
||} else {
|  mov CI, L->ci
|  mov TOP, CI->func
|  mov LCL:reg, TOP->value
||}
|.endmacro

|.macro getLCL;  getLCL eax; .endmacro

[...]

static void jit_op_getupval(jit_State *J, int dest, int uvidx)
{
  |  getLCL
  |  mov UPVAL:ecx, LCL->upvals[uvidx]
  |  mov TOP, UPVAL:ecx->v
  |  copyslot BASE[dest], TOP[0]
}
```

このコードをDynASMでプリプロセスすると次のようになります。

```cpp
#define Dt1(_V) (int)&(((lua_State *)0)_V)

// [...]

static void jit_op_getupval(jit_State *J, int dest, int uvidx)
{
  if (!J->pt->is_vararg) {
    dasm_put(Dst, 1164, Dt2([-1].value));
  } else {
    dasm_put(Dst, 1168, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
  dasm_put(Dst, 1178, Dt5(->upvals[uvidx]), DtF(->v), Dt3([0].value), Dt3([0].value.na[1]), Dt3([0].tt), Dt2([dest].value), Dt2([dest].value.na[1]), Dt2([dest].tt));
}
```
