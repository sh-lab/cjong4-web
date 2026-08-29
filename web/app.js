import createCjong4WebModule from "./cjong4-web.js?v=17";

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
const CONTROLLER_IDS = Object.freeze({
  human: 0,
  betaori: 1,
  chanta: 2,
  chiitoi: 3,
  kokushi: 4,
  pinfu: 5,
  somete: 6,
  tanyao: 7,
  toitoi: 8,
});
const CONTROLLER_NAMES = Object.freeze(Object.keys(CONTROLLER_IDS));
const ACTION_LABELS = Object.freeze({
  discard: "打牌",
  chi: "チー",
  pon: "ポン",
  ankan: "暗槓",
  minkan: "明槓",
  kakan: "加槓",
  riichi: "リーチ",
  tsumo: "ツモ",
  ron: "ロン",
  abortive_draw: "九種九牌",
  pass: "見送る",
});
const PHASE_LABELS = Object.freeze({
  draw: "自摸後",
  kakan_resolve: "加槓確認",
  ankan_resolve: "暗槓確認",
  after_call: "副露後",
  discard: "打牌確認",
  round_end: "局終了",
  settle: "精算",
  game_end: "対局終了",
});
const ROUND_RESULT_LABELS = Object.freeze({
  tsumo: "ツモ和了",
  ron: "ロン和了",
  exhaustive_draw: "流局",
  abortive_draw: "途中流局",
});
const ABORTIVE_DRAW_LABELS = Object.freeze({
  kyuushu_kyuuhai: "九種九牌",
  suufon_renda: "四風連打",
  four_riichi: "四家立直",
  four_kans: "四開槓",
  triple_ron: "三家和",
});
const YAKU_LABELS = Object.freeze({
  riichi: "立直", double_riichi: "ダブル立直", ippatsu: "一発",
  menzen_tsumo: "門前清自摸和", tanyao: "断么九", yakuhai_haku: "役牌 白",
  yakuhai_hatsu: "役牌 發", yakuhai_chun: "役牌 中", yakuhai_seat_wind: "自風牌",
  yakuhai_round_wind: "場風牌", chiitoi: "七対子", kokushi: "国士無双",
  kokushi_13_wait: "国士無双十三面待ち", toitoi: "対々和", honroutou: "混老頭",
  honitsu: "混一色", chinitsu: "清一色", pinfu: "平和", iipeikou: "一盃口",
  ryanpeikou: "二盃口", sanshoku_doujun: "三色同順", ittsuu: "一気通貫",
  chanta: "混全帯么九", junchan: "純全帯么九", sanankou: "三暗刻",
  shousangen: "小三元", daisangen: "大三元", shousuushii: "小四喜",
  daisuushii: "大四喜", tsuuiisou: "字一色", ryuuiisou: "緑一色",
  chinroutou: "清老頭", sankantsu: "三槓子", suukantsu: "四槓子",
  sanshoku_doukou: "三色同刻", suuankou: "四暗刻", suuankou_tanki: "四暗刻単騎",
  chuuren: "九蓮宝燈", junsei_chuuren: "純正九蓮宝燈", rinshan: "嶺上開花",
  haitei: "海底摸月", houtei: "河底撈魚", chankan: "槍槓", tenhou: "天和",
  chiihou: "地和", nagashi_mangan: "流し満貫",
});

let wasmModule;
let currentState;
let playbackTimer;
let dismissedSettlementHistory = -1;

function generateSeed() {
  const values = new Uint32Array(1);
  if (globalThis.crypto?.getRandomValues) globalThis.crypto.getRandomValues(values);
  else values[0] = Date.now() >>> 0;
  return values[0] || 1;
}

function setRandomSeed() {
  document.querySelector("#wall-seed").value = String(generateSeed());
}

