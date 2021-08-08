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

### Dst_DECL

```cpp
#ifndef Dst_DECL
#define Dst_DECL dasm_State **Dst
#endif
```

このユーザー定義マクロは、すべての`dasm_`関数のシグネチャに影響を与え、特に最初のフォーマルパラメータの型と名前を与えます。

## 🧪 関数API

### dasm_checkstep

```cpp
DASM_FDEF int dasm_checkstep(Dst_DECL, int secmatch);
```
