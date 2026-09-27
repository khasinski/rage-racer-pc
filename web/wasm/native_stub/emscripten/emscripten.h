/* Native stand-in so rage_web.c can also be built without Emscripten, for
 * the physics parity check in web/wasm/parity_main.c. */
#ifndef RAGE_WEB_NATIVE_EMSCRIPTEN_STUB_H
#define RAGE_WEB_NATIVE_EMSCRIPTEN_STUB_H
#define EMSCRIPTEN_KEEPALIVE
#endif
