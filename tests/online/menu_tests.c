#include "online/model.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

void OnlineOpen(void);
void OnlineShutdown(void);

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr,"menu check failed: %s\n",#condition); return 1; } } while (0)

int main(void) {
    OnlineView joining={.racing=1,.arcade=1,.localSeat=-1};
    CHECK(OnlineViewJoining(&joining));
    joining.raceReady=1;joining.takeoverSeconds=3;
    CHECK(OnlineViewJoining(&joining));
    joining.localSeat=0;joining.takeoverSeconds=0;
    CHECK(!OnlineViewJoining(&joining));
    joining.racing=0;joining.raceReady=0;
    CHECK(!OnlineViewJoining(&joining));
    joining=(OnlineView){.racing=1,.raceReady=1,.localSeat=0};joining.hud[0]=1;
    CHECK(OnlineViewStarting(&joining));
    joining.hud[0]=2;CHECK(!OnlineViewStarting(&joining));
    joining.hud[0]=1;joining.finishing=1;CHECK(!OnlineViewStarting(&joining));
    CHECK(SDL_Init(SDL_INIT_EVENTS));
    OnlineOpen();
    const OnlineView *v=OnlineViewState();
    CHECK(v->screen==OS_PROFILE && v->count==4);
    CHECK(v->rows[0].action==OA_ARCADE && v->rows[0].enabled);
    CHECK(v->rows[1].action==OA_GUEST);
    for(int i=0;i<v->count;i++)CHECK(!v->rows[i].editable);
    OnlineActionDo(OA_PROFILE_OPTIONS,0,0);
    CHECK(OnlineViewState()->screen==OS_PROFILE_OPTIONS);
    OnlineActionDo(OA_NAME,0,0);
    OnlineText(" Test");
    OnlineFinishEditing();
    CHECK(!strcmp(OnlineViewState()->rows[0].value,"Racer Test"));
    OnlineActionDo(OA_ACCOUNT,0,0);
    CHECK(OnlineViewState()->screen==OS_ACCOUNT);
    CHECK(v->rows[1].action==OA_PASSWORD && v->rows[2].action==OA_LOGIN);
    OnlineActionDo(OA_BACK,0,0);
    CHECK(OnlineViewState()->screen==OS_PROFILE_OPTIONS && v->active);
    OnlineActionDo(OA_CONNECTION,0,0);
    CHECK(OnlineViewState()->screen==OS_CONNECTION && v->rows[0].action==OA_SERVER);
    OnlineActionDo(OA_BACK,0,0);
    OnlineActionDo(OA_BACK,0,0);
    CHECK(OnlineViewState()->screen==OS_PROFILE && v->selected==0 && v->active);
    OnlineActionDo(OA_DUEL_CODE,0,0);
    CHECK(OnlineViewState()->screen==OS_DUEL_CODE&&v->rows[0].editable&&!v->rows[2].enabled&&!v->duelCodeInvalid);
    OnlineActionDo(OA_JOIN_DUEL,0,0);OnlineText("cancelled");OnlinePauseMenu();
    CHECK(!OnlineViewState()->editing&&!v->rows[0].value[0]&&!v->rows[2].enabled);
    const char *bad[]={"", "short", "AbCd_12-4567x", "AbCd 123-4567", "AbCd/123-4567", "AbCd_123-45ą"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){
        OnlineActionDo(OA_JOIN_DUEL,0,0);OnlineText(bad[i]);
        CHECK(!OnlineViewState()->rows[2].enabled);
        OnlineActionDo(OA_SUBMIT_DUEL,0,0);CHECK(OnlineViewState()->status[0]);
        OnlineCancelEditing();
    }
    OnlineActionDo(OA_JOIN_DUEL,0,0);OnlineText("  AbCd_12-4567 \n");
    CHECK(OnlineViewState()->rows[2].enabled&&!v->duelCodeInvalid);
    OnlineFinishEditing();
    CHECK(!strcmp(OnlineViewState()->rows[0].value,"AbCd_12-4567")&&v->rows[2].enabled&&v->selected==2);
    OnlineActionDo(OA_BACK,0,0);CHECK(OnlineViewState()->screen==OS_ROOMS);
    OnlineActionDo(OA_BACK,0,0);CHECK(OnlineViewState()->screen==OS_PROFILE);
    OnlineActionDo(OA_BACK,0,0);
    CHECK(!OnlineViewState()->active);
    OnlineShutdown();
    SDL_Quit();
    return 0;
}
