import createCjong4WebModule from "./cjong4-web.js";

const WINDS = ["東", "南", "西", "北"];
const RULE_GROUPS = [
  {
    title: "対局構成",
    fields: [
      { key: "game_type", label: "対局形式", type: "select", options: [[0, "東風戦"], [1, "半荘戦"], [2, "一荘戦"]] },
      { key: "initial_score", label: "開始点", type: "number", min: 100, step: 100 },
      { key: "target_score", label: "返し点", type: "number", min: 100, step: 100 },
      { key: "tobi_end", label: "飛び終了", type: "checkbox" },
      { key: "target_score_excludes_riichi_sticks", label: "終了判定の目標点から供託を除外", type: "checkbox" },
    ],
  },
  {
    title: "一般",
    fields: [
      { key: "kuitan", label: "喰い断あり", type: "checkbox" },
      { key: "kuikae_forbidden", label: "喰い替え禁止", type: "checkbox" },
      { key: "kan_dora_timing", label: "槓ドラの公開時期", type: "select", options: [[0, "即時"], [1, "打牌後"]] },
      { key: "ippatsu", label: "一発あり", type: "checkbox" },
      { key: "aka_5m", label: "赤五萬", type: "number", min: 0, max: 4, step: 1, suffix: "枚" },
      { key: "aka_5p", label: "赤五筒", type: "number", min: 0, max: 4, step: 1, suffix: "枚" },
      { key: "aka_5s", label: "赤五索", type: "number", min: 0, max: 4, step: 1, suffix: "枚" },
    ],
  },
  {
    title: "ロン",
    fields: [
      { key: "max_ron_players", label: "同時ロン上限", type: "select", options: [[1, "1人（頭ハネ）"], [2, "2人"], [3, "3人"]] },
      { key: "kokushi_ron_on_ankan", label: "国士無双の暗槓ロン", type: "checkbox" },
      { key: "triple_ron_abortive_draw", label: "三家和を途中流局にする", type: "checkbox" },
    ],
  },
  {
    title: "流局",
    fields: [
      { key: "noten_penalty", label: "ノーテン罰符あり", type: "checkbox" },
      { key: "noten_penalty_points", label: "ノーテン罰符の総額", type: "number", min: 0, step: 100, suffix: "点" },
      { key: "abortive_kyuushu_kyuuhai", label: "九種九牌", type: "checkbox" },
      { key: "abortive_suufon_renda", label: "四風連打", type: "checkbox" },
      { key: "abortive_four_riichi", label: "四家立直", type: "checkbox" },
      { key: "nagashi_mangan", label: "流し満貫", type: "checkbox" },
      { key: "nagashi_dealer_tenpai_renchan", label: "親の流し満貫を聴牌連荘扱い", type: "checkbox" },
    ],
  },
  {
    title: "打点",
    fields: [
      { key: "kokushi_13_wait_double", label: "国士無双十三面待ちをダブル役満", type: "checkbox" },
      { key: "suuankou_tanki_double", label: "四暗刻単騎をダブル役満", type: "checkbox" },
      { key: "junsei_chuuren_double", label: "純正九蓮宝燈をダブル役満", type: "checkbox" },
      { key: "daisuushii_double", label: "大四喜をダブル役満", type: "checkbox" },
      { key: "kazoe_yakuman", label: "数え役満あり", type: "checkbox" },
      { key: "kiriage_mangan", label: "切り上げ満貫", type: "checkbox" },
    ],
  },
  {
    title: "責任払い・精算",
    fields: [
      { key: "pao", label: "責任払いあり", type: "checkbox" },
      { key: "pao_liability_only", label: "責任者のみが支払う", type: "checkbox" },
      { key: "pao_daisangen", label: "大三元の責任払い", type: "checkbox" },
      { key: "pao_daisuushii", label: "大四喜の責任払い", type: "checkbox" },
      { key: "pao_suukantsu", label: "四槓子の責任払い", type: "checkbox" },
      { key: "multi_ron_honba_first_only", label: "複数ロンの本場は上家取り", type: "checkbox" },
    ],
  },
];
const RULE_DEFAULTS = Object.freeze({
  game_type: 1,
  initial_score: 25000,
  target_score: 30000,
  tobi_end: true,
  target_score_excludes_riichi_sticks: true,
  kuitan: true,
  kuikae_forbidden: true,
  kan_dora_timing: 1,
  ippatsu: true,
  aka_5m: 1,
  aka_5p: 1,
  aka_5s: 1,
  max_ron_players: 3,
  kokushi_ron_on_ankan: false,
  triple_ron_abortive_draw: true,
  noten_penalty: true,
  noten_penalty_points: 3000,
  abortive_kyuushu_kyuuhai: true,
  abortive_suufon_renda: true,
  abortive_four_riichi: true,
  nagashi_mangan: true,
  nagashi_dealer_tenpai_renchan: true,
  kokushi_13_wait_double: false,
  suuankou_tanki_double: false,
  junsei_chuuren_double: false,
  daisuushii_double: false,
  kazoe_yakuman: true,
  kiriage_mangan: false,
  pao: true,
  pao_liability_only: false,
  pao_daisangen: true,
  pao_daisuushii: true,
  pao_suukantsu: false,
  multi_ron_honba_first_only: true,
});
const SAMPLE_HANDS = [
  ["1m", "2m", "3m", "4p", "5p", "6p", "2s", "3s", "4s", "7s", "8s", "9s", "1z"],
  ["2m", "3m", "4m", "6m", "7m", "8m", "1p", "1p", "4s", "5s", "6s", "6z", "6z"],
  ["1m", "1m", "5m", "5m", "2p", "2p", "7p", "7p", "3s", "3s", "7s", "7s", "5z"],
  ["7m", "8m", "9m", "3p", "4p", "5p", "1s", "2s", "3s", "5s", "5s", "7z", "7z"],
];
const SAMPLE_DISCARDS = [
  ["9m", "1p", "9p"],
  ["1s", "9s", "2z"],
  ["4z", "9m"],
  ["3z", "8p", "1m"],
];