function createTile(tile, label, options = {}) {
  const id = typeof tile === "string" ? tile : tile.tile;
  const selectable = Boolean(options.onSelect);
  const wrapper = document.createElement(selectable ? "button" : "span");
  wrapper.className = `tile${selectable ? " selectable" : ""}${options.className ? ` ${options.className}` : ""}`;
  if (selectable) {
    wrapper.type = "button";
    wrapper.addEventListener("click", options.onSelect);
  }
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
        <span class="meld-state">門前</span><span class="riichi-state">リーチなし</span><span class="turn-state">待機</span>
      </div>
      <div class="tile-row"><span class="row-label">手牌</span><div class="tiles hand"></div><div class="tiles melds"></div></div>
      <div class="tile-row"><span class="row-label">捨て牌</span><div class="tiles discards"></div></div>
    `;
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
  const regenerateSeed = document.querySelector("#regenerate-seed");

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
  document.querySelector("#wall-seed").addEventListener("change", () => {
    summary.textContent = "変更あり";
  });
  reset.addEventListener("click", () => {
    applyRules(defaults);
    wallMode.value = "random";
    setRandomSeed();
    summary.textContent = "デフォルト";
  });
  regenerateSeed.addEventListener("click", () => {
    setRandomSeed();
    summary.textContent = "変更あり";
  });
  setRandomSeed();
}

function readState() {
  const pointer = wasmModule._cj4_web_state_json();
  return JSON.parse(wasmModule.UTF8ToString(pointer));
}

function readMjaiHistory() {
  const pointer = wasmModule._cj4_web_mjai_log_jsonl();
  return wasmModule.UTF8ToString(pointer);
}

function configureRules() {
  wasmModule._cj4_web_rules_reset();
  RULE_GROUPS.flatMap((group) => group.fields).forEach((field, index) => {
    const input = document.querySelector(`#rule-${field.key}`);
    const value = field.type === "checkbox" ? Number(input.checked) : Number(input.value);
    if (!wasmModule._cj4_web_rule_set(index, value)) {
      throw new Error(`ルールを設定できませんでした: ${field.key}`);
    }
  });
}

function getSelectedControllers() {
  return [...document.querySelectorAll(".player-controller")].map((select) => CONTROLLER_IDS[select.value]);
}

function getActionTiles(action) {
  const tiles = [...action.tiles];
  const includesDiscardedTile = ["chi", "pon", "minkan"].includes(action.type);
  if (action.tile && (includesDiscardedTile || tiles.length === 0)) tiles.unshift(action.tile);
  return tiles;
}

function formatAction(action) {
  const tiles = getActionTiles(action).map((tile) => tile.tile);
  const suffix = tiles.length ? ` ${tiles.join(" ")}` : "";
  return `${ACTION_LABELS[action.type] ?? action.type}${suffix}`;
}

function createActionButton(action) {
  const button = document.createElement("button");
  const label = document.createElement("span");
  const tiles = getActionTiles(action);

  button.type = "button";
  button.className = "action-choice";
  button.setAttribute("aria-label", formatAction(action));
  label.className = "action-choice-label";
  label.textContent = ACTION_LABELS[action.type] ?? action.type;
  button.append(label);

  if (tiles.length) {
    const tileGroup = document.createElement("span");
    tileGroup.className = "action-choice-tiles";
    tiles.forEach((tile) => tileGroup.append(createTile(tile, `${tile.tile}の牌`)));
    button.append(tileGroup);
  }

  button.addEventListener("click", () => chooseAction(action.index));
  return button;
}

function stopPlayback() {
  if (playbackTimer !== undefined) {
    clearInterval(playbackTimer);
    playbackTimer = undefined;
  }
}

function advanceForcedState() {
  for (let guard = 0; guard < 4096; guard += 1) {
    if (!currentState?.active || currentState.waiting_for_input || currentState.phase === "game_end") return;

    const controller = currentState.players[currentState.current_player].controller;
    const resolvesReaction = ["discard", "kakan_resolve", "ankan_resolve"].includes(currentState.phase);
    const resolvesSettlement = currentState.phase === "settle";
    const isTurnPhase = ["draw", "after_call"].includes(currentState.phase);
    const handlesTurn = isTurnPhase && controller !== CONTROLLER_IDS.human;
    const preparesHumanTurn = isTurnPhase && controller === CONTROLLER_IDS.human;
    if (!resolvesReaction && !resolvesSettlement && !handlesTurn && !preparesHumanTurn) return;

    const result = wasmModule._cj4_web_game_step();
    currentState = readState();
    if (result === 0) return;
  }

  throw new Error("対局の自動進行が上限回数を超えました。");
}

