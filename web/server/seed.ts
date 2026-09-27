// Creates (or resets) the two development accounts, `admin` (administrator)
// and `rage`:
//   node server/seed.ts [--db server/data/rage.db] [--admin-password P] [--rage-password P]
// Passwords that are not given are generated and printed once.
import { randomBytes } from 'node:crypto';
import { join } from 'node:path';
import { parseArgs } from 'node:util';
import { Store } from './db.ts';

const { values } = parseArgs({
  options: {
    db: { type: 'string', default: process.env.RAGE_DB ?? join(import.meta.dirname, 'data', 'rage.db') },
    'admin-password': { type: 'string' },
    'rage-password': { type: 'string' },
  },
});
const generate = () => randomBytes(24).toString('base64').replace(/[^A-Za-z0-9]/g, '').slice(0, 20);
const accounts = [
  { name: 'admin', password: values['admin-password'] ?? generate(), admin: true },
  { name: 'rage', password: values['rage-password'] ?? generate(), admin: false },
];
const store = new Store(values.db!);
for (const account of accounts) store.ensureUser(account.name, account.password, account.admin);
console.log(`accounts in ${values.db}:`);
for (const account of accounts) console.log(`  ${account.name.padEnd(6)} ${account.password}${account.admin ? '  (admin)' : ''}`);
