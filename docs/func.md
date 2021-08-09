# 🧪 関数API

DynASMのAPIは10個の関数で構成されており、これらは特定の順序で使用する必要があります。

Phase | Callable Functions
-- | -- 
1 | `dasm_init`
2 | `dasm_setupglobal`
3 | `dasm_setup`
4 | `dasm_checkstep`,`dasm_growpc`,`dasm_put`のどれかをそれぞれ0回以上
5 | `dasm_link`
6 | `dasm_encode`
7 | `dasm_getpclabel`を0回以上
8 | `dasm_free`

## dasm_checkstep

```cpp
DASM_FDEF int dasm_checkstep(Dst_DECL, int secmatch);
// ↓
extern int dasm_checkstep(dasm_State **Dst, int secmatch);
```

この関数は`dasm_put`を呼び出し終えた後に、エラーチェックを行うチェックポイントを示すために呼び出すものです。

## dasm_encode

```cpp
DASM_FDEF int dasm_encode(Dst_DECL, void *buffer);
// ↓
extern int dasm_encode(dasm_State **Dst, void *buffer);
```

この関数は`dasm_link`を呼び出し終えた後に、実際のマシンコードを生成するための関数です。

## dasm_free

```cpp
DASM_FDEF void dasm_free(Dst_DECL);
// ↓
extern void dasm_free(dasm_State **Dst);
```

この関数は、`dasm_init`によって事前に確保されたDynASMのステートを解放するためのもので、処理の最後に呼び出されることを想定しています。

ユーザーは`DASM_M_FREE`マクロを再定義することでこの関数の挙動を変更することができます。

## dasm_getpclabel

省略

## dasm_growpc

```cpp
DASM_FDEF void dasm_growpc(Dst_DECL, unsigned int maxpc);
// ↓
extern void dasm_growpc(dasm_State **Dst, unsigned int maxpc);
```

この関数は、`dasm_setup`と`dasm_link`の間に呼び出され、`=>pc`シンタックスを利用可能なラベルの数を増やすために使われます。

特に、この関数を読んだ後で`=>(maxpc-1)`を通して`=>0`というラベルが利用可能になります。

## dasm_init

```cpp
DASM_FDEF void dasm_init(Dst_DECL, int maxsection);
// ↓
extern void dasm_init(dasm_State **Dst, int maxsection);
```

この関数はDynASMステートを初期化する役割を持っており、最初に呼び出すべき関数です。

ユーザーは`DASM_M_GROW`マクロを再定義することでこの関数の挙動を変更することができます。

もし、`.section`ディレクティブが使われているなら、`DASM_MAXSECTION`を`maxsection`としてこの関数に渡すべきです。そうしなかった場合、`1`として扱われます。

この関数の呼び出しに続いて、`dasm_setupglobal`を呼び出して初期化の第2段階を実行し、その後どこかで`dasm_free`を呼び出してDynASMのステートを解放する必要があります。

## dasm_link

```cpp
DASM_FDEF int dasm_link(Dst_DECL, size_t *szp);
// ↓
extern int dasm_link(dasm_State **Dst, size_t *szp);
```

この関数は、`dasm_encode`の前に呼び出され、生成されるマシンコードのサイズを計算します。

通常、このAPIを呼び出す側は`dasm_link`を呼び出し、適切なサイズのメモリを割り当ててから、`dasm_encode`を呼び出します。

## dasm_put

```cpp
DASM_FDEF void dasm_put(Dst_DECL, int start, ...);
// ↓
extern void dasm_put(dasm_State **Dst, int start, ...);
```

DynASMソースファイル内の縦棒1本(`|`)で始まる行は、dynasm.luaプリプロセッサによって `dasm_put`の呼び出しに変換されます。

概念的には、この関数の呼び出しは、DynASMのステートにいくつかのアセンブリコードを追加しており、`dasm_link`/`dasm_encode` が後でマシンコードに変換します。

この関数は`dynasm.lua`プリプロセッサが置換した後のコードで自動的に呼び出すもので人間が書いてはいけません。 

とはいえ、`start`パラメーターは`dasm_setup`に渡されるアクションリストのオフセットを与え、残りの可変長のパラメーターはオリジナルのアセンブリコードでエンコーディング時の定数の具体的な値を渡すために使用されます。

## dasm_setup

```cpp
DASM_FDEF void dasm_setup(Dst_DECL, const void *actionlist);
// ↓
extern void dasm_setup(dasm_State **Dst, const void *actionlist);
```

この関数は、`dasm_setupglobal`の後に初期化処理を完了するために呼び出されるべき関数です。

引数`actionlist`では`.actionlist`で定義された配列を渡す必要があります。この配列は、以降の`dasm_put`の呼び出しで使用されます。

## dasm_setupglobal

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