function chooseAction(index) {
  stopPlayback();
  if (!currentState || !wasmModule._cj4_web_game_choose(currentState.generation, index)) return;
  currentState = readState();
  advanceForcedState();
  renderGameState();
}

function renderPendingActions() {
  const panel = document.querySelector("#action-panel");
  const title = document.querySelector("#action-title");
  const buttons = document.querySelector("#action-buttons");
  buttons.replaceChildren();

  if (!currentState.waiting_for_input) {
    panel.hidden = true;
    return;
  }

  panel.hidden = false;
  const hasDiscard = currentState.legal_actions.some((action) => action.type === "discard");
  title.textContent = `プレイヤー${currentState.pending_player + 1}: ${hasDiscard ? "手牌または行動を選択" : "行動を選択"}`;
  currentState.legal_actions
    .filter((action) => action.type !== "discard")
    .forEach((action) => buttons.append(createActionButton(action)));
}

function renderPlayer(player) {
  const panel = document.querySelectorAll(".player-panel")[player.player];
  const discardActions = new Map(
    currentState.legal_actions
      .filter((action) => action.type === "discard" && action.player === player.player)
      .map((action) => [action.tile.id, action.index]),
  );

  panel.classList.toggle("current-player", player.player === currentState.current_player);
  panel.querySelector(".seat-wind").textContent = WINDS[player.seat_wind];
  panel.querySelector(".player-identity h2").textContent = `プレイヤー${player.player + 1}`;
  panel.querySelector(".player-identity p").textContent = `${player.player === currentState.dealer ? "親" : "子"} · ${player.score}点`;
  const meldState = panel.querySelector(".meld-state");
  meldState.textContent = "門前";
  meldState.hidden = player.melds.length > 0;
  panel.querySelector(".riichi-state").textContent = player.riichi ? "リーチ" : "リーチなし";
  panel.querySelector(".turn-state").textContent = player.player === currentState.current_player ? "手番" : "待機";
  panel.querySelector(".player-controller").value = CONTROLLER_NAMES[player.controller];

  const hand = panel.querySelector(".hand");
  const hidesHand = document.querySelector("#hide-opponent-hands").checked
    && player.controller !== CONTROLLER_IDS.human;
  hand.replaceChildren();
  player.hand.forEach((tile) => {
    if (hidesHand) {
      hand.append(createTile("back", "伏せられた牌"));
      return;
    }
    const actionIndex = discardActions.get(tile.id);
    hand.append(createTile(tile, `${tile.tile}の牌`, {
      onSelect: actionIndex === undefined ? undefined : () => chooseAction(actionIndex),
    }));
  });

  const melds = panel.querySelector(".melds");
  melds.replaceChildren();
  player.melds.forEach((meld, meldIndex) => {
    if (meldIndex) {
      const separator = document.createElement("span");
      separator.className = "meld-break";
      melds.append(separator);
    }
    meld.tiles.forEach((tile) => melds.append(createTile(tile, `${meld.type}の${tile.tile}`)));
  });

  const discards = panel.querySelector(".discards");
  discards.replaceChildren();
  player.discards.forEach((discard) => {
    const classes = [
      discard.active ? "" : "inactive",
      discard.riichi ? "riichi-discard" : "",
    ].filter(Boolean).join(" ");
    discards.append(createTile(discard, `${discard.tile}の捨て牌`, { className: classes }));
  });
}

function settlementPaymentText(result, settlement) {
  if (settlement.type === "ron") return `${result.ron_points}点`;
  if (result.player === currentState.dealer) {
    return `${result.tsumo_non_dealer_payment}点オール`;
  }
  return `${result.tsumo_non_dealer_payment} / ${result.tsumo_dealer_payment}点`;
}

