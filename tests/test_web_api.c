#include <cjong4/web/api.h>
#include <cjong4/mjai.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
assert_supported_mjai_lines_decode(const char *jsonl)
{
    const char *line = jsonl;

    while (*line)
    {
        const char *end = strchr(line, '\n');
        size_t length = end ? (size_t)(end - line + 1) : strlen(line);
        cj4_mjai_event event;

        if (strstr(line, "\"type\":\"start_game\"") != line + 1 &&
            strstr(line, "\"type\":\"end_game\"") != line + 1)
            assert(cj4_mjai_decode_line(line, length, &event) == CJ4_MJAI_OK);
        if (!end)
            break;
        line = end + 1;
    }
}

static unsigned
hand_tile_count(const char *json, unsigned player, const char *tile)
{
    char player_marker[32];
    char tile_marker[32];
    const char *players = strstr(json, "\"players\":[");
    const char *hand;
    const char *hand_end;
    const char *cursor;
    unsigned count = 0;

    assert(players != NULL);
    (void)snprintf(player_marker, sizeof(player_marker), "{\"player\":%u,", player);
    hand = strstr(players, player_marker);
    assert(hand != NULL);
    hand = strstr(hand, "\"hand\":[");
    assert(hand != NULL);
    hand_end = strstr(hand, "],\"discards\":");
    assert(hand_end != NULL);

    (void)snprintf(tile_marker, sizeof(tile_marker), "\"tile\":\"%s\"", tile);
    cursor = hand;
    while ((cursor = strstr(cursor, tile_marker)) != NULL && cursor < hand_end)
    {
        count++;
        cursor += strlen(tile_marker);
    }
    return count;
}

static unsigned
state_generation(const char *json)
{
    const char *generation = strstr(json, "\"generation\":");
    unsigned value = 0;

    assert(generation != NULL);
    assert(sscanf(generation, "\"generation\":%u", &value) == 1);
    return value;
}

static unsigned
action_index(
    const char *json,
    const char *type,
    unsigned player,
    unsigned tile_id)
{
    char action_marker[96];
    const char *actions = strstr(json, "\"legal_actions\":[");
    const char *match;
    const char *index;
    const char *next;
    unsigned value = 0;

    assert(actions != NULL);
    (void)snprintf(
        action_marker,
        sizeof(action_marker),
        "\"type\":\"%s\",\"player\":%u,\"tile\":{\"id\":%u,",
        type,
        player,
        tile_id);
    match = strstr(actions, action_marker);
    assert(match != NULL);

    index = strstr(actions, "{\"index\":");
    assert(index != NULL && index < match);
    while ((next = strstr(index + 1, "{\"index\":")) != NULL && next < match)
        index = next;
    assert(sscanf(index, "{\"index\":%u", &value) == 1);
    return value;
}

