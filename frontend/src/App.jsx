import React, { useState, useEffect } from 'react';
import './App.css';

function App() {
  const [remoteTemp, setRemoteTemp] = useState(0);
  const [remoteHum, setRemoteHum] = useState(0);
  const [localTemp, setLocalTemp] = useState(0);
  const [localHum, setLocalHum] = useState(0);
  const [lastUpdate, setLastUpdate] = useState('--');
  const [connected, setConnected] = useState(false);
  const [loraStatus, setLoraStatus] = useState(true);
  const [dhtStatus, setDhtStatus] = useState(true);

  useEffect(() => {
    const ws = new WebSocket('ws://localhost:8080');

    ws.onopen = () => {
      console.log('Connecté au serveur WebSocket.');
      setConnected(true);
    };

    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        console.log('Données reçues via WebSocket:', data);

        // Mettez à jour l'état en fonction de la structure de vos données
        // Cet exemple suppose que le backend envoie un objet
        // avec les clés correspondantes (remoteTemp, etc.)
        if (data.remoteTemp) setRemoteTemp(data.remoteTemp);
        if (data.remoteHum) setRemoteHum(data.remoteHum);
        if (data.localTemp) setLocalTemp(data.localTemp);
        if (data.localHum) setLocalHum(data.localHum);
        if (data.lastUpdate) setLastUpdate(data.lastUpdate);
        if (data.loraStatus !== undefined) setLoraStatus(data.loraStatus);
        if (data.dhtStatus !== undefined) setDhtStatus(data.dhtStatus);

      } catch (e) {
        console.error("Erreur lors du traitement des données WebSocket:", e);
      }
    };

    ws.onclose = () => {
      console.log('Déconnecté du serveur WebSocket.');
      setConnected(false);
    };

    ws.onerror = (error) => {
      console.error('Erreur WebSocket:', error);
      setConnected(false);
    };

    // Nettoyage à la désinstallation du composant
    return () => {
      ws.close();
    };
  }, []);

  return (
    <main className="container">
      <nav>
        <ul>
          <li><strong>📡 Gateway IoT (R4 WiFi)</strong></li>
        </ul>
        <ul>
          <li>
            <span className={`status-dot ${connected ? 'online' : ''}`}></span>
            <span> {connected ? 'Connecté' : 'Hors ligne'}</span>
          </li>
        </ul>
      </nav>

      <article>
        <header>
          Données Distantes (LoRa)
          {!loraStatus && <span className="error-badge">MODULE ERREUR</span>}
        </header>
        <div className="grid">
          <div className="value-card">
            <span className="label">Température</span>
            <div className="value-big">
              <span>{remoteTemp.toFixed(1)}</span><span className="unit">°C</span>
            </div>
          </div>
          <div className="value-card">
            <span className="label">Humidité</span>
            <div className="value-big">
              <span>{remoteHum.toFixed(1)}</span><span className="unit">%</span>
            </div>
          </div>
        </div>
        <footer>
          <small>Dernière mise à jour LoRa: <span>{lastUpdate}</span> ms</small>
        </footer>
      </article>

      <article>
        <header>
          Données Locales (Gateway)
          {!dhtStatus && <span className="error-badge">DHT ERREUR</span>}
        </header>
        <div className="grid">
          <div className="value-card">
            <span className="label">Température</span>
            <div className="value-big">
              <span>{localTemp.toFixed(1)}</span><span className="unit">°C</span>
            </div>
          </div>
          <div className="value-card">
            <span className="label">Humidité</span>
            <div className="value-big">
              <span>{localHum.toFixed(1)}</span><span className="unit">%</span>
            </div>
          </div>
        </div>
      </article>
    </main>
  );
}

export default App;