function renderSettlementWinner(result, settlement) {
  const card = document.createElement("article");
  const heading = document.createElement("div");
  const identity = document.createElement("div");
  const title = document.createElement("h3");
  const detail = document.createElement("p");
  const payment = document.createElement("strong");
  const yaku = document.createElement("div");

  card.className = "settlement-winner";
  heading.className = "settlement-winner-heading";
  title.textContent = `プレイヤー${result.player + 1}`;
  detail.textContent = result.yakuman_count
    ? `${result.yakuman_count === 1 ? "役満" : `${result.yakuman_count}倍役満`}`
    : `${result.fu}符 ${result.han}翻`;
  payment.className = "settlement-payment";
  payment.textContent = settlementPaymentText(result, settlement);
  yaku.className = "settlement-yaku";

  result.yaku.forEach((name) => {
    const tag = document.createElement("span");
    tag.textContent = YAKU_LABELS[name] ?? name;
    yaku.append(tag);
  });
  [
    ["ドラ", result.dora_count],
    ["裏ドラ", result.ura_dora_count],
    ["赤ドラ", result.aka_dora_count],
  ].forEach(([label, count]) => {
    if (!count) return;
    const tag = document.createElement("span");
    tag.textContent = `${label} ${count}`;
    yaku.append(tag);
  });

  identity.append(title, detail);
  heading.append(identity);
  if (currentState.settlement.winning_tile) {
    heading.append(createTile(
      currentState.settlement.winning_tile,
      `和了牌 ${currentState.settlement.winning_tile.tile}`,
    ));
  }
  card.append(heading, payment, yaku);
  return card;
}

function renderSettlement() {
  const layer = document.querySelector("#settlement-layer");
  const settlement = currentState.settlement;
  const historyIndex = currentState.history.index;

  if (currentState.phase !== "round_end" || !settlement || dismissedSettlementHistory === historyIndex) {
    layer.hidden = true;
    return;
  }

  layer.hidden = false;
  document.querySelector("#settlement-title").textContent = ROUND_RESULT_LABELS[settlement.type] ?? "局終了";
  const summary = document.querySelector("#settlement-summary");
  if (settlement.type === "ron") {
    summary.textContent = `放銃: プレイヤー${settlement.discarder + 1}`;
  } else if (settlement.type === "abortive_draw") {
    summary.textContent = ABORTIVE_DRAW_LABELS[settlement.abortive_reason] ?? "途中流局";
  } else if (settlement.type === "exhaustive_draw" && settlement.winners.length === 0) {
    summary.textContent = "荒牌平局";
  } else {
    summary.textContent = settlement.winners.map((winner) => `プレイヤー${winner.player + 1}`).join("・");
  }

  const winners = document.querySelector("#settlement-winners");
  winners.replaceChildren();
  settlement.winners.forEach((winner) => winners.append(renderSettlementWinner(winner, settlement)));

  const scoreChanges = document.querySelector("#settlement-score-changes");
  scoreChanges.replaceChildren();
  currentState.players.forEach((player) => {
    const delta = settlement.score_deltas[player.player];
    const item = document.createElement("div");
    const label = document.createElement("span");
    const score = document.createElement("strong");
    label.textContent = `P${player.player + 1}`;
    score.textContent = `${player.score} → ${settlement.scores_after[player.player]} (${delta > 0 ? "+" : ""}${delta})`;
    item.append(label, score);
    scoreChanges.append(item);
  });
}

function updateHistoryControls() {
  const history = currentState.history;
  const range = document.querySelector("#history-position");
  range.min = 0;
  range.max = Math.max(0, history.count - 1);
  range.value = history.index;
  range.disabled = history.count <= 1;
  document.querySelector("#history-output").textContent = `${history.index} / ${history.count - 1}手`;

  const ended = currentState.phase === "game_end";
  document.querySelector("#step-game").disabled = ended || currentState.waiting_for_input;
  document.querySelector("#pause-game").disabled = playbackTimer === undefined;
  document.querySelector("#play-game").disabled = ended || currentState.waiting_for_input || playbackTimer !== undefined;
}

