#ifndef CONTENT_SHA256_H
#define CONTENT_SHA256_H
#include <stddef.h>
#include <stdint.h>

/* Content identity, not an authentication/signature API. FIPS 180-4 SHA-256. */
typedef struct Sha256 {
    uint32_t state[8];
    uint64_t bytes;
    unsigned char tail[64];
    size_t used;
} Sha256;
void Sha256Init(Sha256 *hash);
void Sha256Update(Sha256 *hash,const void *bytes,size_t size);
/* Consumes the state and writes 64 lowercase hex digits plus terminator. */
void Sha256Finish(Sha256 *hash,char hex[65]);
/* Failure preserves output; limits prevent hashing unbounded generated files. */
int Sha256File(const char *path,size_t limit,char hex[65]);
#endif
