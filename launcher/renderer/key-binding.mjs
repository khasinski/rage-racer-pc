// SDL scancode names: external/psyz/external/SDL/src/events/SDL_keymap.c.
// Use physical key codes, so binding positions follow the game's scancodes.
export function bindingName(code){
 if(/^Key[A-Z]$/.test(code))return code.slice(3);
 if(/^Numpad[0-9]$/.test(code))return 'Keypad '+code.slice(6);
 if(/^Digit[0-9]$/.test(code))return code.slice(5);
 if(/^F([1-9]|1[0-9]|2[0-4])$/.test(code))return code;
 return {ShiftLeft:'Left Shift',ShiftRight:'Right Shift',ControlLeft:'Left Ctrl',ControlRight:'Right Ctrl',AltLeft:'Left Alt',AltRight:'Right Alt',MetaLeft:'Left GUI',MetaRight:'Right GUI',NumpadEnter:'Keypad Enter',NumpadAdd:'Keypad +',NumpadSubtract:'Keypad -',NumpadMultiply:'Keypad *',NumpadDivide:'Keypad /',NumpadDecimal:'Keypad .',NumpadEqual:'Keypad =',NumLock:'Numlock',CapsLock:'CapsLock',ArrowUp:'Up',ArrowDown:'Down',ArrowLeft:'Left',ArrowRight:'Right',Enter:'Return',Space:'Space',Tab:'Tab',Backspace:'Backspace',Delete:'Delete',Insert:'Insert',Home:'Home',End:'End',PageUp:'PageUp',PageDown:'PageDown',Escape:'Escape',Minus:'-',Equal:'=',BracketLeft:'[',BracketRight:']',Backslash:'\\',Semicolon:';',Quote:"'",Comma:',',Period:'.',Slash:'/'}[code]||null;
}
