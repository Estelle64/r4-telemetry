import { useState, useEffect } from 'react'
import type { Room } from '@/components/room-card'

// URL du WebSocket (à configurer plus tard avec l'adresse réelle de l'adapter)
// ex: const WS_URL = "ws://localhost:3000"
const WS_URL = "ws://localhost:8080" 

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
    // Logique de connexion WebSocket (désactivée pour l'instant tant que le backend n'est pas là)
    // On garde la simulation pour le développement UI
    
    /* 
    const ws = new WebSocket(WS_URL);

    ws.onopen = () => {
      console.log('Connected to Telemetry Adapter');
      // Mettre à jour les status en "connected"
    };

    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        // Supposons que le backend envoie { roomId: 1, type: 'temp', value: 23.5 }
        // Ou un objet complet
        console.log("Received:", data);
        
        // Mettre à jour l'état ici...
      } catch (e) {
        console.error("Error parsing message", e);
      }
    };

    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
      // Mettre les status en "error"
    };

    return () => ws.close();
    */

    // Simulation de données en temps réel pour montrer que l'UI est réactive
    const interval = setInterval(() => {
      setRooms(prevRooms => prevRooms.map(room => {
        if (room.status === 'error' && Math.random() > 0.9) {
           // Simuler une reconnexion aléatoire du Fablab
           return { ...room, status: 'connected', errorMessage: undefined }
        }
        
        // Variation aléatoire des valeurs
        const tempVar = (Math.random() - 0.5) * 0.2;
        const humVar = (Math.random() - 0.5) * 1;

        return {
          ...room,
          temperature: parseFloat((room.temperature + tempVar).toFixed(1)),
          humidity: Math.round(room.humidity + humVar),
        }
      }))
    }, 2000)

    return () => clearInterval(interval)
  }, [])

  return { rooms }
}
