import { ModeToggle } from "@/components/mode-toggle"
import { ThemeProvider } from "@/components/theme-provider"
import { RoomCard } from "@/components/room-card"
import { useTelemetry } from "@/hooks/use-telemetry"

function App() {
  // Utilisation du hook personnalisé pour récupérer les données (simulées ou réelles)
  const { rooms } = useTelemetry()

  return (
    <ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
      <div className="flex flex-col min-h-screen w-full bg-background text-foreground transition-colors duration-300">
        <header className="flex-none p-4 flex justify-between items-center border-b bg-card/50 backdrop-blur-sm sticky top-0 z-50">
          <div className="flex items-center gap-2">
            <ModeToggle />
            <span className="font-semibold text-lg">IoT Dashboard</span>
          </div>
        </header>

        {/* Main : flex-1 pour prendre tout l'espace restant, flex/items-center/justify-center pour centrer le contenu */}
        <main className="flex-1 flex flex-col items-center justify-center p-6 md:p-12">
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-12 w-full max-w-6xl mx-auto">
            {rooms.map((room) => (
              <div key={room.id} className="w-full max-w-md mx-auto lg:max-w-none">
                <RoomCard room={room} />
              </div>
            ))}
          </div>
        </main>
      </div>
    </ThemeProvider>
  )
}

export default App