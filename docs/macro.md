# 🔎 マクロ

DynASMの動作を変更するために、コンパイル時にいくつかのマクロを定義することができます。

特定のマクロのデフォルトの定義が適切でない場合は、自分のCのコードからDynASMのヘッダをインクルードする前にこれらのマクロを再定義する必要があります。

## DASM_ALIGNED_WRITES

省略

## DASM_CHECKS

省略

## DASM_EXTERN

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

## DASM_FDEF

```cpp
#define DASM_FDEF extern

DASM_FDEF void dasm_init(Dst_DECL, int maxsection);
// ↓
extern void dasm_init(Dst_DECL, int maxsection);
```

このユーザー定義マクロは、すべての`dasm_`関数のアクセサを制御するために使用されます。

## DASM_M_FREE

省略

## DASM_M_GROW

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

## Dst_DECL

```cpp
#define Dst_DECL dasm_State **Dst

DASM_FDEF void dasm_init(Dst_DECL, int maxsection);
// ↓
DASM_FDEF void dasm_init(dasm_State **Dst, int maxsection); // Dst_DECL == シグネチャ
```

このユーザー定義マクロは、すべての`dasm_`関数のシグネチャについて、第一引数の型と名前を定義するためのマクロです。

上記のパラメータの名前`Dst`は、`Dst`のままにしておくか、ここで使われている名前に合わせて`Dst`を再定義するプリプロセッサマクロが必要です。

上記のパラメータの型`dasm_State`は、`Dst_REF`に渡せるならばなんでも大丈夫です。

## Dst_REF

```cpp
#define Dst_REF (*Dst)
```

このユーザー定義マクロは、`dasm_x86.h`によって、変数`Dst`から`dasm_State*`型のインターフェース値を取り出したい時に使用されます。

`Dst_DECL`を再定義した場合は、このマクロも再定義する必要があります。