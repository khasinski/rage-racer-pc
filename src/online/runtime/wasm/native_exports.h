#ifndef RAGE_ONLINE_NATIVE_EXPORTS_H
#define RAGE_ONLINE_NATIVE_EXPORTS_H
#ifdef _WIN32
#define ONLINE_EXPORT __declspec(dllexport)
#else
#define ONLINE_EXPORT __attribute__((visibility("default")))
#endif
#endif
