const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
test('captured physical keys map to SDL names and reject unsupported browser codes',async()=>{
 const {bindingName}=await import('../renderer/key-binding.mjs');
 assert.equal(bindingName('KeyQ'),'Q');assert.equal(bindingName('Digit1'),'1');assert.equal(bindingName('Enter'),'Return');assert.equal(bindingName('ArrowLeft'),'Left');assert.equal(bindingName('PageDown'),'PageDown');assert.equal(bindingName('F12'),'F12');assert.equal(bindingName('ShiftLeft'),'Left Shift');assert.equal(bindingName('Unidentified'),null);
 const {validate}=require('../main/config.cjs');assert.equal(validate({'input.cross':bindingName('NumpadMultiply')})['input.cross'],'Keypad *');
 const table=fs.readFileSync(path.resolve(__dirname,'../../external/psyz/external/SDL/src/events/SDL_keymap.c'),'utf8');
 for(const code of ['Enter','ArrowUp','Space','PageDown','PageUp','Backspace','Tab','Home','End','Delete','Insert','F12','ShiftLeft','ShiftRight','ControlLeft','ControlRight','AltLeft','AltRight','MetaLeft','MetaRight','Numpad0','Numpad9','NumpadEnter','NumpadAdd','NumpadSubtract','NumpadMultiply','NumpadDivide','NumpadDecimal','NumpadEqual','NumLock','CapsLock'])assert.ok(table.includes('"'+bindingName(code)+'"'),code);
});
