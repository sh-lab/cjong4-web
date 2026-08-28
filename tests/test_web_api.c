#include <cjong4/web/api.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

int
main(void)
{
    const char *json = cj4_web_bootstrap_json();
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
    assert(strstr(json, "\"schema_version\":2") != NULL);
    assert(strstr(json, "\"active\":true") != NULL);
    assert(strstr(json, "\"seed\":12345") != NULL);
    assert(strstr(json, "\"history\":{\"index\":0,\"count\":1}") != NULL);
    assert(strstr(json, "\"players\":[") != NULL);
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

    for (; steps < 12000; ++steps)
    {
        json = cj4_web_state_json();
        if (strstr(json, "\"phase\":\"game_end\"") != NULL)
            break;
        assert(cj4_web_game_step() == 1);
    }
    assert(steps < 12000);
    assert(strstr(cj4_web_state_json(), "\"phase\":\"game_end\"") != NULL);
    assert(cj4_web_game_rewind(0) == 1);
    assert(strstr(
               cj4_web_state_json(),
               "\"history\":{\"index\":0,") != NULL);

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
