# アクション

x64のアクションを例として解説

## x64の命令のおさらい

```
+-------------+------------+----------+---------+--------------+-------------+
| Instruction |   Opcode   |  ModR/M  |   SIB   | Displacement |  Immediate  |
| Prefixes    |            |          |         |              |             |
+-------------+------------+----------+---------+--------------+-------------+
 Prefixes of   1-, 2- or    1 byte     1 byte    Address        Immediate
 1 byte each   3-byte       (if        (if       displacement   data of 1,
 (optional)    opcode       required)  required) of 1, 2, or 4  2, or 4
                                                 bytes or none  bytes or none
```

## 概要

```lua
-- dasm_x86.lua
local action_names = {
  -- int arg, 1 buffer pos:
  "DISP",  "IMM_S", "IMM_B", "IMM_W", "IMM_D",  "IMM_WB", "IMM_DB",

  -- action arg (1 byte), int arg, 1 buffer pos (reg/num):
  "VREG", "SPACE", -- !x64: VREG support NYI.
  
  -- ptrdiff_t arg, 1 buffer pos (address): !x64
  "SETLABEL", "REL_A",
  
  -- action arg (1 byte) or int arg, 2 buffer pos (link, offset):
  "REL_LG", "REL_PC",
  
  -- action arg (1 byte) or int arg, 1 buffer pos (link):
  "IMM_LG", "IMM_PC",

  -- action arg (1 byte) or int arg, 1 buffer pos (offset):
  "LABEL_LG", "LABEL_PC",
  
  -- action arg (1 byte), 1 buffer pos (offset):
  "ALIGN",

  -- action args (2 bytes), no buffer pos.
  "EXTERN",
  
  -- action arg (1 byte), no buffer pos.
  "ESC",
  
  -- no action arg, no buffer pos.
  "MARK",
  
  -- action arg (1 byte), no buffer pos, terminal action:
  "SECTION",
  
  -- no args, no buffer pos, terminal action:
  "STOP"
}
```

```cpp
// dasm_x86.h
enum {
  DASM_DISP = 233, // オフセット [ebp+xx]のxxの部分
  DASM_IMM_S = 234,
  DASM_IMM_B = 235,
  DASM_IMM_W = 236,
  DASM_IMM_D = 237,
  DASM_IMM_WB = 238,
  DASM_IMM_DB = 239,
  DASM_VREG = 240,
  DASM_SPACE = 241,
  DASM_SETLABEL = 242,
  DASM_REL_A = 243,
  DASM_REL_LG = 244,
  DASM_REL_PC = 245,
  DASM_IMM_LG = 246,
  DASM_IMM_PC = 247,
  DASM_LABEL_LG = 248,
  DASM_LABEL_PC = 249,
  DASM_ALIGN = 250,
  DASM_EXTERN = 251,
  DASM_ESC = 252,
  DASM_MARK = 253,
  DASM_SECTION = 254,
  DASM_STOP = 255
};
```

## 各アクションについて

`b`は1バイトです。

`w`は2バイトです。

`n`は整数で環境依存です。

### DISP

```sh
actinlist: 233, n
```

TODO

### IMM_S, IMM_B

```sh
actinlist: 234, n # IMM_Bの場合は235
buffer: b
```

マシンコードを入れるバッファに整数の下位1バイトを追加します。

### IMM_WB

```sh
actionlist: 238, n
buffer: b または w
```

下の`IMM_DB`のword版です。

### IMM_W

```sh
actinlist: 236, n
buffer: w
```

マシンコードを入れるバッファに整数の下位2バイトを追加します。

### IMM_DB

```sh
actionlist: 239, n
buffer: b または d
```

オペコードが符号あり8bitの範囲で表現できる場合は

- バッファの前のオペコードに+2を加えて符号あり8bit用にオペコードを改変
- バイトをバッファに追加

します。

8bitじゃ表現し切れない場合は`IMM_D`と同様に4バイト整数として格納します。

### IMM_D

```sh
actinlist: 237, n
buffer: n
```

マシンコードを入れるバッファに整数を追加します。

### VREG

```sh
actionlist: 240, n0, n1
buffer: なし
```

すでにバッファに書き込まれた命令のModR/M(コード上では`cp[-1]`と表される)にレジスタインデックスをセットします。

n1は`0~3`のどれかです。

n1が`2, 3`のときは`REG`(bit3-5)、`0, 1`のときは`R/M`(bit0-2)に対してレジスタインデックス`n0`をセットします。


### SPACE

```sh
actionlist: 241, n0, n1
buffer: n1, n1, n1, ... # n0の数だけ続く
```

bufferを`n0`個の整数`n1`で埋めます。

### SETLABEL

```sh
actionlist: 242
buffer: なし
```

TODO

### ALIGN

```sh
actionlist: 250, b # bはアラインメントの単位を表す
buffer: 不定
```

マシンコードを入れるバッファをアラインメントするためのアクションです。

例えばアクションの引数に4を渡した場合、マシンコードのバッファは4の倍数のアドレスにくるまで`nop`命令で埋められます。

### EXTERN

```sh
actionlist: 251, b, b
buffer: 0, 0, 0, 0
```

ユーザーが`DASM_EXTERN`を再定義しない限りは`buffer`には0(4byte)が入ります

### ESC

```sh
actionlist: 252, b
buffer: b
```

おそらく、actionlistで使っている233から255までのバイトをマシンコードとして出力したい時にエスケープするためのアクションです。

### MARK

```sh
actionlist: 253
buffer: なし
```

内部的に使われるものです。

現在マシンコードを詰めているバッファの位置を記録します。記録したバッファの位置は`DISP`や`IMM_DB`などのアクションを処理する際に利用されます。

### SECTION

```sh
actionlist: 254, b
```

### STOP

```sh
actionlist: 255
buffer: なし
```
