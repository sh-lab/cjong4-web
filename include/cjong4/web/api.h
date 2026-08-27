#ifndef CJ4_WEB_API_H
#define CJ4_WEB_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        CJ4_WEB_API_VERSION = 1
    };

    /* Returns the version of the browser-facing C API. */
    uint32_t
    cj4_web_api_version(void);

    /*
     * Returns a read-only, null-terminated JSON string owned by the library.
     * The pointer remains valid until the next call to this function.
     */
    const char *
    cj4_web_bootstrap_json(void);

#ifdef __cplusplus
}
#endif

#endif
