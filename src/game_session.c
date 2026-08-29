#include <cjong4/web/api.h>

#include <cjong4/core/state_abortive.h>
#include <cjong4/core/state_chi.h>
#include <cjong4/core/state_discard.h>
#include <cjong4/core/state_init.h>
#include <cjong4/core/state_kan.h>
#include <cjong4/core/state_pass.h>
#include <cjong4/core/state_pon.h>
#include <cjong4/core/state_query.h>
#include <cjong4/core/state_riichi.h>
#include <cjong4/core/state_ron.h>
#include <cjong4/core/state_round.h>
#include <cjong4/core/state_settle.h>
#include <cjong4/core/state_tsumo.h>
#include <cjong4/manager/manager.h>
#include <cjong4/opponent/opponent_betaori.h>
#include <cjong4/opponent/opponent_chanta.h>
#include <cjong4/opponent/opponent_chiitoi.h>
#include <cjong4/opponent/opponent_kokushi.h>
#include <cjong4/opponent/opponent_pinfu.h>
#include <cjong4/opponent/opponent_somete.h>
#include <cjong4/opponent/opponent_tanyao.h>
#include <cjong4/opponent/opponent_toitoi.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define CJ4_WEB_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define CJ4_WEB_EXPORT
#endif

enum
{
    CJ4_WEB_HISTORY_CAPACITY = 8192,
    CJ4_WEB_STATE_JSON_CAPACITY = 131072,
    CJ4_WEB_MJAI_LOG_CAPACITY = 2097152,
    CJ4_WEB_MJAI_EVENT_CAPACITY = 4096,
    CJ4_WEB_WALL_RANDOM = 0,
    CJ4_WEB_WALL_PRESET = 1,
    CJ4_WEB_PENDING_NONE = 0,
    CJ4_WEB_PENDING_SINGLE = 1,
    CJ4_WEB_PENDING_REACTION = 2
};

typedef struct
{
    cj4_mahjong state;
    uint32_t random_state;
    size_t mjai_length;
    uint32_t mjai_event_count;
} cj4_web_snapshot;

typedef struct
{
    char *data;
    size_t capacity;
    size_t length;
    uint8_t valid;
} cj4_web_json_writer;

