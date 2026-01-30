import React, { useState, useEffect } from "react";
import "./App.css"; // Import the updated CSS

function App() {
  const [cafetTelemetry, setCafetTelemetry] = useState(null);
  const [fablabTelemetry, setFablabTelemetry] = useState(null);
  const [connectionStatus, setConnectionStatus] = useState(
    "Waiting for data...",
  );

  useEffect(() => {
    const ws = new WebSocket(`ws://localhost:3000`);

    ws.onopen = () => {
      console.log("WebSocket connection opened");
      setConnectionStatus("Connected to server. Waiting for data...");
    };

    ws.onmessage = (event) => {
      console.log("Message from server:", event.data);
      try {
        const receivedTelemetry = JSON.parse(event.data);
        if (receivedTelemetry.location === "cafet") {
          setCafetTelemetry(receivedTelemetry);
        } else if (receivedTelemetry.location === "fablab") {
          setFablabTelemetry(receivedTelemetry);
        }
        setConnectionStatus("Connected");
      } catch (e) {
        console.error("Error parsing WebSocket data:", e);
        setConnectionStatus("Error processing data. Please check console.");
      }
    };

    ws.onclose = () => {
      console.log("WebSocket connection closed");
      setConnectionStatus(
        "Connection closed. Please refresh the page to reconnect.",
      );
      setCafetTelemetry(null);
      setFablabTelemetry(null);
    };

    ws.onerror = (error) => {
      console.error("WebSocket error:", error);
      setConnectionStatus("Connection error. Please check the console.");
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
            <span className="label">Temperature</span>
            <div className="value-big">
              <span>
                {telemetryData.data
                  ? telemetryData.remoteTemp.toFixed(1)
                  : "N/A"}
              </span>
              <span className="unit">°C</span>
            </div>
          </div>
          <div className="value-card">
            <span className="label">Humidity</span>
            <div className="value-big">
              <span>
                {telemetryData.data
                  ? telemetryData.remoteHum.toFixed(1)
                  : "N/A"}
              </span>
              <span className="unit">%</span>
            </div>
          </div>
        </div>
        <footer>
          {telemetryData.timestamp && (
            <small>
              Timestamp:{" "}
              <span>{new Date(telemetryData.timestamp).toLocaleString()}</span>
            </small>
          )}
          <br />
          <small>
            Last LoRa Update:{" "}
            <span>{telemetryData.data ? telemetryData.lastUpdate : "N/A"}</span>{" "}
            ms ago
          </small>
          <br />
          <small>
            Time Synced:{" "}
            <span>
              {telemetryData.data
                ? telemetryData.timeSynced
                  ? "Yes"
                  : "No"
                : "N/A"}
            </span>
          </small>
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
              className={`status-dot ${connectionStatus === "Connected" ? "online" : ""}`}
            ></span>
            <span> {connectionStatus}</span>
          </li>
        </ul>
      </nav>

      {connectionStatus === "Connected" || cafetTelemetry || fablabTelemetry ? (
        <>
          {renderTelemetryArticle(cafetTelemetry, "Cafet Data")}
          {renderTelemetryArticle(fablabTelemetry, "Fablab Data")}
        </>
      ) : (
        <article>
          <p>{connectionStatus}</p>
        </article>
      )}
    </main>
  );
}

export default App;
