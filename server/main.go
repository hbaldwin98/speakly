package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"

	"github.com/gorilla/websocket"
	"github.com/pion/webrtc/v3"
	"github.com/rs/cors"
	"github.com/satori/go.uuid"
)

var tokens = make(map[string]bool)
var server = Server{
	Clients: make(map[string]*ConnectedClient),
	Rooms:   make(map[string]*Room),
}

func main() {
	c := cors.New(cors.Options{
		AllowedOrigins: []string{"*"}, // Allow requests from any origin
		AllowedMethods: []string{"GET", "POST", "OPTIONS", "PUT", "DELETE"},
		AllowedHeaders: []string{"*"},
	})

	connectHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		handleConnect(w, r)
	})
	connectTokenHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		handleConnectToken(w, r)
	})

	http.Handle("/connect", c.Handler(connectHandler))
	http.Handle("/connect/token", c.Handler(connectTokenHandler))

	log.Println("Creating default room...")
	createDefaultRoom()

	port := 9000
	addr := fmt.Sprintf(":%d", port)
	log.Printf("Listening on %s...\n", addr)
	if err := http.ListenAndServe(addr, nil); err != nil {
		log.Println("Server error: ", err)
		panic(err)
	}

	log.Println("Server stopped.")
}

func createDefaultRoom() *Room {
	room := Room{
		Id:               uuid.NewV4().String(),
		Name:             "Default",
		ConnectedClients: make(map[string]*ConnectedClient),
		Settings:         RoomSettings{},
	}

	server.Rooms[room.Id] = &room
	return &room
}

func handleConnectToken(w http.ResponseWriter, r *http.Request) {
	uuid := uuid.NewV4().String()
	tokens[uuid] = true

	log.Println("Token generated", uuid)

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{
		"token": uuid,
	})
}

var upgrader = websocket.Upgrader{
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

func handleConnect(w http.ResponseWriter, r *http.Request) {
	token := r.URL.Query().Get("token")
	if token == "" {
		w.WriteHeader(http.StatusBadRequest)
		return
	}

	if !tokens[token] {
		w.WriteHeader(http.StatusUnauthorized)
		return
	}

	delete(tokens, token)

	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println(err)
		return
	}

	connectedClient := ConnectedClient{
		Id:               token,
		Muted:            false,
		MicMuted:         false,
		AdminMuted:       false,
		SocketConnection: conn,
		PeerConnection:   &webrtc.PeerConnection{},
		DataChannels:     make(map[string]*webrtc.DataChannel),
		Room:             nil,
	}

	// enter client into first room
	for _, room := range server.Rooms {
		connectedClient.Room = room
		room.ConnectedClients[connectedClient.Id] = &connectedClient
		break
	}

	server.Clients[connectedClient.Id] = &connectedClient

	log.Println("Client connected", conn.RemoteAddr())

	defer func() {
		conn.Close()
		delete(server.Clients, connectedClient.Id)
	}()

	for {
		_, p, err := conn.ReadMessage()
		if err != nil {
			return
		}

		var message WebSocketMessage
		err = json.Unmarshal(p, &message)
		if err != nil {
			log.Println("Error unmarshalling message", err)
			continue
		}

		log.Println("Message received of type:", message.Type)

		if message.Type == "sdpOffer" {
			handleSdpOffer(&connectedClient, message)
		} else if message.Type == "iceCandidate" {
			handleIceCandidate(&connectedClient, message)
		}
	}
}
