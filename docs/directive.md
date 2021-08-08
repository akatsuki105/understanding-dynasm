# ディレクティブ

## .actionlist

```cpp
| .actionlist ident
```

このディレクティブを使うと、`dynasm.lua`のプリプロセッサが以下のようなC言語のコードを出力します。

```cpp
static const unsigned char ident[] = {
  /* ... */
};
```

このディレクティブは、DynASMソースファイル内で一度だけ使用する必要があり、定義された配列は`dasm_setup`に引数として渡されなければなりません。

配列の内容は、DynASMソースファイルで使用されているアセンブリを反映していますが、そのフォーマットは不定です。

## .globals

```cpp
| .globals prefix
```

このディレクティブを使うと、`dynasm.lua`のプリプロセッサが以下のようなC言語のコードを出力します。

```cpp
enum {
  /* ... */
  prefix_MAX
};
```

## .section

```cpp
| .section name1 [, name2 [, ...]]
```

このディレクティブは、DynASMのソースファイルが単一のコードセグメントではなく、複数のコードセグメントを記述できるようにするものです。

引数`name`ごとに新しいディレクティブ`.name`が導入され、導入されたディレクティブの1つを使用すると、それに続くすべてのアセンブリとデータディレクティブが、指定されたセクションに追加されます。

`dasm_encode`が呼び出されると、セクションは連結され、マシンコードの単一の連続したブロックを形成します。

```cpp
|.section code

| .code // 以降はcodeセクションに追加される
```

## .type

```cpp
| .type name, ctype [, default_reg]
```

このディレクティブは、DynASMのソースファイルの残りの部分に以下のようなシンタックスシュガーを定義することで、`ctype*`型のレジスタの操作を容易にしています。


Sugar | Expansion
-- | --
`#name` | `sizeof(ctype)`
`name:reg->field` | `[reg + offsetof(ctype,field)]`
`name:reg[imm32]` | `[reg + sizeof(ctype)*imm32]`
`name:reg[imm32].field` | `[reg + sizeof(ctype)*imm32 + offsetof(ctype,field)]`
`name:reg...` | `[reg + (int)(ptrdiff_t)&(((ctype*)0)...)]`
