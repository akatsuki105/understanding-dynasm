# 処理の流れ

## parseargs

`dynasm.lua`を実行するとまず`parseargs`が呼び出されます。

```lua
local function parseargs(args)
  -- ...

  translate(args[args.argn], g_opt.outfile)
end
```

## translate

翻訳処理の本体は`translate`で行われます。

```lua
local function translate(infile, outfile)
  g_wbuffer = {}
  g_indent = ""
  g_lineno = 0
  g_synclineno = -1

  -- ヘッダを書き込む
  wline(dasmhead)

  -- 入力ファイルを読み込む
  -- このときにプリプロセスを行う
  local fin
  if infile == "-" then
    g_fname = "(stdin)"
    fin = stdin
  else
    g_fname = infile
    fin = assert(io.open(infile, "r"))
  end
  readfile(fin)

  -- 処理終了後、エラーがあるかどうかチェック
  if not g_arch then wprinterr(g_fname, ":*: error: missing .arch directive\n") end
  checkconds()
  checkmacros()
  checkcaptures()

  if g_errcount ~= 0 then
    stderr:write(g_fname, ":*: info: ", g_errcount, " error",
      (type(g_errcount) == "number" and g_errcount > 1) and "s" or "",
      " in input file -- no output file generated.\n")
    dumpdef(stderr)
    exit(1)
  end

  -- 出力ファイルを書き出す
  writefile(outfile)
end
```

## readfile

`readfile`では入力ファイルの各行ごとに`doline`が呼び出されており、ここでプリプロセスが行われていることがわかります。

```lua
readfile = function(fin)
  g_indent = ""
  g_lineno = 0
  g_synclineno = -1

  -- 全部の行を1行ずつ処理していく
  for line in fin:lines() do
    g_lineno = g_lineno + 1
    g_curline = line
    local ok, err = pcall(doline, line)
    if not ok and wprinterr(err, "\n") then return true end
  end
  wflush()

  -- 入力ファイルを閉じる
  assert(fin == stdin or fin:close())
end
```

## doline

ここから翻訳処理が本格的に始まります。

```lua
local function doline(line)
  if g_opt.flushline then wflush() end

  -- アセンブリを記述している行かどうか(パイプマークで判定)
  -- indent: インデントを表すスペース文字列: "    "
  -- aline: アセンブリ文字列 "| mov eax, foo+17"
  local indent, aline = match(line, "^(%s*)%|(.*)$")
  if not aline then
    -- 普通のCコードの場合はそのまま書き出して終了
    wflush()
    wsync()
    wline(line, false)
    return
  end

  g_indent = indent -- インデントを記憶しておく

  -- Emit C code (even from macros). Avoids echo and line parsing.
  -- アセンブリをプリプロセスして、Cのコードにして書き出す
  if sub(aline, 1, 1) == "|" then
    if not mac_capture then
      wsync()
    elseif g_opt.comment then
      wsync()
      wcomment(aline)
    end
    dostmt(aline) -- ここでプリプロセスを実際に行う
    return
  end

  -- プリプロセスする前のアセンブリをコメントとして出力する
  -- 例: `//| xor eax, eax`
  if g_opt.comment then
    wsync()
    wcomment(aline)
  end

  -- アセンブリに書いてあるコメントは消す
  aline = gsub(aline, "//.*$", "")

  -- 行がセミコロンで区切られた複数の文から成り立っている場合はループで文をすべて処理する。  
  -- 例: `mov R1, S.value;  mov R2, S.value.na[1];  mov R3, S.tt`
  if match(aline, ";") then
    for stmt in gmatch(aline, "[^;]+") do dostmt(stmt) end
  else
    dostmt(aline)
  end
end
```

## dostmt

```lua
dostmt = function(stmt)
  -- 空行は無視
  if match(stmt, "^%s*$") then return end

  -- Capture macro defs before substitution.
  if mac_capture then return mac_capture(stmt) end

  -- .defineによる置換を適用
  stmt = definesubst(stmt)

  -- アセンブリ文のときにdasm_putなどの前に元のアセンブリを書き込む処理
  if sub(stmt, 1, 1) == "|" then -- e.g. `| add aPtr, TAPE_SIZE`
    local tail = sub(stmt, 2) -- e.g. `add aPtr, TAPE_SIZE`
    wflush()
    if sub(tail, 1, 2) == "//" then -- e.g. `// comment` (`元のstmtが | // comment`)
      wcomment(tail)
    else
      wline(tail, true)
    end
    return
  end

  -- オペコードとオペランドを分ける
  local op, params = splitstmt(stmt)

  -- Get opcode handler (matching # of parameters or generic handler).
  -- オペコードハンドラをハンドラテーブルから取り出す
  local f = map_op[op.."_"..#params] or map_op[op.."_*"]

  -- 見つからない場合はエラー
  if not f then
    if not g_arch then wfatal("first statement must be .arch") end
    -- Improve error report.
    for i=0,9 do
      if map_op[op.."_"..i] then werror("wrong number of parameters for `"..op.."'") end
    end
    werror("unknown statement `"..op.."'")
  end

  -- ハンドラを呼び出す
  -- テンプレート文字列の場合は特殊なハンドラを呼び出す
  if type(f) == "string" then
    map_op[".template__"](params, f)
  else
    f(params)
  end
end
```