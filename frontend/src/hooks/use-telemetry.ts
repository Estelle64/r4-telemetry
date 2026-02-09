import { useState, useEffect } from 'react'
import type { Room, HistoryPoint } from '@/components/room-card'

// URL du WebSocket configurée via les variables d'environnement (Vite)
const fallbackProtocol = window.location.protocol === "https:" ? "wss" : "ws";
const fallbackHost = window.location.hostname || "localhost";
const fallbackPort = "3000";
const WS_URL =
  import.meta.env.VITE_WS_URL ||
  `${fallbackProtocol}://${fallbackHost}:${fallbackPort}`;

const API_BASE_URL = WS_URL.replace(/^ws/, 'http');

export function useTelemetry() {
  const [rooms, setRooms] = useState<Room[]>([
    {
      id: 1,
      location: "cafet",
      name: "Cafétéria",
      description: "Espace de pause et déjeuner",
      temperature: 0,
      humidity: 0,
      status: "disconnected",
      history: []
    },
    {
      id: 2,
      location: "fablab",
      name: "Fablab",
      description: "Atelier de prototypage",
      temperature: 0,
      humidity: 0,
      status: "disconnected",
      history: []
    },
  ])

  useEffect(() => {
    // Fetch initial history for each room
    rooms.forEach(room => {
      fetch(`${API_BASE_URL}/api/history/${room.location}?limit=50`)
        .then(res => res.json())
        .then(data => {
            if (!data || data.length === 0) return;

            const history: HistoryPoint[] = data.map((block: any) => ({
                time: block.timestamp,
                temperature: parseFloat(block.data.remoteTemp),
                humidity: parseFloat(block.data.remoteHum)
            }));

            // On prend le dernier point de l'historique comme valeur actuelle
            const lastPoint = history[history.length - 1];
            const lastBlock = data[data.length - 1];

            setRooms(prevRooms => prevRooms.map(r => 
                r.id === room.id ? { 
                  ...r, 
                  history,
                  temperature: lastPoint.temperature,
                  humidity: lastPoint.humidity,
                  dhtStatus: lastBlock.data.dhtStatus,
                  status: lastBlock.data.loraStatus ? "connected" : "disconnected",
                  errorMessage: lastBlock.data.loraStatus ? undefined : "Nœud distant hors ligne (History)",
                  seq: lastBlock.data.seq,
                  hmac: lastBlock.data.hmac,
                  rssi: lastBlock.data.rssi,
                  snr: lastBlock.data.snr,
                  packetsLost: lastBlock.data.packetsLost,
                  packetsReceived: lastBlock.data.packetsReceived
                } : r
            ));
        })
        .catch(err => console.error(`Error fetching history for ${room.location}:`, err));
    });

    const ws = new WebSocket(WS_URL);

    ws.onopen = () => {
      console.log('Connected to Telemetry Adapter');
    };

    ws.onmessage = (event) => {
      try {
        const payload = JSON.parse(event.data);
        console.log("Received:", payload);

        if (payload.location) {
          setRooms(prevRooms => prevRooms.map(room => {
            const isTarget = room.location === payload.location;
            
            if (isTarget && payload.data) {
              const newPoint: HistoryPoint = {
                time: payload.timestamp || new Date().toISOString(),
                temperature: parseFloat(payload.data.remoteTemp),
                humidity: parseFloat(payload.data.remoteHum),
              };

              const updatedHistory = [...(room.history || []), newPoint].slice(-50);

              return {
                ...room,
                temperature: newPoint.temperature,
                humidity: newPoint.humidity,
                dhtStatus: payload.data.dhtStatus,
                status: payload.data.loraStatus ? "connected" : "disconnected",
                errorMessage: payload.data.loraStatus ? undefined : "Nœud distant hors ligne (Timeout)",
                history: updatedHistory,
                seq: payload.data.seq,
                hmac: payload.data.hmac,
                rssi: payload.data.rssi,
                snr: payload.data.snr,
                packetsLost: payload.data.packetsLost,
                packetsReceived: payload.data.packetsReceived
              };
            }
            return room;
          }));
        }
      } catch (e) {
        console.error("Error parsing message", e);
      }
    };

    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
      setRooms(prevRooms => prevRooms.map(room => ({
        ...room,
        status: "error",
        errorMessage: "Erreur de connexion"
      })));
    };

    return () => {
      if (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING) {
        ws.close();
      }
    };
  }, [])

  return { rooms }
}
