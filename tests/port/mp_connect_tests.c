#include "port/mp_client.h"
#include <stdio.h>

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; \
} } while (0)

int main(void) {
    /* A connection nobody is listening on fails cleanly rather than hanging
     * or crashing; this is the one behavior narrow-testable without a real
     * running server. */
    CHECK(MpClientConnect(NULL, 7878) == NULL);
    CHECK(MpClientConnect("127.0.0.1", 0) == NULL);
    /* Invalid numeric addresses also exercise socket/startup cleanup. */
    for (int attempt = 0; attempt < 3; ++attempt) {
        CHECK(MpClientConnect("999.1.1.1", 7878) == NULL);
        CHECK(MpClientConnect("", 7878) == NULL);
    }
    MpClient *client = MpClientConnect("127.0.0.1", 1);
    CHECK(client == NULL);
    MpClientClose(NULL);

    puts("mp_connect: invalid settings and failed connection cleanup pass");
    return 0;
}
