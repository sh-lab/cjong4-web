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
    assert(strstr(json, "\"schema_version\":4") != NULL);
    assert(strstr(json, "\"active\":true") != NULL);
    assert(strstr(json, "\"seed\":12345") != NULL);
    assert(strstr(json, "\"history\":{\"index\":0,\"count\":1}") != NULL);
    assert(strstr(json, "\"players\":[") != NULL);
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

    puts("cjong4-web API tests passed");
    return 0;
}
