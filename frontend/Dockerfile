# Stage 1: Build
FROM node:20-alpine as build

WORKDIR /app

# On définit l'argument de build pour Vite
# Cette variable doit être connue au moment du build car elle est injectée dans le JS
ARG VITE_WS_URL
ENV VITE_WS_URL=$VITE_WS_URL

COPY package*.json ./
RUN npm ci

COPY . .

# Build the app
RUN npm run build

# Stage 2: Serve
FROM nginx:stable-alpine

COPY --from=build /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]