int
main(void)
{
    const char *json = cj4_web_bootstrap_json();
    const char *mjai;
    char initial_mjai[8192];
    unsigned steps = 0;

    assert(cj4_web_api_version() == CJ4_WEB_API_VERSION);
    assert(json != NULL);
    assert(strstr(json, "\"schema_version\":1") != NULL);
    assert(strstr(json, "\"ready\":true") != NULL);
    assert(strstr(json, "\"preset\":\"tenhou\"") != NULL);
    assert(strstr(json, "\"kan_dora_timing\":1") != NULL);
    assert(strstr(json, "\"triple_ron_abortive_draw\":true") != NULL);
    assert(strstr(json, "\"kiriage_mangan\":false") != NULL);
    assert(strstr(json, "\"target_score_excludes_riichi_sticks\":true") != NULL);
    assert(strstr(json, "\"aka_5m\":1") != NULL);
    assert(strstr(json, "\"aka_5p\":1") != NULL);
    assert(strstr(json, "\"aka_5s\":1") != NULL);
    assert(strstr(json, "\"betaori\"") != NULL);
    assert(strstr(json, "\"toitoi\"") != NULL);

    cj4_web_rules_reset();
    assert(cj4_web_rule_set(0, 0) == 1);
    assert(cj4_web_game_start(
               12345,
               0,
               CJ4_WEB_CONTROLLER_BETAORI,
               CJ4_WEB_CONTROLLER_CHANTA,
               CJ4_WEB_CONTROLLER_PINFU,
               CJ4_WEB_CONTROLLER_TOITOI) == 1);

    json = cj4_web_state_json();
    assert(strstr(json, "\"schema_version\":5") != NULL);
    assert(strstr(json, "\"active\":true") != NULL);
    assert(strstr(json, "\"seed\":12345") != NULL);
    assert(strstr(json, "\"history\":{\"index\":0,\"count\":1}") != NULL);
    assert(strstr(json, "\"players\":[") != NULL);
    assert(strstr(json, "\"draw_tile\":{") != NULL);
    assert(strstr(json, "\"mjai\":{\"event_count\":3,\"format\":\"jsonl\"}") != NULL);
    mjai = cj4_web_mjai_log_jsonl();
    assert(strstr(mjai, "{\"type\":\"start_game\"") != NULL);
    assert(strstr(mjai, "{\"type\":\"start_kyoku\"") != NULL);
    assert(strstr(mjai, "{\"type\":\"tsumo\",\"actor\":0") != NULL);
    assert(strstr(mjai, "\"tehais\":[[") != NULL);
    assert(strstr(mjai, "request_id") == NULL);
    assert_supported_mjai_lines_decode(mjai);
    assert(strlen(mjai) < sizeof(initial_mjai));
    strcpy(initial_mjai, mjai);
    {
        const char *dora = strstr(json, "\"dora_indicators\":[");
        const char *dora_end = dora ? strstr(dora, "],\"history\"") : NULL;
        const char *first_id = dora ? strstr(dora, "\"id\"") : NULL;
        const char *second_id = first_id ? strstr(first_id + 1, "\"id\"") : NULL;

        assert(dora != NULL);
        assert(dora_end != NULL);
        assert(first_id != NULL && first_id < dora_end);
        assert(second_id == NULL || second_id > dora_end);
    }

    {
        int saw_round_settlement = 0;
        int saw_winning_settlement = 0;

        for (; steps < 12000; ++steps)
        {
            json = cj4_web_state_json();
            if (strstr(json, "\"phase\":\"game_end\"") != NULL)
                break;
            if (strstr(json, "\"phase\":\"round_end\"") != NULL)
            {
                saw_round_settlement = 1;
                assert(strstr(json, "\"settlement\":{") != NULL);
                assert(strstr(json, "\"score_deltas\":[") != NULL);
                assert(strstr(json, "\"scores_after\":[") != NULL);
                if (strstr(json, "\"type\":\"tsumo\"") != NULL ||
                    strstr(json, "\"type\":\"ron\"") != NULL)
                {
                    saw_winning_settlement = 1;
                    assert(strstr(json, "\"winners\":[{") != NULL);
                    assert(strstr(json, "\"han\":") != NULL);
                    assert(strstr(json, "\"fu\":") != NULL);
                    assert(strstr(json, "\"yaku\":[\"") != NULL);
                    assert(strstr(json, "\"dora_indicators\":[{") != NULL);
                    assert(strstr(json, "\"ura_dora_indicators\":[") != NULL);
                }
            }
            assert(cj4_web_game_step() == 1);
        }
        assert(saw_round_settlement);
        assert(saw_winning_settlement);
    }
    assert(steps < 12000);
    assert(strstr(cj4_web_state_json(), "\"phase\":\"game_end\"") != NULL);
    mjai = cj4_web_mjai_log_jsonl();
    assert(strstr(mjai, "{\"type\":\"dahai\"") != NULL);
    assert(strstr(mjai, "{\"type\":\"end_kyoku\"}") != NULL);
    assert(strstr(mjai, "{\"type\":\"hora\"") != NULL);
    assert(strstr(mjai, "{\"type\":\"end_game\"") != NULL);
    assert_supported_mjai_lines_decode(mjai);
    assert(cj4_web_game_rewind(0) == 1);
    assert(strstr(
               cj4_web_state_json(),
               "\"history\":{\"index\":0,") != NULL);
    assert(strcmp(cj4_web_mjai_log_jsonl(), initial_mjai) == 0);

    assert(cj4_web_game_step() == 1);
    assert(strstr(cj4_web_mjai_log_jsonl(), "{\"type\":\"dahai\"") != NULL);

    cj4_web_rules_reset();
    assert(cj4_web_rule_set(0, 0) == 1);
    assert(cj4_web_game_start(
               12345,
               0,
               CJ4_WEB_CONTROLLER_BETAORI,
               CJ4_WEB_CONTROLLER_CHANTA,
               CJ4_WEB_CONTROLLER_PINFU,
               CJ4_WEB_CONTROLLER_TOITOI) == 1);
    assert(strcmp(cj4_web_mjai_log_jsonl(), initial_mjai) == 0);

    cj4_web_rules_reset();
    assert(cj4_web_game_start(
               67890,
               0,
               CJ4_WEB_CONTROLLER_HUMAN,
               CJ4_WEB_CONTROLLER_BETAORI,
               CJ4_WEB_CONTROLLER_BETAORI,
               CJ4_WEB_CONTROLLER_BETAORI) == 1);
    assert(cj4_web_game_step() == 2);
    json = cj4_web_state_json();
    assert(strstr(json, "\"waiting_for_input\":true") != NULL);
    assert(strstr(json, "\"pending_player\":0") != NULL);
    assert(strstr(json, "\"legal_actions\":[{") != NULL);
    assert(cj4_web_game_choose(999, 0) == 0);
    assert(cj4_web_game_set_controller(0, CJ4_WEB_CONTROLLER_BETAORI) == 1);
    assert(strstr(cj4_web_state_json(), "\"waiting_for_input\":false") != NULL);
    assert(cj4_web_game_set_controller(0, CJ4_WEB_CONTROLLER_HUMAN) == 1);
    assert(cj4_web_game_step() == 2);
    assert(strstr(cj4_web_state_json(), "\"waiting_for_input\":true") != NULL);
    assert(cj4_web_game_choose(3, 0) == 1);
    assert(strstr(cj4_web_state_json(), "\"generation\":4") != NULL);

    cj4_web_rules_reset();
    assert(cj4_web_game_start(
               1,
               2,
               CJ4_WEB_CONTROLLER_HUMAN,
               CJ4_WEB_CONTROLLER_HUMAN,
               CJ4_WEB_CONTROLLER_HUMAN,
               CJ4_WEB_CONTROLLER_HUMAN) == 1);
    json = cj4_web_state_json();
    assert(strstr(json, "\"wall_mode\":2") != NULL);
    assert(strstr(json, "\"draw_tile\":{\"id\":120,\"tile\":\"4z\"}") != NULL);

    assert(hand_tile_count(json, 0, "1m") == 3);
    assert(hand_tile_count(json, 0, "2m") == 1);
    assert(hand_tile_count(json, 0, "3m") == 1);
    assert(hand_tile_count(json, 0, "4m") == 1);
    assert(hand_tile_count(json, 0, "5m") == 1);
    assert(hand_tile_count(json, 0, "6m") == 1);
    assert(hand_tile_count(json, 0, "7m") == 1);
    assert(hand_tile_count(json, 0, "8m") == 1);
    assert(hand_tile_count(json, 0, "9m") == 3);
    assert(hand_tile_count(json, 0, "4z") == 1);

    assert(hand_tile_count(json, 1, "1m") == 1);
    assert(hand_tile_count(json, 1, "9m") == 1);
    assert(hand_tile_count(json, 1, "1p") == 1);
    assert(hand_tile_count(json, 1, "9p") == 1);
    assert(hand_tile_count(json, 1, "1s") == 1);
    assert(hand_tile_count(json, 1, "9s") == 1);
    for (unsigned honor = 1; honor <= 7; ++honor)
    {
        char tile[3] = {(char)('0' + honor), 'z', '\0'};
        assert(hand_tile_count(json, 1, tile) == 1);
    }

    assert(hand_tile_count(json, 2, "1p") == 3);
    assert(hand_tile_count(json, 2, "1z") == 3);
    assert(hand_tile_count(json, 2, "2z") == 3);
    assert(hand_tile_count(json, 2, "3z") == 3);
    assert(hand_tile_count(json, 2, "4z") == 1);

    assert(hand_tile_count(json, 3, "2p") == 3);
    assert(hand_tile_count(json, 3, "4z") == 1);
    assert(hand_tile_count(json, 3, "5z") == 3);
    assert(hand_tile_count(json, 3, "6z") == 3);
    assert(hand_tile_count(json, 3, "7z") == 3);

    assert(cj4_web_game_step() == 2);
    json = cj4_web_state_json();
    assert(cj4_web_game_choose(
               state_generation(json),
               action_index(json, "discard", 0, 120)) == 1);
    assert(cj4_web_game_step() == 2);
    for (unsigned player = 1; player < 4; ++player)
    {
        uint32_t result;

        json = cj4_web_state_json();
        result = cj4_web_game_choose(
            state_generation(json),
            action_index(json, "ron", player, 120));
        assert(result == (player == 3 ? 1u : 2u));
    }
    json = cj4_web_state_json();
    assert(strstr(json, "\"phase\":\"round_end\"") != NULL);
    assert(strstr(json, "\"abortive_reason\":\"triple_ron\"") != NULL);

    puts("cjong4-web API tests passed");
    return 0;
}
