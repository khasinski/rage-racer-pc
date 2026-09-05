file(READ "${INPUT}" mesh HEX)
string(REGEX REPLACE "(..)" "0x\\1," mesh "${mesh}")
file(WRITE "${OUTPUT}" "/* Generated from the authored OBJ by rage-mesh-obj. */\nstatic const unsigned char s_errisoBody[] = {${mesh}};\n")