function renderMjaiHistory() {
  const history = readMjaiHistory();
  document.querySelector("#mjai-history").textContent = history || "対局開始後に履歴を表示します";
  document.querySelector("#mjai-event-count").textContent = `${currentState.mjai.event_count}イベント`;
  document.querySelector("#copy-mjai").disabled = !history;
  document.querySelector("#download-mjai").disabled = !history;
}

function renderGameState() {
  if (!currentState?.active) return;

  const phaseLabel = PHASE_LABELS[currentState.phase] ?? currentState.phase;
  const ranking = [...currentState.players]
    .sort((left, right) => right.score - left.score || left.player - right.player)
    .map((player, index) => `${index + 1}位 P${player.player + 1} ${player.score}点`)
    .join(" / ");

  document.querySelector("#round-name").textContent = `${WINDS[currentState.round_wind]}${currentState.dealer + 1}局`;
  document.querySelector("#round-honba").textContent = `${currentState.honba}本場`;
  document.querySelector("#round-riichi").textContent = `供託 ${currentState.riichi_sticks}`;
  document.querySelector("#round-remaining").textContent = `残り ${currentState.remaining}枚`;
  document.querySelector("#round-note").textContent = currentState.phase === "game_end"
    ? `${phaseLabel} / ${ranking}`
    : `${phaseLabel} / seed ${currentState.seed}`;

  const dora = document.querySelector("#dora-indicators");
  dora.replaceChildren();
  currentState.dora_indicators.forEach((tile) => dora.append(createTile(tile, `ドラ表示牌 ${tile.tile}`)));

  currentState.players.forEach(renderPlayer);
  renderPendingActions();
  renderSettlement();
  updateHistoryControls();
  renderMjaiHistory();

  document.querySelector("#schema-version").textContent = currentState.schema_version;
  document.querySelector("#raw-bootstrap").textContent = JSON.stringify(currentState, null, 2);
  const status = document.querySelector("#engine-status");
  status.textContent = currentState.phase === "game_end" ? "対局終了" : "対局中";
  status.dataset.state = "ready";
}

function stepGame() {
  if (!currentState?.active) return 0;
  const result = wasmModule._cj4_web_game_step();
  currentState = readState();
  if (result === 1) advanceForcedState();
  if (result !== 1 || currentState.waiting_for_input ||
      currentState.phase === "round_end" || currentState.phase === "game_end") stopPlayback();
  renderGameState();
  return result;
}

function playGame() {
  if (!currentState?.active || currentState.waiting_for_input || currentState.phase === "game_end") return;
  playbackTimer = setInterval(stepGame, 70);
  updateHistoryControls();
}

function startGame() {
  stopPlayback();
  dismissedSettlementHistory = -1;
  configureRules();
  const controllers = getSelectedControllers();
  const seedInput = document.querySelector("#wall-seed");
  const seed = Number(seedInput.value);
  if (!Number.isInteger(seed) || seed < 1 || seed > 0xffffffff) {
    seedInput.focus();
    throw new Error("山のシードは1〜4294967295の整数で指定してください。");
  }
  const wallMode = document.querySelector("#wall-mode").value === "preset" ? 1 : 0;
  const started = wasmModule._cj4_web_game_start(seed, wallMode, ...controllers);
  if (!started) throw new Error("対局を開始できませんでした。設定値を確認してください。");

  currentState = readState();
  document.querySelector("#start-game").textContent = "新しい対局";
  renderGameState();
  if (controllers[currentState.current_player] === CONTROLLER_IDS.human) stepGame();
  else playGame();
}

function rewindGame(historyIndex) {
  stopPlayback();
  if (!wasmModule._cj4_web_game_rewind(Number(historyIndex))) return;
  dismissedSettlementHistory = -1;
  currentState = readState();
  renderGameState();
}

