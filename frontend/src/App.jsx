import { useState, useEffect } from "react";
import "./App.css";

function App() {
  const [cafetTelemetry, setCafetTelemetry] = useState(null);
  const [fablabTelemetry, setFablabTelemetry] = useState(null);
  const [connectionStatus, setConnectionStatus] = useState(
    "Waiting for data...",
  );

  useEffect(() => {
    const ws = new WebSocket(`ws://localhost:3000`);

    ws.onopen = () => {
      console.log("Connexion WebSocket ouverte");
      setConnectionStatus("Connecté au serveur. En attente des données...");
    };

    ws.onmessage = (event) => {
      console.log("Message du serveur:", event.data);
      try {
        const receivedTelemetry = JSON.parse(event.data);
        if (receivedTelemetry.location === "cafet") {
          setCafetTelemetry(receivedTelemetry);
        } else if (receivedTelemetry.location === "fablab") {
          setFablabTelemetry(receivedTelemetry);
        }
        setConnectionStatus("Connecté");
      } catch (e) {
        console.error("Erreur de traitement des données:", e);
        setConnectionStatus("Erreur de données.");
      }
    };

    ws.onclose = () => {
      console.log("Connexion WebSocket ouverte");
      setConnectionStatus(
        "Connexion fermée. Rafraichir la page pour se reconnecter.",
      );
      setCafetTelemetry(null);
      setFablabTelemetry(null);
    };

    ws.onerror = (error) => {
      console.error("Erreur WebSocket:", error);
      setConnectionStatus("Erreur de connexion.");
      setCafetTelemetry(null);
      setFablabTelemetry(null);
    };

    return () => {
      ws.close();
    };
  }, []);

  const renderTelemetryArticle = (telemetryData, title) => {
    if (!telemetryData) {
      return (
        <article>
          <header>{title}</header>
          <p>
            No data available for {title.toLowerCase().replace(" data", "")}.
          </p>
        </article>
      );
    }

    return (
      <article>
        <header>
          {title}
          {telemetryData.data && !telemetryData.data.loraStatus && (
            <span className="error-badge">LoRa ERROR</span>
          )}
          {telemetryData.data && !telemetryData.data.dhtStatus && (
            <span className="error-badge">DHT ERROR</span>
          )}
        </header>
        <div className="grid">
          <div className="value-card">
            <span className="label">Température</span>
            <div className="value-big">
              <span>
                {telemetryData.data ? telemetryData.data.remoteTemp : "N/A"}
              </span>
              <span className="unit">°C</span>
            </div>
          </div>
          <div className="value-card">
            <span className="label">Humidité</span>
            <div className="value-big">
              <span>
                {telemetryData.data ? telemetryData.data.remoteHum : "N/A"}
              </span>
              <span className="unit">%</span>
            </div>
          </div>
        </div>
        <footer>
          {telemetryData.timestamp && (
            <small>
              Date & Heure:{" "}
              <span>{new Date(telemetryData.timestamp).toLocaleString()}</span>
            </small>
          )}
          <br />
        </footer>
      </article>
    );
  };

  return (
    <main className="container">
      <nav>
        <ul>
          <li>
            <strong>📡 Gateway IoT (R4 WiFi)</strong>
          </li>
        </ul>
        <ul>
          <li>
            <span
              className={`status-dot ${connectionStatus === "Connecté" ? "online" : ""}`}
            ></span>
            <span> {connectionStatus}</span>
          </li>
        </ul>
      </nav>

      {connectionStatus === "Connected" || cafetTelemetry || fablabTelemetry ? (
        <div className="telemetry-container">
          {renderTelemetryArticle(cafetTelemetry, "Données de la cafétéria")}
          {renderTelemetryArticle(fablabTelemetry, "Données du Fablab")}
        </div>
      ) : (
        <article>
          <p>{connectionStatus}</p>
        </article>
      )}
    </main>
  );
}

export default App;