typedef struct
{
    uint8_t active;
    uint8_t wall_mode;
    uint8_t controllers[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_rules rules;
    cj4_mahjong state;
    uint32_t seed;
    uint32_t random_state;
    uint32_t generation;
    size_t mjai_length;
    uint32_t mjai_event_count;

    cj4_web_snapshot history[CJ4_WEB_HISTORY_CAPACITY];
    uint32_t history_count;
    uint32_t history_index;

    uint8_t pending_kind;
    cj4_player pending_player;
    cj4_action pending_actions[CJ4M_MAX_ACTIONS];
    uint8_t pending_action_count;

    uint8_t reaction_active;
    uint8_t reaction_done_mask;
    cj4_action reaction_actions[CJ4_PLAYER_COUNT];
} cj4_web_session;

static cj4_web_session cj4_web_game;
static cj4_rules cj4_web_configured_rules;
static uint8_t cj4_web_rules_initialized;
static char cj4_web_state_json_buffer[CJ4_WEB_STATE_JSON_CAPACITY];
static char cj4_web_mjai_log_buffer[CJ4_WEB_MJAI_LOG_CAPACITY];
static char cj4_web_mjai_output_buffer[CJ4_WEB_MJAI_LOG_CAPACITY];

static void
cj4_web_record_transition(const cj4_mahjong *previous, const cj4_mahjong *next);

static const char *const cj4_web_phase_names[] = {
    "draw", "kakan_resolve", "ankan_resolve", "after_call",
    "discard", "round_end", "settle", "game_end"};

static const char *const cj4_web_action_names[] = {
    "discard", "chi", "pon", "ankan", "minkan", "kakan",
    "riichi", "tsumo", "ron", "abortive_draw", "pass"};

static const char *const cj4_web_meld_names[] = {
    "chi", "pon", "minkan", "ankan", "kakan"};

static const char *const cj4_web_round_end_names[] = {
    "none", "tsumo", "ron", "exhaustive_draw", "abortive_draw"};

static const char *const cj4_web_abortive_draw_names[] = {
    "none", "kyuushu_kyuuhai", "suufon_renda", "four_riichi",
    "four_kans", "triple_ron"};

static const char *const cj4_web_yaku_names[] = {
    "riichi", "double_riichi", "ippatsu", "menzen_tsumo", "tanyao",
    "yakuhai_haku", "yakuhai_hatsu", "yakuhai_chun", "yakuhai_seat_wind",
    "yakuhai_round_wind", "chiitoi", "kokushi", "kokushi_13_wait", "toitoi",
    "honroutou", "honitsu", "chinitsu", "pinfu", "iipeikou", "ryanpeikou",
    "sanshoku_doujun", "ittsuu", "chanta", "junchan", "sanankou",
    "shousangen", "daisangen", "shousuushii", "daisuushii", "tsuuiisou",
    "ryuuiisou", "chinroutou", "sankantsu", "suukantsu", "sanshoku_doukou",
    "suuankou", "suuankou_tanki", "chuuren", "junsei_chuuren", "rinshan",
    "haitei", "houtei", "chankan", "tenhou", "chiihou", "nagashi_mangan"};

static void
cj4_web_json_append(cj4_web_json_writer *writer, const char *format, ...)
{
    va_list arguments;
    int written;

    if (!writer || !writer->valid || writer->length >= writer->capacity)
        return;

    va_start(arguments, format);
    written = vsnprintf(
        writer->data + writer->length,
        writer->capacity - writer->length,
        format,
        arguments);
    va_end(arguments);

    if (written < 0 || (size_t)written >= writer->capacity - writer->length)
    {
        writer->valid = 0;
        writer->data[writer->capacity - 1] = '\0';
        return;
    }
    writer->length += (size_t)written;
}

static uint32_t
cj4_web_random_next(void)
{
    uint32_t value = cj4_web_game.random_state;

    if (value == 0)
        value = 0x6d2b79f5u;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    cj4_web_game.random_state = value;
    return value;
}

static void
cj4_web_fill_wall(cj4_tile_id wall[CJ4_TILE_ID_COUNT])
{
    for (uint16_t index = 0; index < CJ4_TILE_ID_COUNT; ++index)
        wall[index] = (cj4_tile_id)index;

    if (cj4_web_game.wall_mode == CJ4_WEB_WALL_PRESET)
        return;

    for (int index = CJ4_TILE_ID_COUNT - 1; index > 0; --index)
    {
        uint32_t selected = cj4_web_random_next() % (uint32_t)(index + 1);
        cj4_tile_id temporary = wall[index];
        wall[index] = wall[selected];
        wall[selected] = temporary;
    }
}

static cj4m_player_delegate
cj4_web_make_delegate(uint8_t controller)
{
    switch (controller)
    {
    case CJ4_WEB_CONTROLLER_CHANTA:
        return cj4_opponent_chanta(1);
    case CJ4_WEB_CONTROLLER_CHIITOI:
        return cj4_opponent_chiitoi(1);
    case CJ4_WEB_CONTROLLER_KOKUSHI:
        return cj4_opponent_kokushi(1);
    case CJ4_WEB_CONTROLLER_PINFU:
        return cj4_opponent_pinfu(1);
    case CJ4_WEB_CONTROLLER_SOMETE:
        return cj4_opponent_somete(1);
    case CJ4_WEB_CONTROLLER_TANYAO:
        return cj4_opponent_tanyao(1);
    case CJ4_WEB_CONTROLLER_TOITOI:
        return cj4_opponent_toitoi(1);
    case CJ4_WEB_CONTROLLER_HUMAN:
    case CJ4_WEB_CONTROLLER_BETAORI:
    default:
        return cj4_opponent_betaori(1);
    }
}

static uint8_t
cj4_web_action_equal(const cj4_action *left, const cj4_action *right)
{
    if (left->type != right->type || left->player != right->player ||
        left->tile != right->tile || left->tile_count != right->tile_count)
        return 0;

    for (uint8_t index = 0; index < left->tile_count; ++index)
        if (left->tiles[index] != right->tiles[index])
            return 0;
    return 1;
}

static cj4_action
cj4_web_select_delegate_action(
    cj4_player player,
    const cj4_action *actions,
    uint8_t action_count)
{
    cj4_action fallback;
    cj4_player_view view = cj4m_make_player_view(&cj4_web_game.state, player);

    memset(&fallback, 0, sizeof(fallback));
    fallback.type = CJ4_ACTION_PASS;
    fallback.player = player;
    fallback.tile = CJ4_TILE_ID_INVALID;
    if (!actions || action_count == 0)
        return fallback;

    cj4_action selected = cj4_web_game.delegates[player].decide(
        cj4_web_game.delegates[player].ctx,
        &view,
        actions,
        action_count);

    for (uint8_t index = 0; index < action_count; ++index)
        if (cj4_web_action_equal(&selected, &actions[index]))
            return selected;

    for (uint8_t index = 0; index < action_count; ++index)
        if (actions[index].type == CJ4_ACTION_PASS)
            return actions[index];
    return actions[0];
}

static cj4_mahjong
cj4_web_apply_action(const cj4_action *action)
{
    const cj4_mahjong *state = &cj4_web_game.state;

    switch (action->type)
    {
    case CJ4_ACTION_DISCARD:
        return cj4_do_discard_with_rules(*state, &cj4_web_game.rules, action->tile);
    case CJ4_ACTION_CHI:
        return cj4_do_chi(*state, action->tiles[0], action->tiles[1]);
    case CJ4_ACTION_PON:
        return cj4_do_pon(*state, action->player, action->tiles[0], action->tiles[1]);
    case CJ4_ACTION_ANKAN:
        return cj4_do_ankan(
            *state, action->tiles[0], action->tiles[1], action->tiles[2], action->tiles[3]);
    case CJ4_ACTION_MINKAN:
        return cj4_do_minkan(
            *state, &cj4_web_game.rules, action->player,
            action->tiles[0], action->tiles[1], action->tiles[2]);
    case CJ4_ACTION_KAKAN:
        return cj4_do_kakan(*state, action->tile);
    case CJ4_ACTION_RIICHI:
        return cj4_do_riichi(*state, action->tile);
    case CJ4_ACTION_TSUMO:
        return cj4_do_tsumo(*state);
    case CJ4_ACTION_RON:
    {
        cj4_player player = action->player;
        return cj4_do_ron_multi(*state, &player, 1, &cj4_web_game.rules);
    }
    case CJ4_ACTION_ABORTIVE_DRAW:
        return cj4_do_kyuushu_kyuuhai(*state);
    case CJ4_ACTION_PASS:
    default:
        return *state;
    }
}

static void
cj4_web_clear_pending(void)
{
    cj4_web_game.pending_kind = CJ4_WEB_PENDING_NONE;
    cj4_web_game.pending_player = CJ4_PLAYER_COUNT;
    cj4_web_game.pending_action_count = 0;
}

static void
cj4_web_store_state(cj4_mahjong state)
{
    if (cj4_web_game.history_index + 1 < cj4_web_game.history_count)
    {
        cj4_web_game.history_count = cj4_web_game.history_index + 1;
        cj4_web_game.mjai_length =
            cj4_web_game.history[cj4_web_game.history_index].mjai_length;
        cj4_web_game.mjai_event_count =
            cj4_web_game.history[cj4_web_game.history_index].mjai_event_count;
    }

    cj4_web_record_transition(&cj4_web_game.state, &state);

    cj4_web_game.state = state;
    cj4_web_game.generation++;
    cj4_web_clear_pending();
    cj4_web_game.reaction_active = 0;
    cj4_web_game.reaction_done_mask = 0;

    if (cj4_web_game.history_count < CJ4_WEB_HISTORY_CAPACITY)
    {
        cj4_web_game.history_index = cj4_web_game.history_count;
        cj4_web_game.history[cj4_web_game.history_count].state = state;
        cj4_web_game.history[cj4_web_game.history_count].random_state =
            cj4_web_game.random_state;
        cj4_web_game.history[cj4_web_game.history_count].mjai_length =
            cj4_web_game.mjai_length;
        cj4_web_game.history[cj4_web_game.history_count].mjai_event_count =
            cj4_web_game.mjai_event_count;
        cj4_web_game.history_count++;
    }
}

static void
cj4_web_prepare_pending(
    uint8_t kind,
    cj4_player player,
    const cj4_action *actions,
    uint8_t action_count)
{
    cj4_web_game.pending_kind = kind;
    cj4_web_game.pending_player = player;
    cj4_web_game.pending_action_count = action_count;
    memcpy(
        cj4_web_game.pending_actions,
        actions,
        (size_t)action_count * sizeof(actions[0]));
}

static uint8_t
cj4_web_has_choice(const cj4_action *actions, uint8_t action_count)
{
    if (action_count > 1)
        return 1;
    return action_count == 1 && actions[0].type != CJ4_ACTION_PASS;
}

static uint8_t
cj4_web_distance_from_current(cj4_player player)
{
    return (uint8_t)((player + CJ4_PLAYER_COUNT -
                      cj4_state_current_player(&cj4_web_game.state)) %
                     CJ4_PLAYER_COUNT);
}

static void
cj4_web_resolve_reactions(void)
{
    cj4_phase phase = cj4_state_phase(&cj4_web_game.state);
    cj4_player ron_players[CJ4_PLAYER_COUNT];
    uint8_t ron_count = 0;

    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
        if (player != cj4_state_current_player(&cj4_web_game.state) &&
            cj4_web_game.reaction_actions[player].type == CJ4_ACTION_RON)
            ron_players[ron_count++] = player;

    if (ron_count > 0)
    {
        cj4_web_store_state(cj4_do_ron_multi(
            cj4_web_game.state, ron_players, ron_count, &cj4_web_game.rules));
        return;
    }

    if (phase == CJ4_PHASE_DISCARD)
    {
        const cj4_action *best_call = NULL;
        const cj4_action *best_chi = NULL;

        for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
        {
            const cj4_action *action = &cj4_web_game.reaction_actions[player];
            if (action->type == CJ4_ACTION_CHI)
                best_chi = action;
            if (action->type != CJ4_ACTION_PON && action->type != CJ4_ACTION_MINKAN)
                continue;
            if (!best_call ||
                cj4_web_distance_from_current(action->player) <
                    cj4_web_distance_from_current(best_call->player))
                best_call = action;
        }

        if (best_call)
            cj4_web_store_state(cj4_web_apply_action(best_call));
        else if (best_chi)
            cj4_web_store_state(cj4_web_apply_action(best_chi));
        else
            cj4_web_store_state(cj4_do_pass(cj4_web_game.state, &cj4_web_game.rules));
        return;
    }

    cj4_web_store_state(cj4_do_rinshan_draw(cj4_web_game.state, &cj4_web_game.rules));
}

static uint32_t
cj4_web_continue_reactions(void)
{
    cj4_player current = cj4_state_current_player(&cj4_web_game.state);

    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        cj4_action actions[CJ4M_MAX_ACTIONS];
        uint8_t count;

        if (player == current ||
            (cj4_web_game.reaction_done_mask & (uint8_t)(1u << player)))
            continue;

        count = cj4m_collect_actions(
            &cj4_web_game.state,
            &cj4_web_game.rules,
            player,
            actions,
            CJ4M_MAX_ACTIONS);

        if (cj4_web_game.controllers[player] == CJ4_WEB_CONTROLLER_HUMAN &&
            cj4_web_has_choice(actions, count))
        {
            cj4_web_prepare_pending(CJ4_WEB_PENDING_REACTION, player, actions, count);
            return 2;
        }

        cj4_web_game.reaction_actions[player] =
            cj4_web_select_delegate_action(player, actions, count);
        cj4_web_game.reaction_done_mask |= (uint8_t)(1u << player);
    }

    cj4_web_resolve_reactions();
    return 1;
}

