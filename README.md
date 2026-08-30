# cjong4-web

`cjong4`、`cjong4-opponent`、`cjong4-mjai`をWebAssembly化し、ブラウザで4人打ち麻雀と
視覚的デバッグを行うためのプロジェクトです。

Emscriptenビルド、ブラウザ向けC API、編集可能な対局ルール、4席の人間／opponent
選択、実対局の進行、合法手入力、局終了時の清算結果、再生・一時停止・次の停止地点までの進行、履歴の復元に対応しています。
対局状態はWasm側で保持し、ブラウザへバージョン付きJSONスナップショットを渡します。
各席の操作は対局中でも人間／opponentを切り替えられます。
山のシード指定と、MJAIイベント履歴の表示・コピー・JSON Lines保存に対応しています。

## ステータス / Status

v0.9.1リリース / v0.9.1 Release

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
2. 必要なら対局設定、山の設定、山のシードを変更します。
3. 「対局開始」を押します。
4. 「次へ進める」または「再生」で進行します。人間の手番では合法手だけを選べます。
5. 局終了時は、和了役、符・翻、支払点、本場・供託を含む点数移動を確認できます。
6. 対局履歴のスライダーで任意の状態へ戻れます。過去から進行すると後続履歴は破棄されます。
7. 「MJAI履歴」を開くと、その履歴位置までのイベントを確認・コピー・保存できます。

山の「ランダム」は画面で指定したseedから再現可能なシャッフルを行います。「プリセット
（デバッグ用）」は物理牌ID順の固定山を使用します。

ブラウザ向けAPI version 4では、設定、対局開始、操作切り替え、1状態遷移、人間の合法手選択、
履歴復元、UI snapshot JSON、MJAI JSON Linesを公開しています。状態JSON schema version 4には
局終了時の和了結果、清算後点数、MJAIイベント数を含みます。返却文字列はライブラリ所有です。

## ネイティブテスト

```sh
cmake -S . -B build -DCJ4_WEB_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## ライセンス

MIT License。Web部分と牌画像は本プロジェクトのオリジナル実装です。
Wasmツールチェーン由来部分のライセンスは
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)に記載し、公開用ビルドにも同梱します。
