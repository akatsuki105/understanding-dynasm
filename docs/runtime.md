# ランタイム

今回はx86アーキテクチャを例として、DynASMのランタイムのコードの内容を追っていきます。

具体的には[`dasm_proto.h`](../dasm_proto.h)と[`dasm_x86.h`](../dasm_x86.h)についての解説です。

## 📜 概要

**dasm_proto.h**

[`dasm_proto.h`](../dasm_proto.h)には、様々なマクロのデフォルト定義と、様々な関数の型シグネチャが含まれており、これらをまとめてDynASMランタイムライブラリを構成しています。

DynASMを使ってマシンコードを生成したいすべてのC/C++ファイルでは、このヘッダファイルをインクルードする必要があります。

**dasm_x86.h**

[`dasm_x86.h`](../dasm_x86.h)には、x86用のDynASMランタイムライブラリの様々な関数の実装が含まれています。

## 🔎 マクロ

DynASMの動作を変更するために、コンパイル時にいくつかのマクロを定義することができます。

特定のマクロのデフォルトの定義が適切でない場合は、自分のCのコードからDynASMのヘッダをインクルードする前にこれらのマクロを再定義する必要があります。

### DASM_ALIGNED_WRITES

省略

### DASM_CHECKS

省略

### DASM_EXTERN

```cpp
#define DASM_EXTERN(ctx, addr, idx, type) 0
```

このユーザー定義マクロは、`dasm_encode`がexternアドレスをエンコードする必要があるときに、`dasm_x86.h`によって使用され、externアドレスが使用される場合は再定義する必要があります。

`ctx`パラメータは、`Dst_DECL`で定義された変数と同じ型であり、`dasm_encode`の第1引数に設定されます。

`addr`パラメータは，`unsigned char*`型で，x86/x64命令の一部として書き込まれる`int32_t`フィールドのアドレスを指定します。

`idx`パラメータは`unsigned int`型で、`.externnames`配列へのインデックスを指定し、書き込まれる特定のexternアドレスを特定しています。

例えば、extern名が動的に検索可能なシンボルであれば、`DASM_EXTERN`は次のように定義されるかもしれません。

```cpp
#define DASM_EXTERN(ctx, addr, idx, type) (\
    (type) ? (int)((unsigned char*)dlsym(RTLD_DEFAULT, externs[idx]) - (addr) - 4) : (int)dlsym(RTLD_DEFAULT, externs[idx])\
)
```

### DASM_FDEF

```cpp
#define DASM_FDEF extern

DASM_FDEF void dasm_init(Dst_DECL, int maxsection);
// ↓
extern void dasm_init(Dst_DECL, int maxsection);
```

このユーザー定義マクロは、すべての`dasm_`関数のアクセサを制御するために使用されます。

### DASM_M_FREE

省略

### DASM_M_GROW

```cpp
#define DASM_M_GROW(ctx, t, p, sz, need) \
  do { \
    size_t _sz = (sz), _need = (need); \
    if (_sz < _need) { \
      if (_sz < 16) _sz = 16; \
      while (_sz < _need) _sz += _sz; \
      (p) = (t *)realloc((p), _sz); \
      if ((p) == NULL) exit(1); \
      (sz) = _sz; \
    } \
  } while(0)
```

このユーザー定義マクロは、`dasm_x86.h`によって、メモリの割り当てや再割り当てが必要なとき、つまり`dasm_init`が常に必要であり他の様々な`dasm_`関数が時々必要なときに、使用されます。

このマクロは、デフォルトのCの`realloc/free`によるメモリ割り当てメカニズムが適切でないときに、`DASM_M_FREE`と共に再定義されるべきです。

`ctx`パラメータは、`Dst_DECL`で定義された変数と同じ型であり、割り当てを行う`dasm_`関数の第1引数に設定されます。

### Dst_DECL

```cpp
#define Dst_DECL dasm_State **Dst

DASM_FDEF void dasm_init(Dst_DECL, int maxsection);
// ↓
DASM_FDEF void dasm_init(dasm_State **Dst, int maxsection); // Dst_DECL == シグネチャ
```

このユーザー定義マクロは、すべての`dasm_`関数のシグネチャについて、第一引数の型と名前を定義するためのマクロです。

上記のパラメータの名前`Dst`は、`Dst`のままにしておくか、ここで使われている名前に合わせて`Dst`を再定義するプリプロセッサマクロが必要です。

上記のパラメータの型`dasm_State`は、`Dst_REF`に渡せるならばなんでも大丈夫です。

### Dst_REF

```cpp
#define Dst_REF (*Dst)
```

このユーザー定義マクロは、`dasm_x86.h`によって、変数`Dst`から`dasm_State*`型のインターフェース値を取り出したい時に使用されます。

`Dst_DECL`を再定義した場合は、このマクロも再定義する必要があります。

## 🧪 関数API

DynASMのAPIは10個の関数で構成されており、これらは特定の順序で使用する必要があります。

