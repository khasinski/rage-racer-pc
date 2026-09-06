set(root_evidence "${EVIDENCE}")
set(cases
    "0,1,12,Erriso"
    "0,2,14,Erriso"
    "0,3,16,Erriso"
    "1,1,20,Abeille"
    "1,2,22,Abeille"
    "2,1,26,Pegase"
    "3,1,30,Esperanza"
    "3,2,32,Esperanza"
    "3,3,34,Esperanza"
    "3,4,36,Esperanza"
    "4,1,40,Acceron"
    "4,2,42,Acceron"
    "4,3,44,Acceron"
    "5,1,48,Bayonet"
    "5,2,50,Bayonet"
    "6,1,54,Hijack"
    "7,1,58,Fatalita"
    "7,2,60,Fatalita"
    "8,1,64,Istante")
foreach(case IN LISTS cases)
    string(REPLACE "," ";" fields "${case}")
    list(GET fields 0 CAR)
    list(GET fields 1 VARIANT)
    list(GET fields 2 PLAYER_ASSET)
    list(GET fields 3 PLAYER_NAME)
    set(CLASS 2)
    set(NAME Esperanza)
    set(RIVAL_ASSET 104)
    set(GRID 0,1,2,3,4,5,6,7,8,9,10)
    set(EVIDENCE "${root_evidence}/player-grade-${PLAYER_ASSET}")
    include("${CMAKE_CURRENT_LIST_DIR}/verify_authored_transition.cmake")
endforeach()
