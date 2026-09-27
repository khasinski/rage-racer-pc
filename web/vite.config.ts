import { defineConfig } from 'vite';

export default defineConfig({
  base: './',
  build: { target: 'esnext', chunkSizeWarningLimit: 1024 },
});
