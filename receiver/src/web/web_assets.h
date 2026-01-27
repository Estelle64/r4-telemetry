#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H

#include <Arduino.h>

// HTML principal (Dashboard avec Pico.css et Alpine.js via CDN)
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr" data-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Gateway LoRa IoT</title>
    <!-- Pico.css pour le style -->
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@picocss/pico@1/css/pico.min.css">
    <!-- Alpine.js pour la réactivité -->
    <script defer src="https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js"></script>
    <style>
        .value-card { text-align: center; padding: 20px; border-radius: 8px; background: #1f2937; margin-bottom: 20px; position: relative; }
        .value-big { font-size: 2.5rem; font-weight: bold; color: #3b82f6; }
        .unit { font-size: 1rem; color: #9ca3af; }
        .label { font-size: 1.1rem; margin-bottom: 10px; display: block; }
        .status-dot { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background-color: #ef4444; margin-right: 5px;}
        .online { background-color: #22c55e; }
        .error-badge { position: absolute; top: 10px; right: 10px; background-color: #ef4444; color: white; padding: 2px 8px; border-radius: 4px; font-size: 0.8rem; }
    </style>
</head>
<body>
    <main class="container" x-data="dashboard()">
        <nav>
            <ul>
                <li><strong>📡 Gateway IoT (R4 WiFi)</strong></li>
            </ul>
            <ul>
                <li><span class="status-dot" :class="connected ? 'online' : ''"></span> <span x-text="connected ? 'Connecté' : 'Hors ligne'"></span></li>
            </ul>
        </nav>

        <article>
            <header>
                Données Distantes (LoRa)
                <span x-show="!loraStatus" class="error-badge">MODULE ERREUR</span>
            </header>
            <div class="grid">
                <div class="value-card">
                    <span class="label">Température</span>
                    <div class="value-big"><span x-text="remoteTemp.toFixed(1)">--</span><span class="unit">°C</span></div>
                </div>
                <div class="value-card">
                    <span class="label">Humidité</span>
                    <div class="value-big"><span x-text="remoteHum.toFixed(1)">--</span><span class="unit">%</span></div>
                </div>
            </div>
            <footer>
                <small>Dernière mise à jour LoRa: <span x-text="lastUpdate">--</span> ms</small>
            </footer>
        </article>

        <article>
            <header>
                Données Locales (Gateway)
                <span x-show="!dhtStatus" class="error-badge">DHT ERREUR</span>
            </header>
            <div class="grid">
                <div class="value-card">
                    <span class="label">Température</span>
                    <div class="value-big"><span x-text="localTemp.toFixed(1)">--</span><span class="unit">°C</span></div>
                </div>
                <div class="value-card">
                    <span class="label">Humidité</span>
                    <div class="value-big"><span x-text="localHum.toFixed(1)">--</span><span class="unit">%</span></div>
                </div>
            </div>
        </article>
    </main>

    <script>
        function dashboard() {
            return {
                remoteTemp: 0,
                remoteHum: 0,
                localTemp: 0,
                localHum: 0,
                lastUpdate: 0,
                connected: false,
                loraStatus: true,
                dhtStatus: true,
                
                init() {
                    this.fetchData();
                    setInterval(() => this.fetchData(), 2000);
                },
                
                async fetchData() {
                    try {
                        const res = await fetch('/api/data');
                        if (!res.ok) throw new Error('Network error');
                        const data = await res.json();
                        
                        this.remoteTemp = data.remoteTemp;
                        this.remoteHum = data.remoteHum;
                        this.localTemp = data.localTemp;
                        this.localHum = data.localHum;
                        this.lastUpdate = data.lastUpdate;
                        this.loraStatus = data.loraStatus;
                        this.dhtStatus = data.dhtStatus;
                        this.connected = true;
                    } catch (e) {
                        console.error(e);
                        this.connected = false;
                    }
                }
            }
        }
    </script>
</body>
</html>
)=====";

#endif // WEB_ASSETS_H