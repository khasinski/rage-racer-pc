#ifndef RAGE_ONLINE_MODEL_H
#define RAGE_ONLINE_MODEL_H
#include <stddef.h>
#include <stdint.h>
enum { ONLINE_ROWS=20, ONLINE_ROOMS=64, ONLINE_PLAYERS=12, ONLINE_MEMBERS=24, ONLINE_CHAT_LINES=32 };
typedef enum OnlineAction {
    OA_NONE,OA_BACK,OA_SERVER,OA_NAME,OA_PASSWORD,OA_GUEST,OA_LOGIN,OA_REGISTER,
    OA_QUICK_ROOM,OA_DUEL,OA_JOIN,OA_COURSE,OA_CLASS,OA_LAPS,OA_REVERSE,OA_RIVALS,
    OA_CAR,OA_GEARBOX,OA_TIRE,OA_READY,OA_START,OA_LEAVE_ROOM,OA_LEAVE_RACE,
    OA_RESUME,OA_PLAYERS,OA_WATCH,OA_CHAT,OA_NEXT_ROOMS,OA_SETTINGS,
    OA_JOIN_DUEL,OA_COPY_INVITE,OA_SPECTATOR,OA_MAX_PLAYERS,
    OA_ARCADE,OA_LEAVE_ARCADE,OA_ARCADE_LOBBY,OA_ARCADE_CAR,OA_ARCADE_VOTE,OA_OPEN_CHAT,OA_CHAT_SCROLL,
    OA_PROFILE_OPTIONS,OA_ACCOUNT,OA_CONNECTION
} OnlineAction;
typedef enum OnlineScreen { OS_PROFILE,OS_CONNECTING,OS_ROOMS,OS_ROOM,OS_RACE,OS_RESULTS,OS_PLAYERS,OS_ARCADE,OS_CHAT,OS_PROFILE_OPTIONS,OS_ACCOUNT,OS_CONNECTION } OnlineScreen;
typedef struct OnlineRow {char label[96],value[256];OnlineAction action;int argument,enabled,editable;} OnlineRow;
typedef struct OnlineView {
    OnlineScreen screen;int active,racing,panel,selected,count,editing,localSeat,userId;
    int raceReady,finishing;
    char title[96],subtitle[160],status[256];OnlineRow rows[ONLINE_ROWS];
    int32_t hud[16];
    int arcade,arcadeEpoch,nextRaceSeconds,nextCourse,nextClass,lateJoin,takeoverSeconds;
    int fastTransport,chatCount,chatOffset;
    char messages[ONLINE_CHAT_LINES][256];
} OnlineView;
static inline int OnlineViewJoining(const OnlineView *v){
    return v->racing && !v->finishing && (!v->raceReady || (v->arcade && v->localSeat<0));
}
static inline int OnlineViewStarting(const OnlineView *v){
    return v->racing&&v->raceReady&&!v->finishing&&!v->lateJoin&&v->hud[0]==1;
}
void OnlineSetDisc(const char *path);
void OnlineSetServer(const char *url);
void OnlineSetSpeedUnits(const char *units);
const OnlineView *OnlineViewState(void);
void OnlineActionDo(OnlineAction action,int argument,int direction);
void OnlineSelect(int row);
void OnlineText(const char *utf8);
void OnlineBackspace(void);
void OnlineFinishEditing(void);
void OnlinePauseMenu(void);
void OnlineSetInput(int held,int stick,int throttle,int brake,int analog);
void OnlineSetDriverInput(int held,const int32_t words[8]);
int OnlineForceFeedback(int32_t values[8]);
void OnlineEffectsVolume(float gain);
#endif
