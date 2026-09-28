# Rage Racer online server image (CapRover). Built from the bundle that
# scripts/package-deploy.mjs assembles: the built client (dist/), the server
# (server/, shared/ and the rage-server WebAssembly) and the operator's disc.
FROM node:26-slim
WORKDIR /app
ENV NODE_ENV=production
# The race's WebRTC data channel (server/rtc.ts): one UDP port for every
# client. Set RAGE_RTC_PUBLIC_IP to the host's public address and publish
# the port; without them the race stays on the WebSocket.
ENV RAGE_RTC_PORT=7243
COPY package.json package-lock.json ./
RUN npm ci --omit=dev --ignore-scripts && npm cache clean --force
COPY shared ./shared
COPY server ./server
COPY dist ./dist
COPY disc ./disc
# Accounts, rooms and results: mount a persistent volume here.
VOLUME /data
EXPOSE 7243
EXPOSE 7243/udp
CMD ["node", "server/main.ts", "--disc", "/app/disc/track01.bin", "--db", "/data/rage.db", "--port", "7243"]
