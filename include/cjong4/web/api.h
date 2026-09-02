#ifndef CJ4_WEB_API_H
#define CJ4_WEB_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        CJ4_WEB_API_VERSION = 4
    };

    typedef enum
    {
        CJ4_WEB_CONTROLLER_HUMAN = 0,
        CJ4_WEB_CONTROLLER_BETAORI,
        CJ4_WEB_CONTROLLER_CHANTA,
        CJ4_WEB_CONTROLLER_CHIITOI,
        CJ4_WEB_CONTROLLER_KOKUSHI,
        CJ4_WEB_CONTROLLER_PINFU,
        CJ4_WEB_CONTROLLER_SOMETE,
        CJ4_WEB_CONTROLLER_TANYAO,
        CJ4_WEB_CONTROLLER_TOITOI,
        CJ4_WEB_CONTROLLER_COUNT
    } cj4_web_controller;

    /* Returns the version of the browser-facing C API. */
    uint32_t
    cj4_web_api_version(void);

    /*
     * Returns a read-only, null-terminated JSON string owned by the library.
     * The pointer remains valid until the next call to this function.
     */
    const char *
    cj4_web_bootstrap_json(void);

    /* Restores the configurable rules to the built-in default. */
    void
    cj4_web_rules_reset(void);

    /* Sets one rule by the browser schema field index. */
    uint32_t
    cj4_web_rule_set(uint32_t field_index, int32_t value);

    /* Starts a new game. wall_mode: 0=random, 1=tile ID order, 2-7=presets. */
    uint32_t
    cj4_web_game_start(
        uint32_t seed,
        uint32_t wall_mode,
        uint32_t controller0,
        uint32_t controller1,
        uint32_t controller2,
        uint32_t controller3);

    /* Changes one seat between human and opponent during an active game. */
    uint32_t
    cj4_web_game_set_controller(uint32_t player, uint32_t controller);

    /* Advances one resolved engine transition, or prepares human input. */
    uint32_t
    cj4_web_game_step(void);

    /* Chooses one of the legal actions in the current state JSON. */
    uint32_t
    cj4_web_game_choose(uint32_t generation, uint32_t action_index);

    /* Selects a retained history snapshot without discarding later snapshots. */
    uint32_t
    cj4_web_game_rewind(uint32_t history_index);

    /* Returns the current versioned UI snapshot as library-owned JSON. */
    const char *
    cj4_web_state_json(void);

    /* Returns the retained match history as MJAI JSON Lines. */
    const char *
    cj4_web_mjai_log_jsonl(void);

#ifdef __cplusplus
}
#endif

#endif
