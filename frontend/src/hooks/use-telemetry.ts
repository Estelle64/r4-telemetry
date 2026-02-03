import { useState, useEffect } from 'react'
import type { Room } from '@/components/room-card'

// URL du WebSocket configurée via les variables d'environnement (Vite)
// Voir fichier .env à la racine du dossier frontend
const WS_URL = import.meta.env.VITE_WS_URL || "ws://localhost:3000"; 

export function useTelemetry() {
  const [rooms, setRooms] = useState<Room[]>([
    {
      id: 1,
      name: "Cafétéria",
      description: "Espace de pause et déjeuner",
      temperature: 22.5,
      humidity: 45,
      status: "connected",
    },
    {
      id: 2,
      name: "Fablab",
      description: "Atelier de prototypage",
      temperature: 24.0,
      humidity: 50,
      status: "error",
      errorMessage: "Attente connexion...",
    },
  ])

  useEffect(() => {
    const ws = new WebSocket(WS_URL);

    ws.onopen = () => {
      console.log('Connected to Telemetry Adapter');
      setRooms(prevRooms => prevRooms.map(room => ({ 
        ...room, 
        status: "connected",
        errorMessage: undefined 
      })));
    };

    ws.onmessage = (event) => {
      try {
        const payload = JSON.parse(event.data);
        // Payload format: { location: "cafet" | "fablab", data: { remoteTemp, remoteHum, ... }, ... }
        console.log("Received:", payload);

        if (payload.location) {
          setRooms(prevRooms => prevRooms.map(room => {
            const isTarget = 
              (payload.location === "cafet" && room.id === 1) ||
              (payload.location === "fablab" && room.id === 2);
            
            if (isTarget && payload.data) {
              return {
                ...room,
                temperature: payload.data.remoteTemp ?? room.temperature,
                humidity: payload.data.remoteHum ?? room.humidity,
                status: "connected",
                errorMessage: undefined
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
