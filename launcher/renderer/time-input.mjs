// Exact full-time clipboard syntax; plain numbers remain native segment edits.
export function parseTimeClipboard(text) {
 const match=/^(\d{1,5})\s*:\s*(\d{2})\s*\.\s*(\d{3})$/.exec(text.trim());
 if(!match)return null;
 const [minutes,seconds,milliseconds]=match.slice(1).map(Number);
 if(seconds>59||minutes*60000+seconds*1000+milliseconds>2147483647)return null;
 return {m:minutes,s:seconds,ms:milliseconds};
}
