#ifndef RAGE_USER_SETTINGS_H
#define RAGE_USER_SETTINGS_H
#include "port_config.h"
#include "session_options.h"

/* Atomic replacement. Audio remains part of the save game; HD packs keep
 * their own store. A failed write preserves the previous preferences. */
int UserSettingsSave(const char *path, const RagePortConfig *config,
                     const PortSessionOptions *session);
#endif
