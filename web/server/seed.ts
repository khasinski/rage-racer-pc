// Creates (or resets) the local development accounts:
//   node server/seed.ts [--db server/data/rage.db]
// admin / admin (administrator) and rage / racer.
import { join } from 'node:path';
import { parseArgs } from 'node:util';
import { Store } from './db.ts';

const { values } = parseArgs({
  options: { db: { type: 'string', default: process.env.RAGE_DB ?? join(import.meta.dirname, 'data', 'rage.db') } },
});
const store = new Store(values.db!);
store.ensureUser('admin', 'admin', true);
store.ensureUser('rage', 'racer', false);
console.log(`accounts admin/admin (admin) and rage/racer are ready in ${values.db}`);
