#include <cjong4/web/api.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

int
main(void)
{
    const char *json = cj4_web_bootstrap_json();

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

    puts("cjong4-web API tests passed");
    return 0;
}