function createTile(id, label, selectable = false) {
  const wrapper = document.createElement(selectable ? "button" : "span");
  wrapper.className = `tile${selectable ? " selectable" : ""}`;
  if (selectable) wrapper.type = "button";
  wrapper.setAttribute("aria-label", label);
  wrapper.innerHTML = `<svg viewBox="0 0 72 100" aria-hidden="true"><use href="./assets/tiles.svg#tile-${id}"></use></svg>`;
  return wrapper;
}

function renderPlayers(opponents) {
  const players = document.querySelector("#players");
  players.replaceChildren();

  WINDS.forEach((wind, index) => {
    const panel = document.createElement("article");
    panel.className = "player-panel";
    panel.innerHTML = `
      <header class="player-header">
        <div class="player-identity">
          <span class="seat-wind">${wind}</span>
          <div><h2>プレイヤー${index + 1}</h2><p>${index === 0 ? "親" : "子"} · 25000点</p></div>
        </div>
        <label>操作
          <select class="player-controller" aria-label="プレイヤー${index + 1}の操作">
            <option value="human">人間</option>
            ${opponents.map((name) => `<option value="${name}"${name === "betaori" ? " selected" : ""}>opponent: ${name}</option>`).join("")}
          </select>
        </label>
      </header>
      <div class="player-state">
        <span>門前</span><span>リーチなし</span><span>手番待ち</span>
      </div>
      <div class="tile-row"><span class="row-label">手牌</span><div class="tiles hand"></div></div>
      <div class="tile-row"><span class="row-label">捨て牌</span><div class="tiles discards"></div></div>
    `;

    const hand = panel.querySelector(".hand");
    SAMPLE_HANDS[index].forEach((id) => hand.append(createTile(id, `${id}の牌`, index === 0)));
    const discards = panel.querySelector(".discards");
    SAMPLE_DISCARDS[index].forEach((id) => discards.append(createTile(id, `${id}の捨て牌`)));
    players.append(panel);
  });
}