Phase | Callable Functions
1 | `dasm_init`
2 | `dasm_setupglobal`
3 | `dasm_setup`
4 | `dasm_checkstep`,`dasm_growpc`,`dasm_put`のどれかをそれぞれ0回以上
5 | `dasm_link`
6 | `dasm_encode`
7 | `dasm_getpclabel`を0回以上
8 | `dasm_free`

### dasm_checkstep

```cpp
DASM_FDEF int dasm_checkstep(Dst_DECL, int secmatch);
// ↓
extern int dasm_checkstep(dasm_State **Dst, int secmatch);
```

この関数は`dasm_put`を呼び出し終えた後に、エラーチェックを行うチェックポイントを示すために呼び出すものです。

### dasm_encode

```cpp
DASM_FDEF int dasm_encode(Dst_DECL, void *buffer);
// ↓
extern int dasm_encode(dasm_State **Dst, void *buffer);
```

この関数は`dasm_link`を呼び出し終えた後に、実際のマシンコードを生成するための関数です。

### dasm_free

```cpp
DASM_FDEF void dasm_free(Dst_DECL);
// ↓
extern void dasm_free(dasm_State **Dst);
```

この関数は、`dasm_init`によって事前に確保されたDynASMのステートを解放するためのもので、処理の最後に呼び出されることを想定しています。

ユーザーは`DASM_M_FREE`マクロを再定義することでこの関数の挙動を変更することができます。

### dasm_getpclabel

省略

### dasm_growpc

省略

### dasm_init

```cpp
DASM_FDEF void dasm_init(Dst_DECL, int maxsection);
// ↓
extern void dasm_init(dasm_State **Dst, int maxsection);
```

この関数はDynASMステートを初期化する役割を持っており、最初に呼び出すべき関数です。

ユーザーは`DASM_M_GROW`マクロを再定義することでこの関数の挙動を変更することができます。

もし、`.section`ディレクティブが使われているなら、`DASM_MAXSECTION`を`maxsection`としてこの関数に渡すべきです。そうしなかった場合、`1`として扱われます。

この関数の呼び出しに続いて、`dasm_setupglobal`を呼び出して初期化の第2段階を実行し、その後どこかで`dasm_free`を呼び出してDynASMのステートを解放する必要があります。

### dasm_link

```cpp
DASM_FDEF int dasm_link(Dst_DECL, size_t *szp);
// ↓
extern int dasm_link(dasm_State **Dst, size_t *szp);
```

この関数は、`dasm_encode`の前に呼び出され、生成されるマシンコードのサイズを計算します。

通常、このAPIを呼び出す側は`dasm_link`を呼び出し、適切なサイズのメモリを割り当ててから、`dasm_encode`を呼び出します。

### dasm_put

```cpp
DASM_FDEF void dasm_put(Dst_DECL, int start, ...);
// ↓
extern void dasm_put(dasm_State **Dst, int start, ...);
```

DynASMソースファイル内の縦棒1本(`|`)で始まる行は、dynasm.luaプリプロセッサによって `dasm_put`の呼び出しに変換されます。

概念的には、この関数の呼び出しは、DynASMのステートにいくつかのアセンブリコードを追加しており、`dasm_link`/`dasm_encode` が後でマシンコードに変換します。

この関数は`dynasm.lua`プリプロセッサが置換した後のコードで自動的に呼び出すもので人間が書いてはいけません。 

とはいえ、`start`パラメーターは`dasm_setup`に渡されるアクションリストのオフセットを与え、残りの可変長のパラメーターはオリジナルのアセンブリコードでエンコーディング時の定数の具体的な値を渡すために使用されます。

### dasm_setup

```cpp
DASM_FDEF void dasm_setup(Dst_DECL, const void *actionlist);
// ↓
extern void dasm_setup(dasm_State **Dst, const void *actionlist);
```

この関数は、`dasm_setupglobal`の後に初期化処理を完了するために呼び出されるべき関数です。

引数`actionlist`では`.actionlist`で定義された配列を渡す必要があります。この配列は、以降の`dasm_put`の呼び出しで使用されます。

### dasm_setupglobal

```cpp
DASM_FDEF void dasm_setupglobal(Dst_DECL, void **gl, unsigned int maxgl);
// ↓
extern void dasm_setupglobal(dasm_State **Dst, void **gl, unsigned int maxgl);
```

この関数は`dasm_init`の後で`dasm_setup`を呼び出す前に実行する必要があります。この関数は`dasm_init`によって引き起こされたDynASMステートの初期化処理の2段階目の処理を行うためのものです。


`.globals ident`で定義された`ident_MAX`の値は`maxgl`として渡され、`maxgl`個の要素を持つ`void *`の配列はglとして渡されます。

次のようにローカル変数を当該配列として使用するのが一般的です。

```cpp
void* globals[ident_MAX];
dasm_setupglobal(&d, globals, ident_MAX);
```

