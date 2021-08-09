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

## MARK, STOP

