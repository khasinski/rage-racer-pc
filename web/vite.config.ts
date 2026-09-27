import { defineConfig } from 'vite';

// `npm run dev` proxies the API and WebSocket to a running multiplayer server.
const server = process.env.RAGE_SERVER ?? 'http://localhost:7243';

export default defineConfig({
  base: './',
  build: { target: 'esnext', chunkSizeWarningLimit: 1024 },
  server: {
    proxy: {
      '/api': server,
      '/ws': { target: server.replace(/^http/, 'ws'), ws: true },
    },
  },
});
