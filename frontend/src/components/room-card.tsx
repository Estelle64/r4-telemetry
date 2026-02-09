import { useState, useMemo } from "react"
import { Area, AreaChart, CartesianGrid, XAxis } from "recharts"
import { Droplets, Thermometer } from "lucide-react"

import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card"
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select"
import {
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
} from "@/components/ui/chart"
import type { ChartConfig } from "@/components/ui/chart"
import { Badge } from "@/components/ui/badge"

// Types
export type RoomStatus = "connected" | "disconnected" | "error"

export interface HistoryPoint {
  time: string
  temperature: number
  humidity: number
}

export interface Room {
  id: number
  name: string
  location: "cafet" | "fablab"
  description: string
  temperature: number
  humidity: number
  status: RoomStatus
  errorMessage?: string
  history?: HistoryPoint[]
  seq?: number
  hmac?: string
}

// Configuration des graphiques (Couleurs et Libellés)
const chartConfig = {
  temperature: {
    label: "Temperature",
    color: "#f97316", // Orange-500
  },
  humidity: {
    label: "Humidity",
    color: "#3b82f6", // Blue-500
  },
} satisfies ChartConfig

// Helper pour le badge de status
const getStatusBadge = (status: RoomStatus, errorMessage?: string) => {
  switch (status) {
    case "connected":
      return (
        <Badge className="bg-green-500 hover:bg-green-600 text-white border-transparent">
          Connecté
        </Badge>
      )
    case "disconnected":
      return (
        <Badge variant="destructive">
          Déconnecté
        </Badge>
      )
    case "error":
      return (
        <Badge className="bg-orange-500 hover:bg-orange-600 text-white border-transparent" title={errorMessage}>
          Erreur
        </Badge>
      )
  }
}

export function RoomCard({ room }: { room: Room }) {
  const [timeRange, setTimeRange] = useState("24h")

  // On utilise l'historique fourni par le room object
  const chartData = useMemo(() => {
    if (!room.history || room.history.length === 0) {
        return [];
    }
    
    return room.history.map(p => ({
        ...p,
        time: new Date(p.time).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
    }));
  }, [room.history])

  return (
    <Card className="hover:shadow-lg transition-shadow overflow-hidden">
      <CardHeader className="pb-4">
        <div className="flex justify-between items-start">
          <div className="space-y-1">
            <div className="flex items-center gap-2">
               <CardTitle>{room.name}</CardTitle>
               {getStatusBadge(room.status, room.errorMessage)}
            </div>
            <CardDescription>{room.description}</CardDescription>
            {room.status === "error" && room.errorMessage && (
                <p className="text-xs text-orange-500 font-medium pt-1">
                    Attention: {room.errorMessage}
                </p>
            )}
          </div>
          <Select value={timeRange} onValueChange={setTimeRange}>
            <SelectTrigger className="w-[80px] h-8 text-xs">
              <SelectValue placeholder="Range" />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value="1h">1h</SelectItem>
              <SelectItem value="24h">24h</SelectItem>
              <SelectItem value="7d">7j</SelectItem>
            </SelectContent>
          </Select>
        </div>
      </CardHeader>

      <CardContent className="space-y-6">
        {/* Indicateurs actuels (Gros chiffres) */}
        <div className="grid grid-cols-2 gap-4">
          <div className="flex flex-col items-center p-3 bg-secondary/30 rounded-lg">
            <div className="flex items-center gap-2 mb-1 text-muted-foreground">
              <Thermometer className="h-4 w-4" />
              <span className="text-sm">Temp</span>
            </div>
            <span className="text-2xl font-bold">{room.temperature}°C</span>
          </div>
          <div className="flex flex-col items-center p-3 bg-secondary/30 rounded-lg">
            <div className="flex items-center gap-2 mb-1 text-muted-foreground">
              <Droplets className="h-4 w-4" />
              <span className="text-sm">Humidité</span>
            </div>
            <span className="text-2xl font-bold">{room.humidity}%</span>
          </div>
        </div>

        {/* Graphique Température */}
        <div className="space-y-1">
          <div className="text-xs font-semibold text-muted-foreground ml-1">Historique Température</div>
          <ChartContainer config={chartConfig} className="h-[100px] w-full">
            <AreaChart
              data={chartData}
              margin={{ top: 5, right: 0, left: 0, bottom: 0 }}
            >
              <defs>
                <linearGradient id="fillTemp" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%" stopColor="var(--color-temperature)" stopOpacity={0.8} />
                  <stop offset="95%" stopColor="var(--color-temperature)" stopOpacity={0.1} />
                </linearGradient>
              </defs>
              <CartesianGrid vertical={false} strokeDasharray="3 3" strokeOpacity={0.1} />
              <XAxis 
                dataKey="time" 
                tickLine={false} 
                axisLine={false} 
                tickMargin={8} 
                tick={{ fontSize: 10 }}
                minTickGap={30}
              />
              <ChartTooltip cursor={false} content={<ChartTooltipContent />} />
              <Area
                dataKey="temperature"
                type="monotone"
                stroke="var(--color-temperature)"
                fill="url(#fillTemp)"
                strokeWidth={2}
                dot={false}
              />
            </AreaChart>
          </ChartContainer>
        </div>

        {/* Graphique Humidité */}
        <div className="space-y-1">
          <div className="text-xs font-semibold text-muted-foreground ml-1">Historique Humidité</div>
          <ChartContainer config={chartConfig} className="h-[100px] w-full">
            <AreaChart
              data={chartData}
              margin={{ top: 5, right: 0, left: 0, bottom: 0 }}
            >
              <defs>
                 <linearGradient id="fillHum" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%" stopColor="var(--color-humidity)" stopOpacity={0.8} />
                  <stop offset="95%" stopColor="var(--color-humidity)" stopOpacity={0.1} />
                </linearGradient>
              </defs>
              <CartesianGrid vertical={false} strokeDasharray="3 3" strokeOpacity={0.1} />
              <XAxis 
                dataKey="time" 
                tickLine={false} 
                axisLine={false} 
                tickMargin={8} 
                tick={{ fontSize: 10 }}
                minTickGap={30}
              />
              <ChartTooltip cursor={false} content={<ChartTooltipContent />} />
              <Area
                dataKey="humidity"
                type="monotone"
                stroke="var(--color-humidity)"
                fill="url(#fillHum)"
                strokeWidth={2}
                dot={false}
              />
            </AreaChart>
          </ChartContainer>
        </div>

        {/* Footer avec Preuve d'intégrité */}
        {(room.seq !== undefined || room.hmac) && (
          <div className="pt-4 border-t border-border/50">
            <div className="flex flex-col gap-1">
              <span className="text-[10px] uppercase tracking-wider text-muted-foreground font-semibold">
                Preuve d'intégrité (HMAC SHA256)
              </span>
              <div className="flex justify-between items-center bg-secondary/20 p-2 rounded text-[10px] font-mono">
                <span className="text-orange-500">SEQ: {room.seq}</span>
                <span className="text-muted-foreground truncate ml-4" title={room.hmac}>
                  {room.hmac ? `${room.hmac.substring(0, 16)}...` : "N/A"}
                </span>
              </div>
            </div>
          </div>
        )}
      </CardContent>
    </Card>
  )
}