#ifndef RAGE_ONLINE_TRANSPORT_H
#define RAGE_ONLINE_TRANSPORT_H
#include <stddef.h>
#include <stdint.h>
typedef struct OnlineTransport OnlineTransport;
enum { ONLINE_TEXT=1, ONLINE_FRAME, ONLINE_AUTH, ONLINE_ERROR, ONLINE_CLOSED, ONLINE_REPLACED };
/* Worker owns TLS, HTTP authentication and RFC6455 framing. No game or GPU
 * state is touched off-thread. The caller owns the queue consumer. */
OnlineTransport *OnlineTransportStart(const char *base,const char *action,const char *json);
int OnlineTransportSend(OnlineTransport *t,int binary,const void *bytes,size_t size);
int OnlineTransportPoll(OnlineTransport *t,int *type,void *out,size_t capacity,size_t *size);
void OnlineTransportStop(OnlineTransport *t);
int OnlineTransportDone(OnlineTransport *t);
void OnlineTransportFree(OnlineTransport *t);
void OnlineTransportUseRtc(OnlineTransport *t);
int OnlineTransportFast(OnlineTransport *t);
/* Protocol helpers are independent of sockets and have RFC test vectors. */
void OnlineWebSocketAccept(const char *key,char out[29]);
#endif