static const char *
cj4_web_tile_name(cj4_tile_id tile, char output[3])
{
    cj4_tile_type type = cj4_tile_get_type(tile);
    uint8_t number;
    char suit;

    if (type < 9)
    {
        number = (uint8_t)(type + 1);
        suit = 'm';
    }
    else if (type < 18)
    {
        number = (uint8_t)(type - 8);
        suit = 'p';
    }
    else if (type < 27)
    {
        number = (uint8_t)(type - 17);
        suit = 's';
    }
    else
    {
        number = (uint8_t)(type - 26);
        suit = 'z';
    }

    if (number == 5 && suit != 'z' && cj4_web_game.rules.aka_tiles[tile])
        number = 0;
    output[0] = (char)('0' + number);
    output[1] = suit;
    output[2] = '\0';
    return output;
}

static const char *
cj4_web_mjai_tile_name(cj4_tile_id tile, char output[4])
{
    static const char honors[] = "ESWNPFC";
    cj4_tile_type type = cj4_tile_get_type(tile);
    uint8_t number;
    char suit;

    if (type >= 27)
    {
        output[0] = honors[type - 27];
        output[1] = '\0';
        return output;
    }

    if (type < 9)
    {
        number = (uint8_t)(type + 1);
        suit = 'm';
    }
    else if (type < 18)
    {
        number = (uint8_t)(type - 8);
        suit = 'p';
    }
    else
    {
        number = (uint8_t)(type - 17);
        suit = 's';
    }

    output[0] = (char)('0' + number);
    output[1] = suit;
    if (number == 5 && cj4_web_game.rules.aka_tiles[tile])
    {
        output[2] = 'r';
        output[3] = '\0';
    }
    else
    {
        output[2] = '\0';
    }
    return output;
}

static void
cj4_web_mjai_append_event(cj4_web_json_writer *event)
{
    size_t needed;

    if (!event->valid)
        return;
    needed = event->length + 1;
    if (cj4_web_game.mjai_length + needed >= sizeof(cj4_web_mjai_log_buffer))
        return;

    memcpy(
        cj4_web_mjai_log_buffer + cj4_web_game.mjai_length,
        event->data,
        event->length);
    cj4_web_game.mjai_length += event->length;
    cj4_web_mjai_log_buffer[cj4_web_game.mjai_length++] = '\n';
    cj4_web_mjai_log_buffer[cj4_web_game.mjai_length] = '\0';
    cj4_web_game.mjai_event_count++;
}

static void
cj4_web_mjai_write_tile_string(cj4_web_json_writer *writer, cj4_tile_id tile)
{
    char name[4];
    cj4_web_json_append(writer, "\"%s\"", cj4_web_mjai_tile_name(tile, name));
}

static void
cj4_web_mjai_write_scores(cj4_web_json_writer *writer, const cj4_mahjong *state)
{
    cj4_web_json_append(writer, "[");
    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (player)
            cj4_web_json_append(writer, ",");
        cj4_web_json_append(writer, "%d", (int)state->scores[player]);
    }
    cj4_web_json_append(writer, "]");
}

static void
cj4_web_mjai_write_deltas(
    cj4_web_json_writer *writer,
    const cj4_mahjong *before,
    const cj4_mahjong *after)
{
    cj4_web_json_append(writer, "[");
    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (player)
            cj4_web_json_append(writer, ",");
        cj4_web_json_append(
            writer,
            "%d",
            (int)(after->scores[player] - before->scores[player]));
    }
    cj4_web_json_append(writer, "]");
}

