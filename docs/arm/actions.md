# アクション

## 概要

```lua
-- dasm_arm.lua
local action_names = {
  "STOP","SECTION", "ESC", "REL_EXT",
  "ALIGN", "REL_LG", "LABEL_LG",
  "REL_PC", "LABEL_PC", "IMM", "IMM12", "IMM16", "IMML8", "IMML12", "IMMV8",
}
```

```cpp
// dasm_arm.h
enum {
  DASM_STOP = 0,
  DASM_SECTION = 1,
  DASM_ESC = 2,
  DASM_REL_EXT = 3,

  /* 次の3つのアクションはバッファのポジションを引数として必要とするアクションです */
  DASM_ALIGN = 4,
  DASM_REL_LG = 5,
  DASM_LABEL_LG = 6,
  
  /* 以降のポジションは引数を持っているアクションです */
  DASM_REL_PC = 7,
  DASM_LABEL_PC = 8,
  DASM_IMM = 9,
  DASM_IMM12 = 10,
  DASM_IMM16 = 11,
  DASM_IMML8 = 12,
  DASM_IMML12 = 13,
  DASM_IMMV8 = 14,
  DASM__MAX = 15
};
```

## 各アクションについて

`b`は1バイトです。

`w`は2バイトです。

`n`は整数で環境依存です。

### ESC

```sh
actionlist: 2, b
buffer: b
```

おそらく、actionlistで使っている0から15までのバイトをマシンコードとして出力したい時にエスケープするためのアクションです。

### ALIGN

```sh
actionlist: 250, n
buffer: 不定
```

マシンコードを入れるバッファをアラインメントするためのアクションです。

例えばアクションの引数に15を渡した場合、マシンコードのバッファは16の倍数のアドレスにくるまで`0xe1a0_0000`つまり`nop(mov r0, r0)`で埋められます。


