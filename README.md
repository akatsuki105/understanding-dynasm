# understanding-dynasm

[LuaJIT](https://luajit.org/)のDynASMを読んでコメントつけたり理解したことを書きつけたりするための自分用レポジトリです。

ちなみに筆者にLua歴はないです。

## docs/

読んで理解したことはここにまとめてあります。

## example/

`DynASM`で処理する前のコード(`mandelbrot.c`)と処理した後のコード(`mandelbrot_x64.c`)を例として置いています。

これはbranfxxkを実行するJIT処理系のコードです。詳細は[こちら](https://corsix.github.io/dynasm-doc/tutorial.html)

## Credits

- https://luajit.org/
- https://corsix.github.io/dynasm-doc/index.html
- https://qiita.com/TomoShiozawa/items/25dcce1540085df71053
