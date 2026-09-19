const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');
const vm = require('node:vm');

const source = fs.readFileSync(path.join(__dirname, '../web/app.js'), 'utf8')
  .replace(/^import .*;\n/, '').replace(/\nmain\(\);\s*$/, '');

class Element {
  constructor() { this.children = []; this.value = '0'; this.attributes = {}; }
  append(...children) { this.children.push(...children); }
  setAttribute(name, value) { this.attributes[name] = value; }
}

function setup(state) {
  const timers = new Map();
  let nextTimer = 0;
  const context = vm.createContext({
    document: { createElement: () => new Element(), querySelector: () => new Element() },
    setTimeout: (callback) => { timers.set(++nextTimer, callback); return nextTimer; },
    clearTimeout: (id) => timers.delete(id),
  });
  vm.runInContext(source + `
    globalThis.app = {
      setState(value) { currentState = value; },
      state() { return currentState; },
      bind(module, reader, render) {
        wasmModule = module;
        readState = reader;
        renderGameState = render;
        updateHistoryControls = () => {};
      },
      dismiss() { dismissedSettlementHistory = currentState.history.index; },
      playGame, stepGame, closeSettlement, continueAfterSettlement, createSettlementHand,
    };
  `, context);
  context.app.setState(state);
  return { app: context.app, timers };
}

function state(phase, type = 'ron') {
  return { active: true, phase, current_player: 0, waiting_for_input: false,
    history: { index: 10 }, players: [{ player: 0, controller: 9 }],
    settlement: { type } };
}

for (const type of ['ron', 'tsumo', 'exhaustive_draw', 'abortive_draw']) {
  test(`closing ${type} cancels playback and redraws without advancing`, () => {
    const { app, timers } = setup(state('draw', type));
    let steps = 0;
    let renders = 0;
    app.bind({ _cj4_web_game_step: () => ++steps }, () => app.state(), () => ++renders);
    app.playGame();
    assert.equal(timers.size, 1);
    app.setState(state('round_end', type));
    app.closeSettlement();
    assert.equal(steps, 0);
    assert.equal(timers.size, 0);
    assert.equal(renders, 1);
    assert.equal(app.state().phase, 'round_end');
  });
}

for (const action of ['stepGame', 'playGame', 'continueAfterSettlement']) {
  test(`${action} draws the next deal before any automated discard`, () => {
    const { app, timers } = setup(state('round_end'));
    const phases = ['settle', 'draw', 'discard', 'draw', 'round_end'];
    let steps = 0;
    const rendered = [];
    app.bind({ _cj4_web_game_step: () => { ++steps; return 1; } },
      () => state(phases[steps - 1]), () => rendered.push(app.state().phase));
    app.dismiss();
    app[action]();
    assert.equal(steps, 2);
    assert.deepEqual(rendered, ['draw']);
    assert.equal(timers.size, 1);
  });
}

test('render failure cannot leave automated engine advancement queued', () => {
  const { app, timers } = setup(state('draw'));
  app.bind({ _cj4_web_game_step: () => 1 }, () => state('discard'),
    () => { throw new Error('render failed'); });
  app.playGame();
  const [id, callback] = timers.entries().next().value;
  timers.delete(id);
  assert.throws(callback, /render failed/);
  assert.equal(timers.size, 0);
});

test('next round prepares a human turn without scheduling playback', () => {
  const { app, timers } = setup(state('round_end'));
  let steps = 0;
  app.bind({ _cj4_web_game_step: () => { ++steps; return steps === 3 ? 2 : 1; } }, () => {
    const next = state(steps === 1 ? 'settle' : 'draw');
    next.players[0].controller = 0;
    next.waiting_for_input = steps === 3;
    return next;
  }, () => {});
  app.continueAfterSettlement();
  assert.equal(steps, 3);
  assert.equal(timers.size, 0);
  assert.equal(app.state().waiting_for_input, true);
});

function tiles(count) {
  return Array.from({ length: count }, (_, id) => ({ id, tile: `${id % 9 + 1}m` }));
}

for (const type of ['ron', 'tsumo']) {
  test(`${type} displays 13 concealed tiles and one separate winning tile`, () => {
    const current = state('round_end', type);
    current.players[0].hand = tiles(type === 'tsumo' ? 14 : 13);
    current.players[0].melds = [];
    const { app } = setup(current);
    const hand = app.createSettlementHand({ player: 0 }, { type, winning_tile: tiles(14)[13] });
    assert.equal(hand.children[0].children[1].children.length, 13);
    assert.equal(hand.children[1].children[0].textContent, type === 'ron' ? 'ロン牌' : 'ツモ牌');
    assert.equal(hand.children[1].children[1].children.length, 1);
  });
}

test('winner-specific hands include open melds and concealed kan backs', () => {
  const current = state('round_end');
  current.players.push({ player: 1, hand: tiles(7), melds: [
    { type: 'pon', tiles: tiles(3) }, { type: 'ankan', tiles: tiles(4) },
  ] });
  const { app } = setup(current);
  const hand = app.createSettlementHand({ player: 1 }, { type: 'ron', winning_tile: { id: 99, tile: '1z' } });
  assert.equal(hand.children[0].children[1].children.length, 7);
  const melds = hand.children[2];
  assert.equal(melds.children[1].children.length, 3);
  assert.deepEqual(Array.from(melds.children[2].children, (tile) => tile.innerHTML.includes('tile-back')), [true, false, false, true]);
});

test('nagashi mangan does not append a winning tile', () => {
  const current = state('round_end', 'exhaustive_draw');
  current.players[0].hand = tiles(13);
  current.players[0].melds = [];
  const { app } = setup(current);
  const hand = app.createSettlementHand({ player: 0 }, { type: 'exhaustive_draw', winning_tile: null });
  assert.equal(hand.children.length, 1);
  assert.equal(hand.children[0].children[1].children.length, 13);
});
