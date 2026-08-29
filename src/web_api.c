#include <cjong4/web/api.h>

#include <cjong4/core/rules.h>
#include <cjong4/core/tile_const.h>
#include <cjong4/opponent/opponent_betaori.h>
#include <cjong4/opponent/opponent_chanta.h>
#include <cjong4/opponent/opponent_chiitoi.h>
#include <cjong4/opponent/opponent_kokushi.h>
#include <cjong4/opponent/opponent_pinfu.h>
#include <cjong4/opponent/opponent_somete.h>
#include <cjong4/opponent/opponent_tanyao.h>
#include <cjong4/opponent/opponent_toitoi.h>

#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define CJ4_WEB_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define CJ4_WEB_EXPORT
#endif

static int
cj4_web_opponents_ready(void)
{
    const cj4m_player_delegate opponents[] = {
        cj4_opponent_betaori(1), cj4_opponent_chanta(1), cj4_opponent_chiitoi(1),
        cj4_opponent_kokushi(1), cj4_opponent_pinfu(1), cj4_opponent_somete(1),
        cj4_opponent_tanyao(1), cj4_opponent_toitoi(1)};

    for (size_t i = 0; i < sizeof(opponents) / sizeof(opponents[0]); ++i)
    {
        if (opponents[i].decide == NULL)
            return 0;
    }
    return 1;
}

static unsigned
cj4_web_red_tile_count(const cj4_rules *rules, cj4_tile_type type)
{
    unsigned count = 0;

    for (uint8_t index = 0; index < CJ4_TILE_PER_TYPE; ++index)
    {
        if (rules->aka_tiles[cj4_tile_make(type, index)])
            ++count;
    }
    return count;
}

CJ4_WEB_EXPORT uint32_t
cj4_web_api_version(void)
{
    return CJ4_WEB_API_VERSION;
}

CJ4_WEB_EXPORT const char *
cj4_web_bootstrap_json(void)
{
    static char json[4096];
    const cj4_rules rules = cj4_rules_tenhou();
    const int ready = cj4_rules_validate(&rules) && cj4_web_opponents_ready();

    (void)snprintf(
        json, sizeof(json),
        "{\"schema_version\":1,\"engine\":{\"ready\":%s,\"api_version\":%u},"
        "\"rules\":{\"preset\":\"tenhou\",\"version\":%u,\"initial_score\":%d,"
        "\"target_score\":%d,\"game_type\":%d,\"tobi_end\":%s,\"kuitan\":%s,"
        "\"kuikae_forbidden\":%s,\"kan_dora_timing\":%d,\"ippatsu\":%s,"
        "\"max_ron_players\":%u,\"kokushi_ron_on_ankan\":%s,"
        "\"triple_ron_abortive_draw\":%s,\"noten_penalty\":%s,"
        "\"noten_penalty_points\":%d,\"abortive_kyuushu_kyuuhai\":%s,"
        "\"abortive_suufon_renda\":%s,\"abortive_four_riichi\":%s,"
        "\"nagashi_mangan\":%s,\"kokushi_13_wait_double\":%s,"
        "\"suuankou_tanki_double\":%s,\"junsei_chuuren_double\":%s,"
        "\"daisuushii_double\":%s,\"kazoe_yakuman\":%s,\"kiriage_mangan\":%s,"
        "\"pao\":%s,\"pao_liability_only\":%s,\"pao_daisangen\":%s,"
        "\"pao_daisuushii\":%s,\"pao_suukantsu\":%s,"
        "\"multi_ron_honba_first_only\":%s,\"nagashi_dealer_tenpai_renchan\":%s,"
        "\"target_score_excludes_riichi_sticks\":%s,\"aka_5m\":%u,\"aka_5p\":%u,"
        "\"aka_5s\":%u},\"opponents\":[\"betaori\",\"chanta\",\"chiitoi\","
        "\"kokushi\",\"pinfu\",\"somete\",\"tanyao\",\"toitoi\"]}",
        ready ? "true" : "false", (unsigned)CJ4_WEB_API_VERSION, (unsigned)rules.version,
        (int)rules.initial_score, (int)rules.target_score, (int)rules.game_type,
        rules.tobi_end ? "true" : "false", rules.kuitan ? "true" : "false",
        rules.kuikae_forbidden ? "true" : "false", (int)rules.kan_dora_timing,
        rules.ippatsu ? "true" : "false", (unsigned)rules.max_ron_players,
        rules.kokushi_ron_on_ankan ? "true" : "false",
        rules.triple_ron_abortive_draw ? "true" : "false",
        rules.noten_penalty ? "true" : "false", (int)rules.noten_penalty_points,
        rules.abortive_kyuushu_kyuuhai ? "true" : "false",
        rules.abortive_suufon_renda ? "true" : "false",
        rules.abortive_four_riichi ? "true" : "false",
        rules.nagashi_mangan ? "true" : "false",
        rules.kokushi_13_wait_double ? "true" : "false",
        rules.suuankou_tanki_double ? "true" : "false",
        rules.junsei_chuuren_double ? "true" : "false",
        rules.daisuushii_double ? "true" : "false",
        rules.kazoe_yakuman ? "true" : "false", rules.kiriage_mangan ? "true" : "false",
        rules.pao ? "true" : "false", rules.pao_liability_only ? "true" : "false",
        rules.pao_daisangen ? "true" : "false", rules.pao_daisuushii ? "true" : "false",
        rules.pao_suukantsu ? "true" : "false",
        rules.multi_ron_honba_first_only ? "true" : "false",
        rules.nagashi_dealer_tenpai_renchan ? "true" : "false",
        rules.target_score_excludes_riichi_sticks ? "true" : "false",
        cj4_web_red_tile_count(&rules, CJ4_TILE_TYPE_5M),
        cj4_web_red_tile_count(&rules, CJ4_TILE_TYPE_5P),
        cj4_web_red_tile_count(&rules, CJ4_TILE_TYPE_5S));
    return json;
}
