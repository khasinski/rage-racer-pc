/* How each human seat's car looks (web_looks.c): paint, team logo, the name
 * on the windscreen. Presentation only: neither the server's simulation nor
 * the wire format knows about it. */
#ifndef WEB_LOOKS_H
#define WEB_LOOKS_H
#include <stdint.h>
#include "car_custom.h"
#include "game/race_sim.h"

typedef struct WebSeatLook {
    int paint[2]; /* two catalogue colours, or -1 for the car's factory ones */
    CarCustom custom; /* logo and name */
} WebSeatLook;

/* Forgets the looks set for the next race (a new disc). */
void WebLooksClear(void);
/* Hands over the looks set for the next race and forgets them: a race
 * prepared uses them up whether or not it starts. */
void WebLooksTake(WebSeatLook looks[DRIVER_SEAT_LIMIT]);
/* The race being prepared reads its seats' logos and names from here from
 * now on (the old race no longer does). */
void WebLooksUseInRace(const WebSeatLook looks[DRIVER_SEAT_LIMIT]);
/* The running race's logo and name for a seat (seat 0 is the garage
 * preview's car), and its content hash (0 for none). */
const CarCustom *WebRaceCustom(int seat);
uint32_t WebRaceCustomHash(uint32_t seat);
#endif