function changeController(player, controllerName) {
  if (!currentState?.active) return;

  stopPlayback();
  const controller = CONTROLLER_IDS[controllerName];
  if (!wasmModule._cj4_web_game_set_controller(player, controller)) {
    showFailure(new Error("操作を切り替えられませんでした。"), "操作切り替えに失敗");
    return;
  }

  currentState = readState();
  const isCurrentHumanTurn = controller === CONTROLLER_IDS.human &&
    player === currentState.current_player &&
    (currentState.phase === "draw" || currentState.phase === "after_call") &&
    !currentState.waiting_for_input;
  if (isCurrentHumanTurn) stepGame();
  else renderGameState();
}

function wireGameControls() {
  document.querySelectorAll(".player-controller").forEach((select, player) => {
    select.addEventListener("change", () => changeController(player, select.value));
  });
  document.querySelector("#start-game").addEventListener("click", () => {
    try {
      startGame();
    } catch (error) {
      showFailure(error, "対局開始に失敗");
    }
  });
  document.querySelector("#step-game").addEventListener("click", stepGame);
  document.querySelector("#pause-game").addEventListener("click", () => {
    stopPlayback();
    updateHistoryControls();
  });
  document.querySelector("#play-game").addEventListener("click", playGame);
  document.querySelector("#history-position").addEventListener("input", (event) => rewindGame(event.target.value));
  document.querySelector("#hide-opponent-hands").addEventListener("change", renderGameState);
  document.querySelector("#settlement-close").addEventListener("click", () => {
    dismissedSettlementHistory = currentState?.history.index ?? -1;
    document.querySelector("#settlement-layer").hidden = true;
  });
  document.querySelector("#settlement-next").addEventListener("click", () => {
    dismissedSettlementHistory = currentState?.history.index ?? -1;
    stepGame();
  });
  document.querySelector("#copy-mjai").addEventListener("click", async () => {
    const status = document.querySelector("#mjai-copy-status");
    try {
      await navigator.clipboard.writeText(readMjaiHistory());
      status.textContent = "コピーしました";
    } catch (error) {
      status.textContent = "コピーできませんでした";
    }
  });
  document.querySelector("#download-mjai").addEventListener("click", () => {
    const blob = new Blob([readMjaiHistory()], { type: "application/x-ndjson;charset=utf-8" });
    const link = document.createElement("a");
    link.href = URL.createObjectURL(blob);
    link.download = `cjong4-${currentState.seed}.jsonl`;
    link.click();
    URL.revokeObjectURL(link.href);
  });
}

function showFailure(error, message = "Wasmの読み込みに失敗") {
  const status = document.querySelector("#engine-status");
  status.textContent = message;
  status.dataset.state = "error";
  document.querySelector("#raw-bootstrap").textContent = String(error);
}

async function main() {
  try {
    wasmModule = await createCjong4WebModule({
      locateFile: (path) => path.endsWith(".wasm") ? `${path}?v=17` : path,
    });
    const pointer = wasmModule._cj4_web_bootstrap_json();
    const raw = wasmModule.UTF8ToString(pointer);
    const bootstrap = JSON.parse(raw);
    const apiVersion = wasmModule._cj4_web_api_version();
    if (apiVersion !== 4) {
      throw new Error(`Wasm APIの版が一致しません（期待値: 4、実際: ${apiVersion}）。`);
    }

    renderPlayers(bootstrap.opponents);
    initializeRules(bootstrap.rules);
    wireGameControls();

    document.querySelector("#api-version").textContent = apiVersion;
    document.querySelector("#schema-version").textContent = bootstrap.schema_version;
    document.querySelector("#engine-ready").textContent = bootstrap.engine.ready ? "ready" : "error";
    document.querySelector("#raw-bootstrap").textContent = JSON.stringify(bootstrap, null, 2);

    const status = document.querySelector("#engine-status");
    status.textContent = bootstrap.engine.ready ? "Wasm接続済み" : "エンジン初期化エラー";
    status.dataset.state = bootstrap.engine.ready ? "ready" : "error";
    document.querySelector("#start-game").disabled = !bootstrap.engine.ready;
  } catch (error) {
    showFailure(error);
  }
}

main();
