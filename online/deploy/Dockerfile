FROM node:26-slim
WORKDIR /app
ENV NODE_ENV=production RAGE_RTC_PORT=7243
COPY package.json package-lock.json ./
RUN npm ci --omit=dev --ignore-scripts && npm cache clean --force
COPY shared ./shared
COPY server ./server
COPY dist ./dist
COPY disc ./disc
ARG RAGE_BUILD_ID=development
ENV RAGE_BUILD_ID=$RAGE_BUILD_ID
VOLUME /data
EXPOSE 7243 7243/udp
HEALTHCHECK --interval=30s --timeout=5s --start-period=20s CMD node -e "fetch('http://127.0.0.1:7243/healthz').then(r=>process.exit(r.ok?0:1)).catch(()=>process.exit(1))"
CMD ["node", "server/main.ts", "--disc", "/app/disc/track01.bin", "--db", "/data/rage.db", "--port", "7243"]