static void
cj4_web_mjai_start_game(void)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};

    cj4_web_json_append(
        &event,
        "{\"type\":\"start_game\",\"names\":[\"P1\",\"P2\",\"P3\",\"P4\"]}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_start_kyoku(const cj4_mahjong *state)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};
    cj4_dora_indicator_list dora =
        cj4_location_collect_dora_indicators(state->locations);

    cj4_web_json_append(&event, "{\"type\":\"start_kyoku\",\"bakaze\":");
    cj4_web_mjai_write_tile_string(
        &event,
        cj4_tile_make((cj4_tile_type)(27 + state->round_wind), 0));
    cj4_web_json_append(&event, ",\"dora_marker\":");
    cj4_web_mjai_write_tile_string(&event, dora.items[0]);
    cj4_web_json_append(
        &event,
        ",\"kyoku\":%u,\"honba\":%u,\"kyotaku\":%u,\"oya\":%u,\"scores\":",
        (unsigned)(state->dealer + 1),
        (unsigned)state->honba,
        (unsigned)state->riichi_sticks,
        (unsigned)state->dealer);
    cj4_web_mjai_write_scores(&event, state);
    cj4_web_json_append(&event, ",\"tehais\":[");
    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        cj4_hand hand = cj4_location_collect_hand(state->locations, player);
        uint8_t written = 0;
        uint8_t skipped_draw = 0;

        if (player)
            cj4_web_json_append(&event, ",");
        cj4_web_json_append(&event, "[");
        for (uint8_t index = 0; index < hand.count; ++index)
        {
            if (player == state->dealer && !skipped_draw && hand.items[index] == state->draw_tile)
            {
                skipped_draw = 1;
                continue;
            }
            if (written++)
                cj4_web_json_append(&event, ",");
            cj4_web_mjai_write_tile_string(&event, hand.items[index]);
        }
        cj4_web_json_append(&event, "]");
    }
    cj4_web_json_append(&event, "]}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_tsumo(cj4_player actor, cj4_tile_id tile)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};

    cj4_web_json_append(&event, "{\"type\":\"tsumo\",\"actor\":%u,\"pai\":", (unsigned)actor);
    cj4_web_mjai_write_tile_string(&event, tile);
    cj4_web_json_append(&event, "}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_simple_actor_event(const char *type, cj4_player actor)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};

    cj4_web_json_append(
        &event,
        "{\"type\":\"%s\",\"actor\":%u}",
        type,
        (unsigned)actor);
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_dahai(cj4_player actor, const cj4_discard *discard)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};

    if (discard->is_riichi)
        cj4_web_mjai_simple_actor_event("reach", actor);
    cj4_web_json_append(&event, "{\"type\":\"dahai\",\"actor\":%u,\"pai\":", (unsigned)actor);
    cj4_web_mjai_write_tile_string(&event, discard->tile);
    cj4_web_json_append(
        &event,
        ",\"tsumogiri\":%s}",
        discard->is_tsumogiri ? "true" : "false");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_meld(cj4_player actor, const cj4_meld *meld)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};
    const char *type = meld->type == CJ4_MELD_CHI ? "chi" :
                       meld->type == CJ4_MELD_PON ? "pon" : "daiminkan";
    cj4_tile_id called = meld->tiles[meld->called_index];

    cj4_web_json_append(
        &event,
        "{\"type\":\"%s\",\"actor\":%u,\"target\":%u,\"pai\":",
        type,
        (unsigned)actor,
        (unsigned)meld->from_player);
    cj4_web_mjai_write_tile_string(&event, called);
    cj4_web_json_append(&event, ",\"consumed\":[");
    for (uint8_t index = 0, written = 0; index < meld->size; ++index)
    {
        if (index == meld->called_index)
            continue;
        if (written++)
            cj4_web_json_append(&event, ",");
        cj4_web_mjai_write_tile_string(&event, meld->tiles[index]);
    }
    cj4_web_json_append(&event, "]}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_ankan(cj4_player actor, const cj4_mahjong *state)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};

    cj4_web_json_append(&event, "{\"type\":\"ankan\",\"actor\":%u,\"consumed\":[", (unsigned)actor);
    for (uint8_t index = 0; index < CJ4_TILE_PER_TYPE; ++index)
    {
        if (index)
            cj4_web_json_append(&event, ",");
        cj4_web_mjai_write_tile_string(&event, state->pending_ankan_tiles[index]);
    }
    cj4_web_json_append(&event, "]}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_kakan(cj4_player actor, const cj4_mahjong *previous, cj4_tile_id tile)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};
    cj4_meld_list melds = cj4_location_collect_melds(previous->locations, actor);
    cj4_tile_type type = cj4_tile_get_type(tile);

    cj4_web_json_append(&event, "{\"type\":\"kakan\",\"actor\":%u,\"pai\":", (unsigned)actor);
    cj4_web_mjai_write_tile_string(&event, tile);
    cj4_web_json_append(&event, ",\"consumed\":[");
    for (uint8_t meld_index = 0; meld_index < melds.count; ++meld_index)
    {
        const cj4_meld *meld = &melds.items[meld_index];
        if (meld->type != CJ4_MELD_PON || cj4_tile_get_type(meld->tiles[0]) != type)
            continue;
        for (uint8_t index = 0; index < meld->size; ++index)
        {
            if (index)
                cj4_web_json_append(&event, ",");
            cj4_web_mjai_write_tile_string(&event, meld->tiles[index]);
        }
        break;
    }
    cj4_web_json_append(&event, "]}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_dora(cj4_tile_id tile)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};

    cj4_web_json_append(&event, "{\"type\":\"dora\",\"dora_marker\":");
    cj4_web_mjai_write_tile_string(&event, tile);
    cj4_web_json_append(&event, "}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_round_end(const cj4_mahjong *state)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event;
    cj4_round_end_type type = cj4_state_round_end_type(state);
    cj4_mahjong settled = cj4_do_settle(*state, &cj4_web_game.rules);

    if (type == CJ4_ROUND_END_TSUMO || type == CJ4_ROUND_END_RON)
    {
        for (cj4_player actor = 0; actor < CJ4_PLAYER_COUNT; ++actor)
        {
            if (!cj4_state_is_winner(state, actor))
                continue;
            event = (cj4_web_json_writer){line, sizeof(line), 0, 1};
            cj4_web_json_append(
                &event,
                "{\"type\":\"hora\",\"actor\":%u,\"target\":%u,\"pai\":",
                (unsigned)actor,
                (unsigned)(type == CJ4_ROUND_END_TSUMO
                               ? actor
                               : cj4_state_current_player(state)));
            cj4_web_mjai_write_tile_string(&event, state->winning_tile);
            cj4_web_json_append(&event, "}");
            cj4_web_mjai_append_event(&event);
        }
    }
    else
    {
        static const char *const reasons[] = {
            "", "kyushukyuhai", "sufonrenta", "suchareach", "suchakantsu", "sanchaho"};
        const char *reason = type == CJ4_ROUND_END_EXHAUSTIVE_DRAW
                                 ? "fanpai"
                                 : reasons[cj4_state_abortive_reason(state)];
        event = (cj4_web_json_writer){line, sizeof(line), 0, 1};
        cj4_web_json_append(&event, "{\"type\":\"ryukyoku\",\"reason\":\"%s\",\"deltas\":", reason);
        cj4_web_mjai_write_deltas(&event, state, &settled);
        cj4_web_json_append(&event, "}");
        cj4_web_mjai_append_event(&event);
    }

    event = (cj4_web_json_writer){line, sizeof(line), 0, 1};
    cj4_web_json_append(&event, "{\"type\":\"end_kyoku\"}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_mjai_end_game(const cj4_mahjong *state)
{
    char line[CJ4_WEB_MJAI_EVENT_CAPACITY];
    cj4_web_json_writer event = {line, sizeof(line), 0, 1};

    cj4_web_json_append(&event, "{\"type\":\"end_game\",\"scores\":");
    cj4_web_mjai_write_scores(&event, state);
    cj4_web_json_append(&event, "}");
    cj4_web_mjai_append_event(&event);
}

static void
cj4_web_record_transition(const cj4_mahjong *previous, const cj4_mahjong *next)
{
    cj4_phase previous_phase = cj4_state_phase(previous);
    cj4_phase next_phase = cj4_state_phase(next);

    if (previous_phase == CJ4_PHASE_SETTLE && next_phase == CJ4_PHASE_DRAW)
    {
        cj4_web_mjai_start_kyoku(next);
        cj4_web_mjai_tsumo(cj4_state_current_player(next), next->draw_tile);
        return;
    }

    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        cj4_discard_list before =
            cj4_location_collect_player_discards(previous->locations, player);
        cj4_discard_list after =
            cj4_location_collect_player_discards(next->locations, player);
        cj4_meld_list before_melds =
            cj4_location_collect_melds(previous->locations, player);
        cj4_meld_list after_melds =
            cj4_location_collect_melds(next->locations, player);

        if (after.count > before.count)
            cj4_web_mjai_dahai(player, &after.items[after.count - 1]);
        if (after_melds.count > before_melds.count)
        {
            const cj4_meld *meld = &after_melds.items[after_melds.count - 1];
            if (meld->type == CJ4_MELD_CHI || meld->type == CJ4_MELD_PON ||
                meld->type == CJ4_MELD_MINKAN)
                cj4_web_mjai_meld(player, meld);
        }
        if (!cj4_state_is_riichi(previous, player) && cj4_state_is_riichi(next, player))
            cj4_web_mjai_simple_actor_event("reach_accepted", player);
    }

    if (next_phase == CJ4_PHASE_ANKAN_RESOLVE &&
        previous_phase != CJ4_PHASE_ANKAN_RESOLVE)
        cj4_web_mjai_ankan(cj4_state_current_player(next), next);
    if (next_phase == CJ4_PHASE_KAKAN_RESOLVE &&
        previous_phase != CJ4_PHASE_KAKAN_RESOLVE)
        cj4_web_mjai_kakan(
            cj4_state_current_player(next), previous, next->pending_kakan_tile);

    if (next->dora_count > previous->dora_count)
    {
        cj4_dora_indicator_list dora =
            cj4_location_collect_dora_indicators(next->locations);
        for (uint8_t index = previous->dora_count;
             index < next->dora_count && index < dora.count;
             ++index)
            cj4_web_mjai_dora(dora.items[index]);
    }

    if (next_phase == CJ4_PHASE_ROUND_END && previous_phase != CJ4_PHASE_ROUND_END)
        cj4_web_mjai_round_end(next);
    else if (next_phase == CJ4_PHASE_GAME_END && previous_phase != CJ4_PHASE_GAME_END)
        cj4_web_mjai_end_game(next);
    else if (next_phase == CJ4_PHASE_DRAW && cj4_tile_id_is_valid(next->draw_tile) &&
             (previous_phase != CJ4_PHASE_DRAW || previous->draw_tile != next->draw_tile))
        cj4_web_mjai_tsumo(cj4_state_current_player(next), next->draw_tile);
}

static void
cj4_web_write_tile(cj4_web_json_writer *writer, cj4_tile_id tile)
{
    char name[3];
    cj4_web_json_append(
        writer,
        "{\"id\":%u,\"tile\":\"%s\"}",
        (unsigned)tile,
        cj4_web_tile_name(tile, name));
}

static void
cj4_web_write_hand(cj4_web_json_writer *writer, cj4_player player)
{
    cj4_hand hand = cj4_location_collect_hand(cj4_web_game.state.locations, player);
    cj4_web_json_append(writer, "[");
    for (uint8_t index = 0; index < hand.count; ++index)
    {
        if (index)
            cj4_web_json_append(writer, ",");
        cj4_web_write_tile(writer, hand.items[index]);
    }
    cj4_web_json_append(writer, "]");
}

static void
cj4_web_write_discards(cj4_web_json_writer *writer, cj4_player player)
{
    cj4_discard_list discards =
        cj4_location_collect_player_discards(cj4_web_game.state.locations, player);
    cj4_web_json_append(writer, "[");
    for (uint8_t index = 0; index < discards.count; ++index)
    {
        char name[3];
        if (index)
            cj4_web_json_append(writer, ",");
        cj4_web_json_append(
            writer,
            "{\"id\":%u,\"tile\":\"%s\",\"tsumogiri\":%s,"
            "\"riichi\":%s,\"active\":%s}",
            (unsigned)discards.items[index].tile,
            cj4_web_tile_name(discards.items[index].tile, name),
            discards.items[index].is_tsumogiri ? "true" : "false",
            discards.items[index].is_riichi ? "true" : "false",
            discards.items[index].is_active ? "true" : "false");
    }
    cj4_web_json_append(writer, "]");
}

static void
cj4_web_write_melds(cj4_web_json_writer *writer, cj4_player player)
{
    cj4_meld_list melds =
        cj4_location_collect_melds(cj4_web_game.state.locations, player);
    cj4_web_json_append(writer, "[");
    for (uint8_t index = 0; index < melds.count; ++index)
    {
        cj4_meld *meld = &melds.items[index];
        if (index)
            cj4_web_json_append(writer, ",");
        cj4_web_json_append(
            writer,
            "{\"type\":\"%s\",\"from\":%u,\"tiles\":[",
            meld->type <= CJ4_MELD_KAKAN ? cj4_web_meld_names[meld->type] : "unknown",
            (unsigned)meld->from_player);
        for (uint8_t tile = 0; tile < meld->size; ++tile)
        {
            if (tile)
                cj4_web_json_append(writer, ",");
            cj4_web_write_tile(writer, meld->tiles[tile]);
        }
        cj4_web_json_append(writer, "]}");
    }
    cj4_web_json_append(writer, "]");
}

static void
cj4_web_write_action(cj4_web_json_writer *writer, uint8_t index, const cj4_action *action)
{
    cj4_web_json_append(
        writer,
        "{\"index\":%u,\"type\":\"%s\",\"player\":%u,\"tile\":",
        (unsigned)index,
        action->type <= CJ4_ACTION_PASS ? cj4_web_action_names[action->type] : "unknown",
        (unsigned)action->player);
    if (cj4_tile_id_is_valid(action->tile))
        cj4_web_write_tile(writer, action->tile);
    else
        cj4_web_json_append(writer, "null");
    cj4_web_json_append(writer, ",\"tiles\":[");
    for (uint8_t tile = 0; tile < action->tile_count; ++tile)
    {
        if (tile)
            cj4_web_json_append(writer, ",");
        cj4_web_write_tile(writer, action->tiles[tile]);
    }
    cj4_web_json_append(writer, "]}");
}

static void
cj4_web_write_tile_array(
    cj4_web_json_writer *writer,
    const cj4_tile_id *tiles,
    uint8_t count)
{
    cj4_web_json_append(writer, "[");
    for (uint8_t index = 0; index < count; ++index)
    {
        if (index)
            cj4_web_json_append(writer, ",");
        cj4_web_write_tile(writer, tiles[index]);
    }
    cj4_web_json_append(writer, "]");
}

static void
cj4_web_write_settlement(cj4_web_json_writer *writer)
{
    const cj4_mahjong *state = &cj4_web_game.state;
    cj4_phase phase = cj4_state_phase(state);
    cj4_round_end_type type = cj4_state_round_end_type(state);
    cj4_abortive_draw_reason abortive_reason = cj4_state_abortive_reason(state);
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_mahjong settled;

    if (phase != CJ4_PHASE_ROUND_END ||
        type <= CJ4_ROUND_END_NONE || type > CJ4_ROUND_END_ABORTIVE_DRAW)
    {
        cj4_web_json_append(writer, "null");
        return;
    }

    settled = cj4_do_settle(*state, &cj4_web_game.rules);
    (void)cj4_collect_winning_results(
        state,
        &cj4_web_game.rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count);

    cj4_web_json_append(
        writer,
        "{\"type\":\"%s\",\"abortive_reason\":",
        cj4_web_round_end_names[type]);
    if (type == CJ4_ROUND_END_ABORTIVE_DRAW &&
        abortive_reason > CJ4_ABORTIVE_DRAW_NONE &&
        abortive_reason <= CJ4_ABORTIVE_DRAW_TRIPLE_RON)
    {
        cj4_web_json_append(
            writer,
            "\"%s\"",
            cj4_web_abortive_draw_names[abortive_reason]);
    }
    else
    {
        cj4_web_json_append(writer, "null");
    }

    cj4_web_json_append(writer, ",\"discarder\":");
    if (type == CJ4_ROUND_END_RON)
        cj4_web_json_append(writer, "%u", (unsigned)cj4_state_current_player(state));
    else
        cj4_web_json_append(writer, "null");

    cj4_web_json_append(writer, ",\"winning_tile\":");
    if (cj4_tile_id_is_valid(state->winning_tile))
        cj4_web_write_tile(writer, state->winning_tile);
    else
        cj4_web_json_append(writer, "null");

    cj4_web_json_append(writer, ",\"score_deltas\":[");
    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (player)
            cj4_web_json_append(writer, ",");
        cj4_web_json_append(
            writer,
            "%d",
            (int)(settled.scores[player] - state->scores[player]));
    }
    cj4_web_json_append(writer, "],\"scores_after\":[");
    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (player)
            cj4_web_json_append(writer, ",");
        cj4_web_json_append(writer, "%d", (int)settled.scores[player]);
    }

    cj4_web_json_append(writer, "],\"winners\":[");
    for (uint8_t index = 0; index < result_count; ++index)
    {
        const cj4_win_result *result = &results[index];
        if (index)
            cj4_web_json_append(writer, ",");
        cj4_web_json_append(
            writer,
            "{\"player\":%u,\"han\":%u,\"fu\":%u,\"yakuman_count\":%u,"
            "\"ron_points\":%d,\"tsumo_dealer_payment\":%d,"
            "\"tsumo_non_dealer_payment\":%d,\"dora_count\":%u,"
            "\"ura_dora_count\":%u,\"aka_dora_count\":%u,"
            "\"dora_indicators\":",
            (unsigned)result->player,
            (unsigned)result->han,
            (unsigned)result->fu,
            (unsigned)result->yakuman_count,
            (int)result->ron_points,
            (int)result->tsumo_dealer_payment,
            (int)result->tsumo_non_dealer_payment,
            (unsigned)result->dora_count,
            (unsigned)result->ura_dora_count,
            (unsigned)result->aka_dora_count);
        cj4_web_write_tile_array(
            writer,
            result->dora_indicators,
            result->dora_indicators_count);
        cj4_web_json_append(writer, ",\"ura_dora_indicators\":");
        cj4_web_write_tile_array(
            writer,
            result->ura_dora_indicators,
            result->ura_dora_indicators_count);
        cj4_web_json_append(writer, ",\"yaku\":[");
        for (uint8_t yaku_index = 0; yaku_index < result->yaku_count; ++yaku_index)
        {
            unsigned yaku = (unsigned)result->yaku[yaku_index];
            if (yaku_index)
                cj4_web_json_append(writer, ",");
            cj4_web_json_append(
                writer,
                "\"%s\"",
                yaku < sizeof(cj4_web_yaku_names) / sizeof(cj4_web_yaku_names[0])
                    ? cj4_web_yaku_names[yaku]
                    : "unknown");
        }
        cj4_web_json_append(writer, "]}");
    }
    cj4_web_json_append(writer, "]}");
}

static void
cj4_web_set_red_count(cj4_tile_type type, int32_t value)
{
    if (value < 0)
        value = 0;
    if (value > CJ4_TILE_PER_TYPE)
        value = CJ4_TILE_PER_TYPE;
    for (uint8_t index = 0; index < CJ4_TILE_PER_TYPE; ++index)
        cj4_web_configured_rules.aka_tiles[cj4_tile_make(type, index)] =
            index < (uint8_t)value;
}

CJ4_WEB_EXPORT void
cj4_web_rules_reset(void)
{
    cj4_web_configured_rules = cj4_rules_tenhou();
    cj4_web_rules_initialized = 1;
}

CJ4_WEB_EXPORT uint32_t
cj4_web_rule_set(uint32_t field_index, int32_t value)
{
    if (!cj4_web_rules_initialized)
        cj4_web_rules_reset();

    switch (field_index)
    {
    case 0: cj4_web_configured_rules.game_type = (cj4_game_type)value; break;
    case 1: cj4_web_configured_rules.initial_score = value; break;
    case 2: cj4_web_configured_rules.target_score = value; break;
    case 3: cj4_web_configured_rules.tobi_end = (uint8_t)value; break;
    case 4: cj4_web_configured_rules.target_score_excludes_riichi_sticks = (uint8_t)value; break;
    case 5: cj4_web_configured_rules.kuitan = (uint8_t)value; break;
    case 6: cj4_web_configured_rules.kuikae_forbidden = (uint8_t)value; break;
    case 7: cj4_web_configured_rules.kan_dora_timing = (cj4_kan_dora_timing)value; break;
    case 8: cj4_web_configured_rules.ippatsu = (uint8_t)value; break;
    case 9: cj4_web_set_red_count(4, value); break;
    case 10: cj4_web_set_red_count(13, value); break;
    case 11: cj4_web_set_red_count(22, value); break;
    case 12: cj4_web_configured_rules.max_ron_players = (uint8_t)value; break;
    case 13: cj4_web_configured_rules.kokushi_ron_on_ankan = (uint8_t)value; break;
    case 14: cj4_web_configured_rules.triple_ron_abortive_draw = (uint8_t)value; break;
    case 15: cj4_web_configured_rules.noten_penalty = (uint8_t)value; break;
    case 16: cj4_web_configured_rules.noten_penalty_points = value; break;
    case 17: cj4_web_configured_rules.abortive_kyuushu_kyuuhai = (uint8_t)value; break;
    case 18: cj4_web_configured_rules.abortive_suufon_renda = (uint8_t)value; break;
    case 19: cj4_web_configured_rules.abortive_four_riichi = (uint8_t)value; break;
    case 20: cj4_web_configured_rules.nagashi_mangan = (uint8_t)value; break;
    case 21: cj4_web_configured_rules.nagashi_dealer_tenpai_renchan = (uint8_t)value; break;
    case 22: cj4_web_configured_rules.kokushi_13_wait_double = (uint8_t)value; break;
    case 23: cj4_web_configured_rules.suuankou_tanki_double = (uint8_t)value; break;
    case 24: cj4_web_configured_rules.junsei_chuuren_double = (uint8_t)value; break;
    case 25: cj4_web_configured_rules.daisuushii_double = (uint8_t)value; break;
    case 26: cj4_web_configured_rules.kazoe_yakuman = (uint8_t)value; break;
    case 27: cj4_web_configured_rules.kiriage_mangan = (uint8_t)value; break;
    case 28: cj4_web_configured_rules.pao = (uint8_t)value; break;
    case 29: cj4_web_configured_rules.pao_liability_only = (uint8_t)value; break;
    case 30: cj4_web_configured_rules.pao_daisangen = (uint8_t)value; break;
    case 31: cj4_web_configured_rules.pao_daisuushii = (uint8_t)value; break;
    case 32: cj4_web_configured_rules.pao_suukantsu = (uint8_t)value; break;
    case 33: cj4_web_configured_rules.multi_ron_honba_first_only = (uint8_t)value; break;
    default: return 0;
    }
    return 1;
}

CJ4_WEB_EXPORT uint32_t
cj4_web_game_start(
    uint32_t seed,
    uint32_t wall_mode,
    uint32_t controller0,
    uint32_t controller1,
    uint32_t controller2,
    uint32_t controller3)
{
    const uint32_t controllers[CJ4_PLAYER_COUNT] = {
        controller0, controller1, controller2, controller3};
    cj4_tile_id wall[CJ4_TILE_ID_COUNT];

    if (!cj4_web_rules_initialized)
        cj4_web_rules_reset();
    if (!cj4_rules_validate(&cj4_web_configured_rules) || wall_mode > CJ4_WEB_WALL_PRESET)
        return 0;
    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
        if (controllers[player] >= CJ4_WEB_CONTROLLER_COUNT)
            return 0;

    memset(&cj4_web_game, 0, sizeof(cj4_web_game));
    cj4_web_game.active = 1;
    cj4_web_game.wall_mode = (uint8_t)wall_mode;
    cj4_web_game.rules = cj4_web_configured_rules;
    cj4_web_game.seed = seed ? seed : 1;
    cj4_web_game.random_state = cj4_web_game.seed;
    cj4_web_game.generation = 1;
    cj4_web_clear_pending();

    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        cj4_web_game.controllers[player] = (uint8_t)controllers[player];
        cj4_web_game.delegates[player] = cj4_web_make_delegate((uint8_t)controllers[player]);
    }

    cj4_web_fill_wall(wall);
    cj4_web_game.state = cj4_create_initial_state(wall, &cj4_web_game.rules);
    if (cj4_state_phase(&cj4_web_game.state) == CJ4_PHASE_GAME_END)
        return 0;

    cj4_web_mjai_log_buffer[0] = '\0';
    cj4_web_mjai_start_game();
    cj4_web_mjai_start_kyoku(&cj4_web_game.state);
    cj4_web_mjai_tsumo(
        cj4_state_current_player(&cj4_web_game.state),
        cj4_web_game.state.draw_tile);

    cj4_web_game.history[0].state = cj4_web_game.state;
    cj4_web_game.history[0].random_state = cj4_web_game.random_state;
    cj4_web_game.history[0].mjai_length = cj4_web_game.mjai_length;
    cj4_web_game.history[0].mjai_event_count = cj4_web_game.mjai_event_count;
    cj4_web_game.history_count = 1;
    cj4_web_game.history_index = 0;
    return 1;
}

CJ4_WEB_EXPORT uint32_t
cj4_web_game_set_controller(uint32_t player, uint32_t controller)
{
    uint8_t pending_kind;

    if (!cj4_web_game.active || player >= CJ4_PLAYER_COUNT ||
        controller >= CJ4_WEB_CONTROLLER_COUNT)
        return 0;

    cj4_web_game.controllers[player] = (uint8_t)controller;
    cj4_web_game.delegates[player] = cj4_web_make_delegate((uint8_t)controller);
    cj4_web_game.generation++;

    if (cj4_web_game.pending_kind == CJ4_WEB_PENDING_NONE ||
        cj4_web_game.pending_player != (cj4_player)player ||
        controller == CJ4_WEB_CONTROLLER_HUMAN)
        return 1;

    pending_kind = cj4_web_game.pending_kind;
    cj4_web_clear_pending();
    if (pending_kind == CJ4_WEB_PENDING_REACTION)
        return cj4_web_continue_reactions();
    return 1;
}

CJ4_WEB_EXPORT uint32_t
cj4_web_game_step(void)
{
    cj4_phase phase;
    cj4_player current;

    if (!cj4_web_game.active)
        return 0;
    if (cj4_web_game.pending_kind != CJ4_WEB_PENDING_NONE)
        return 2;

    phase = cj4_state_phase(&cj4_web_game.state);
    current = cj4_state_current_player(&cj4_web_game.state);

    if (phase == CJ4_PHASE_GAME_END)
        return 0;

    if (phase == CJ4_PHASE_SETTLE && cj4_can_next_round(cj4_web_game.state))
    {
        cj4_tile_id wall[CJ4_TILE_ID_COUNT];
        cj4_web_fill_wall(wall);
        cj4_web_store_state(cj4_do_next_round(
            cj4_web_game.state, wall, &cj4_web_game.rules));
        return 1;
    }

    if (phase == CJ4_PHASE_DRAW || phase == CJ4_PHASE_AFTER_CALL)
    {
        if (cj4_web_game.controllers[current] == CJ4_WEB_CONTROLLER_HUMAN)
        {
            cj4_action actions[CJ4M_MAX_ACTIONS];
            uint8_t count = cj4m_collect_actions(
                &cj4_web_game.state,
                &cj4_web_game.rules,
                current,
                actions,
                CJ4M_MAX_ACTIONS);
            cj4_web_prepare_pending(CJ4_WEB_PENDING_SINGLE, current, actions, count);
            return 2;
        }
        cj4_web_store_state(cj4m_step(
            &cj4_web_game.state, &cj4_web_game.rules, cj4_web_game.delegates));
        return 1;
    }

    if (phase == CJ4_PHASE_DISCARD || phase == CJ4_PHASE_KAKAN_RESOLVE ||
        phase == CJ4_PHASE_ANKAN_RESOLVE)
    {
        cj4_web_game.reaction_active = 1;
        cj4_web_game.reaction_done_mask = (uint8_t)(1u << current);
        memset(cj4_web_game.reaction_actions, 0, sizeof(cj4_web_game.reaction_actions));
        return cj4_web_continue_reactions();
    }

    cj4_web_store_state(cj4m_step(
        &cj4_web_game.state, &cj4_web_game.rules, cj4_web_game.delegates));
    return 1;
}

CJ4_WEB_EXPORT uint32_t
cj4_web_game_choose(uint32_t generation, uint32_t action_index)
{
    cj4_action selected;
    uint8_t pending_kind;

    if (!cj4_web_game.active || generation != cj4_web_game.generation ||
        cj4_web_game.pending_kind == CJ4_WEB_PENDING_NONE ||
        action_index >= cj4_web_game.pending_action_count)
        return 0;

    selected = cj4_web_game.pending_actions[action_index];
    pending_kind = cj4_web_game.pending_kind;
    cj4_web_clear_pending();

    if (pending_kind == CJ4_WEB_PENDING_SINGLE)
    {
        cj4_web_store_state(cj4_web_apply_action(&selected));
        return 1;
    }

    cj4_web_game.reaction_actions[selected.player] = selected;
    cj4_web_game.reaction_done_mask |= (uint8_t)(1u << selected.player);
    return cj4_web_continue_reactions();
}

CJ4_WEB_EXPORT uint32_t
cj4_web_game_rewind(uint32_t history_index)
{
    if (!cj4_web_game.active || history_index >= cj4_web_game.history_count)
        return 0;

    cj4_web_game.history_index = history_index;
    cj4_web_game.state = cj4_web_game.history[history_index].state;
    cj4_web_game.random_state = cj4_web_game.history[history_index].random_state;
    cj4_web_game.mjai_length = cj4_web_game.history[history_index].mjai_length;
    cj4_web_game.mjai_event_count =
        cj4_web_game.history[history_index].mjai_event_count;
    cj4_web_game.generation++;
    cj4_web_clear_pending();
    cj4_web_game.reaction_active = 0;
    cj4_web_game.reaction_done_mask = 0;
    return 1;
}

CJ4_WEB_EXPORT const char *
cj4_web_state_json(void)
{
    cj4_web_json_writer writer = {
        cj4_web_state_json_buffer,
        sizeof(cj4_web_state_json_buffer),
        0,
        1};

    if (!cj4_web_game.active)
    {
        (void)snprintf(
            cj4_web_state_json_buffer,
            sizeof(cj4_web_state_json_buffer),
            "{\"schema_version\":4,\"active\":false}");
        return cj4_web_state_json_buffer;
    }

    cj4_phase phase = cj4_state_phase(&cj4_web_game.state);
    uint8_t remaining = 0;
    uint8_t used = (uint8_t)(cj4_web_game.state.wall_pos +
                             cj4_web_game.state.dead_wall_draw_count);
    if (used < CJ4_TILE_ID_COUNT - 14)
        remaining = (uint8_t)(CJ4_TILE_ID_COUNT - 14 - used);

    cj4_web_json_append(
        &writer,
        "{\"schema_version\":4,\"active\":true,\"generation\":%u,"
        "\"seed\":%u,\"wall_mode\":%u,\"phase\":\"%s\","
        "\"current_player\":%u,\"dealer\":%u,\"round_wind\":%u,"
        "\"honba\":%u,\"riichi_sticks\":%u,\"remaining\":%u,"
        "\"round_end_type\":%u,\"players\":[",
        (unsigned)cj4_web_game.generation,
        (unsigned)cj4_web_game.seed,
        (unsigned)cj4_web_game.wall_mode,
        phase <= CJ4_PHASE_GAME_END ? cj4_web_phase_names[phase] : "unknown",
        (unsigned)cj4_state_current_player(&cj4_web_game.state),
        (unsigned)cj4_web_game.state.dealer,
        (unsigned)cj4_web_game.state.round_wind,
        (unsigned)cj4_web_game.state.honba,
        (unsigned)cj4_web_game.state.riichi_sticks,
        (unsigned)remaining,
        (unsigned)cj4_state_round_end_type(&cj4_web_game.state));

    for (cj4_player player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (player)
            cj4_web_json_append(&writer, ",");
        cj4_web_json_append(
            &writer,
            "{\"player\":%u,\"seat_wind\":%u,\"score\":%d,"
            "\"controller\":%u,\"riichi\":%s,\"hand\":",
            (unsigned)player,
            (unsigned)((player + CJ4_PLAYER_COUNT - cj4_web_game.state.dealer) %
                       CJ4_PLAYER_COUNT),
            (int)cj4_web_game.state.scores[player],
            (unsigned)cj4_web_game.controllers[player],
            cj4_state_is_riichi(&cj4_web_game.state, player) ? "true" : "false");
        cj4_web_write_hand(&writer, player);
        cj4_web_json_append(&writer, ",\"discards\":");
        cj4_web_write_discards(&writer, player);
        cj4_web_json_append(&writer, ",\"melds\":");
        cj4_web_write_melds(&writer, player);
        cj4_web_json_append(&writer, "}");
    }

    cj4_web_json_append(&writer, "],\"dora_indicators\":[");
    cj4_dora_indicator_list dora =
        cj4_location_collect_dora_indicators(cj4_web_game.state.locations);
    uint8_t visible_dora_count = cj4_web_game.state.dora_count;
    if (visible_dora_count > dora.count)
        visible_dora_count = dora.count;
    for (uint8_t index = 0; index < visible_dora_count; ++index)
    {
        if (index)
            cj4_web_json_append(&writer, ",");
        cj4_web_write_tile(&writer, dora.items[index]);
    }

    cj4_web_json_append(
        &writer,
        "],\"history\":{\"index\":%u,\"count\":%u},"
        "\"mjai\":{\"event_count\":%u,\"format\":\"jsonl\"},"
        "\"waiting_for_input\":%s,\"pending_player\":",
        (unsigned)cj4_web_game.history_index,
        (unsigned)cj4_web_game.history_count,
        (unsigned)cj4_web_game.mjai_event_count,
        cj4_web_game.pending_kind != CJ4_WEB_PENDING_NONE ? "true" : "false");
    if (cj4_web_game.pending_kind != CJ4_WEB_PENDING_NONE)
        cj4_web_json_append(&writer, "%u", (unsigned)cj4_web_game.pending_player);
    else
        cj4_web_json_append(&writer, "null");

    cj4_web_json_append(&writer, ",\"legal_actions\":[");
    for (uint8_t index = 0; index < cj4_web_game.pending_action_count; ++index)
    {
        if (index)
            cj4_web_json_append(&writer, ",");
        cj4_web_write_action(&writer, index, &cj4_web_game.pending_actions[index]);
    }
    cj4_web_json_append(&writer, "],\"settlement\":");
    cj4_web_write_settlement(&writer);
    cj4_web_json_append(&writer, "}");

    if (!writer.valid)
        (void)snprintf(
            cj4_web_state_json_buffer,
            sizeof(cj4_web_state_json_buffer),
            "{\"schema_version\":4,\"active\":false,\"error\":\"snapshot_too_large\"}");
    return cj4_web_state_json_buffer;
}

CJ4_WEB_EXPORT const char *
cj4_web_mjai_log_jsonl(void)
{
    size_t length = cj4_web_game.active ? cj4_web_game.mjai_length : 0;

    if (length >= sizeof(cj4_web_mjai_output_buffer))
        length = sizeof(cj4_web_mjai_output_buffer) - 1;
    if (length)
        memcpy(cj4_web_mjai_output_buffer, cj4_web_mjai_log_buffer, length);
    cj4_web_mjai_output_buffer[length] = '\0';
    return cj4_web_mjai_output_buffer;
}