function createRuleControl(field) {
  const control = document.createElement("label");
  control.className = field.type === "checkbox" ? "rule-control rule-check" : "rule-control";

  const input = document.createElement(field.type === "select" ? "select" : "input");
  input.id = `rule-${field.key}`;
  input.dataset.ruleKey = field.key;

  if (field.type === "checkbox") {
    input.type = "checkbox";
    const label = document.createElement("span");
    label.textContent = field.label;
    control.append(input, label);
    return control;
  }

  const label = document.createElement("span");
  label.textContent = field.label;
  control.append(label, input);

  if (field.type === "select") {
    field.options.forEach(([value, text]) => {
      const option = document.createElement("option");
      option.value = value;
      option.textContent = text;
      input.append(option);
    });
  } else {
    input.type = "number";
    input.min = field.min;
    if (field.max !== undefined) input.max = field.max;
    input.step = field.step;
  }

  if (field.suffix) {
    const valueRow = document.createElement("span");
    valueRow.className = "rule-value";
    input.replaceWith(valueRow);
    valueRow.append(input, document.createTextNode(field.suffix));
  }
  return control;
}

function renderRuleControls() {
  const sections = document.querySelector("#rule-sections");
  sections.replaceChildren();

  RULE_GROUPS.forEach((group) => {
    const section = document.createElement("section");
    section.className = "rule-group";
    const heading = document.createElement("h3");
    heading.textContent = group.title;
    const grid = document.createElement("div");
    grid.className = "rule-grid";
    group.fields.forEach((field) => grid.append(createRuleControl(field)));
    section.append(heading, grid);
    sections.append(section);
  });
}

function applyRules(rules) {
  RULE_GROUPS.flatMap((group) => group.fields).forEach((field) => {
    const input = document.querySelector(`#rule-${field.key}`);
    if (field.type === "checkbox") input.checked = Boolean(rules[field.key]);
    else input.value = String(rules[field.key]);
  });
}

function initializeRules(rules) {
  const defaults = { ...RULE_DEFAULTS, ...rules };
  const summary = document.querySelector("#rule-summary");
  const reset = document.querySelector("#reset-rules");
  const wallMode = document.querySelector("#wall-mode");

  renderRuleControls();
  applyRules(defaults);
  summary.textContent = "デフォルト";
  reset.disabled = false;

  document.querySelector("#rule-sections").addEventListener("change", () => {
    summary.textContent = "変更あり";
  });
  wallMode.addEventListener("change", () => {
    summary.textContent = "変更あり";
  });
  reset.addEventListener("click", () => {
    applyRules(defaults);
    wallMode.value = "random";
    summary.textContent = "デフォルト";
  });
}

function showFailure(error) {
  const status = document.querySelector("#engine-status");
  status.textContent = "Wasmの読み込みに失敗";
  status.dataset.state = "error";
  document.querySelector("#raw-bootstrap").textContent = String(error);
}

async function main() {
  try {
    const module = await createCjong4WebModule();
    const pointer = module._cj4_web_bootstrap_json();
    const raw = module.UTF8ToString(pointer);
    const bootstrap = JSON.parse(raw);
    const apiVersion = module._cj4_web_api_version();

    renderPlayers(bootstrap.opponents);
    initializeRules(bootstrap.rules);

    document.querySelector("#api-version").textContent = apiVersion;
    document.querySelector("#schema-version").textContent = bootstrap.schema_version;
    document.querySelector("#engine-ready").textContent = bootstrap.engine.ready ? "ready" : "error";
    document.querySelector("#raw-bootstrap").textContent = JSON.stringify(bootstrap, null, 2);

    const status = document.querySelector("#engine-status");
    status.textContent = bootstrap.engine.ready ? "Wasm接続済み" : "エンジン初期化エラー";
    status.dataset.state = bootstrap.engine.ready ? "ready" : "error";
  } catch (error) {
    showFailure(error);
  }
}

main();
