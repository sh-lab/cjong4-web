# cjong4-web

`cjong4` と `cjong4-opponent` をWebAssembly化し、ブラウザで4人打ち麻雀と
視覚的デバッグを行うためのプロジェクトです。

現在は、Emscriptenビルド、ブラウザ向けC API、全項目を編集できる天鳳ルールの
初期設定、opponent一覧、4人分の縦型画面骨格まで実装しています。対局進行は次段階です。

## 準備

サブモジュールを初期化します。

```sh
git submodule update --init --recursive
```

emsdkを有効化してからビルドします。

```sh
. /path/to/emsdk/emsdk_env.sh
```

または、ビルド時に `EMSDK_DIR` 環境変数で指定できます。

## Wasmビルド

```sh
./scripts/build-wasm.sh
```

```sh
EMSDK_DIR=/path/to/emsdk ./scripts/build-wasm.sh
```

ビルド後、リポジトリのルートでローカルHTTPサーバーを起動します。

```sh
python3 -m http.server 8000 --directory build-wasm/site
```

ブラウザで `http://localhost:8000/` を開きます。`file://` ではWasmを読み込めません。

## ネイティブテスト

```sh
cmake -S . -B build -DCJ4_WEB_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## ライセンス

MIT License。Web部分と牌画像は本プロジェクトのオリジナル実装です。
