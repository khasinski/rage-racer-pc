# Rage Racer online server image (CapRover). Built from the bundle that
# scripts/package-deploy.mjs assembles: the built client (dist/), the server
# (server/, shared/ and the rage-server WebAssembly) and the operator's disc.
FROM node:26-slim
WORKDIR /app
ENV NODE_ENV=production
COPY package.json package-lock.json ./
RUN npm ci --omit=dev --ignore-scripts && npm cache clean --force
COPY shared ./shared
COPY server ./server
COPY dist ./dist
COPY disc ./disc
# Accounts, rooms and results: mount a persistent volume here.
VOLUME /data
EXPOSE 7243
CMD ["node", "server/main.ts", "--disc", "/app/disc/track01.bin", "--db", "/data/rage.db", "--port", "7243"]
