const schema = require('../renderer/settings.json');
const defaults = Object.fromEntries(schema.map(f => [f.key, f.default]));
function validate(values) {
  if (!values || typeof values !== 'object' || Array.isArray(values)) throw Error('Invalid settings');
  const result = {};
  for (const [key, value] of Object.entries(values)) {
    const f = schema.find(x => x.key === key);
    if (!f) throw Error(`Unknown setting: ${key}`);
    if (f.type === 'bool') {
      if (typeof value !== 'boolean') throw Error(`Invalid ${f.label}`);
    } else if (typeof value !== 'string' || /[\r\n\0]/.test(value)) throw Error(`Invalid ${f.label}`);
    if (f.type === 'number' && (!value.trim() || !Number.isFinite(Number(value)) || Number(value) < f.min || Number(value) > f.max)) throw Error(`${f.label}: expected ${f.min}–${f.max}`);
    if (f.type === 'select' && !f.options.some(o => o[0] === value)) throw Error(`Invalid ${f.label}`);
    if (f.type === 'key' && (!value || value.length > 40 || !/^[A-Za-z0-9 *+\-\[\];',./\\=]+$/.test(value))) throw Error(`Invalid key binding: ${f.label}`);
    result[key] = value;
  }
  return result;
}
/* Retain comments, unknown keys and sections; update all duplicate occurrences
 * because the runtime uses the last occurrence. Add missing keys as dotted keys
 * in a fresh section (the runtime accepts dotted keys only outside a section). */
function patchIni(text, changes) {
  for (const [k,v] of Object.entries(changes)) if (!/^[a-z0-9_.]+$/.test(k) || /[\r\n\0]/.test(String(v))) throw Error('Invalid INI value');
  const pending = new Map(Object.entries(changes)); let section = '';
  const lines = text.split(/\r?\n/).map(line => {
    const heading = line.match(/^\s*\[([^\]]+)\]/);
    if (heading) { section = heading[1].trim(); return line; }
    const m = line.match(/^(\s*)([^#;=\s][^=]*?)\s*=(.*)$/);
    if (!m) return line;
    const key = (section ? section + '.' : '') + m[2].trim();
    if (!Object.hasOwn(changes,key)) return line;
    pending.delete(key); return `${m[1]}${m[2].trim()} = ${changes[key]}`;
  });
  const groups = {};
  for (const [key,value] of pending) { const dot=key.indexOf('.');const group=key.slice(0,dot), name=key.slice(dot+1);(groups[group]??=[]).push(`${name} = ${value}`); }
  for (const [group,rows] of Object.entries(groups)) lines.push('',`[${group}]`,...rows);
  return lines.join('\n');
}
module.exports = { schema, defaults, validate, patchIni };
