# cjong4-web

`cjong4` と `cjong4-opponent` をWebAssembly化し、ブラウザで4人打ち麻雀と
視覚的デバッグを行うためのプロジェクトです。

Emscriptenビルド、ブラウザ向けC API、編集可能な対局ルール、4席の人間／opponent
選択、実対局の進行、合法手入力、局終了時の清算結果、再生・一時停止・次の停止地点までの進行、履歴の復元に対応しています。
対局状態はWasm側で保持し、ブラウザへバージョン付きJSONスナップショットを渡します。
各席の操作は対局中でも人間／opponentを切り替えられます。

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

## 対局操作

1. 各席の操作を人間またはopponentから選びます。初期値は全席 `betaori` で、人間以外の手牌は非表示です。
2. 必要なら対局設定と山の設定を変更します。
3. 「対局開始」を押します。
4. 「次へ進める」または「再生」で進行します。人間の手番では合法手だけを選べます。
5. 局終了時は、和了役、符・翻、支払点、本場・供託を含む点数移動を確認できます。
6. 対局履歴のスライダーで任意の状態へ戻れます。過去から進行すると後続履歴は破棄されます。

山の「ランダム」は開始時のseedから再現可能なシャッフルを行います。「プリセット
（デバッグ用）」は構成決定までの仮実装として物理牌ID順の固定山を使用します。

ブラウザ向けAPI version 3では、設定、対局開始、操作切り替え、1状態遷移、人間の合法手選択、
履歴復元、UI snapshot JSONを公開しています。状態JSON schema version 3には局終了時の和了結果と
清算後点数を含みます。返却JSON文字列はライブラリ所有で、次回呼び出しまで有効です。

## ネイティブテスト

```sh
cmake -S . -B build -DCJ4_WEB_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## ライセンス

MIT License。Web部分と牌画像は本プロジェクトのオリジナル実装です。
