#ifndef PORT_KEYBOARD_TEXT_H
#define PORT_KEYBOARD_TEXT_H

/* Raw typed-character edge detection, independent of the pad-button
 * abstraction (input_config.c maps keys to pad buttons, not to letters).
 * Returns the character the player just pressed this frame -- 'A'-'Z',
 * '0'-'9', ' ', '.', ':' (shifted ';'), or '\b' for backspace -- or 0 if
 * nothing new was typed. Used only by arcade-style text entry screens
 * (see mp_menu.c's EditText); the rest of the game never needs this. */
char PortConsumeTypedChar(void);

#endif
