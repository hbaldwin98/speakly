package main

import (
	"encoding/json"
	"fmt"
	"log"

	"github.com/gorilla/websocket"
	"github.com/pion/webrtc/v3"
)

type Server struct {
	Rooms   map[string]*Room
	Clients map[string]*ConnectedClient
}

type Room struct {
	Id               string
	Name             string
	ConnectedClients map[string]*ConnectedClient
	Settings         RoomSettings
}

type RoomSettings struct {
	PrioritySpeaker *ConnectedClient
}

type ConnectedClient struct {
	Id               string
	Muted            bool
	MicMuted         bool
	AdminMuted       bool
	SocketConnection *websocket.Conn
	PeerConnection   *webrtc.PeerConnection
	DataChannels     map[string]*webrtc.DataChannel
	Room             *Room
}

type SdpOffer struct {
	Sdp  string `json:"sdp"`
	Type string `json:"type"`
}

type ConnectRequest struct {
	SdpOffer SdpOffer `json:"sdpOffer"`
}

type WebSocketMessage struct {
	Type string `json:"type"`
	Data string `json:"data"`
}

func handleSdpOffer(client *ConnectedClient, message WebSocketMessage) {
	log.Println("Received offer from client", client.Id)
	sdpOffer := message.Data

	log.Println("Received SDP offer from client", client.Id)
	config := webrtc.Configuration{
		ICEServers: []webrtc.ICEServer{},
	}
	peerConnection, err := webrtc.NewPeerConnection(config)
	if err != nil {
		log.Println("Error creating peer connection", err)
		return
	}

	client.PeerConnection = peerConnection

	remoteOffer := webrtc.SessionDescription{
		Type: webrtc.SDPTypeOffer,
		SDP:  sdpOffer,
	}

	err = peerConnection.SetRemoteDescription(remoteOffer)
	if err != nil {
		log.Println("Error setting remote description", err)
		return
	}

	answer, err := peerConnection.CreateAnswer(nil)
	if err != nil {
		log.Println("Error creating answer", err)
		return
	}

	err = peerConnection.SetLocalDescription(answer)
	if err != nil {
		log.Println("Error setting local description", err)
		return
	}

	sdpAnswer := answer.SDP
	log.Println("Sending SDP answer to client", client.Id)
	data := map[string]string{"type": "sdpAnswer", "data": sdpAnswer}
	jsonData, err := json.Marshal(data)
	if err != nil {
		log.Println("Error marshalling SDP answer", err)
		return
	}

	err = client.SocketConnection.WriteMessage(websocket.TextMessage, jsonData)
	if err != nil {
		log.Println("Error sending SDP answer", err)
		return
	}

	peerConnection.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
		log.Println("Connection state changed", state)
		if state == webrtc.PeerConnectionStateDisconnected {
			onClientDisconnect(client)
		}
	})

	peerConnection.CreateDataChannel("myDataChannel", nil)
	peerConnection.CreateDataChannel("voip", nil)
	peerConnection.OnDataChannel(func(dataChannel *webrtc.DataChannel) {
		log.Println("Setting up channel:", dataChannel.Label(), "for client:", client.Id)
		client.DataChannels[dataChannel.Label()] = dataChannel

		if dataChannel.Label() == "voip" {
			dataChannel.OnMessage(func(msg webrtc.DataChannelMessage) {
				//log.Println(fmt.Sprintf("Message received on voip data channel of size %d", len(msg.Data)))

				for _, client := range client.Room.ConnectedClients {
					channel := client.DataChannels["voip"]

					if channel == nil {
						continue
					}

					err := client.DataChannels["voip"].Send(msg.Data)
					if err != nil {
						log.Println("Error sending voice packet", err)
						return
					}
				}
			})
		}
	})

	peerConnection.OnICECandidate(func(candidate *webrtc.ICECandidate) {
		if candidate == nil {
			log.Println("ICE candidate gathering complete")
			return
		}

		sendICECandidate(client, candidate)
	})
}

func onClientDisconnect(client *ConnectedClient) {
	if client == nil || client.SocketConnection == nil {
		log.Println("No client found for connection")
		return
	}

	log.Println("Client disconnected", client.Id)

	for _, peerClient := range client.Room.ConnectedClients {
		if peerClient.SocketConnection != nil {
			peerClient.SocketConnection.WriteMessage(websocket.TextMessage, []byte(fmt.Sprintf("{\"type\": \"clientDisconnected\", \"data\": \"%s\"}", client.Id)))
		}
	}

	for _, dataChannel := range client.DataChannels {
		if dataChannel == nil {
			continue
		}

		dataChannel.Close()
	}

	if client.PeerConnection != nil {
		client.PeerConnection.Close()
	}

	if client.SocketConnection != nil {
		client.SocketConnection.Close()
	}

	delete(client.Room.ConnectedClients, client.Id)
	delete(server.Clients, client.Id)
}
func sendICECandidate(client *ConnectedClient, candidate *webrtc.ICECandidate) {
	peerConnection := client.PeerConnection

	if peerConnection == nil {
		log.Println("No peer connection found")
		return
	}

	iceCandidateInit := webrtc.ICECandidateInit{
		Candidate:        candidate.ToJSON().Candidate,
		SDPMid:           candidate.ToJSON().SDPMid,
		SDPMLineIndex:    candidate.ToJSON().SDPMLineIndex,
		UsernameFragment: candidate.ToJSON().UsernameFragment,
	}

	iceCandidateInitJson, err := json.Marshal(iceCandidateInit)
	if err != nil {
		log.Println("Error marshalling ICE candidate init", err)
		return
	}

	iceCandidate := map[string]string{
		"type": "iceCandidate",
		"data": "",
	}
	iceCandidate["data"] = string(iceCandidateInitJson)

	iceCandidateJson, err := json.Marshal(iceCandidate)
	if err != nil {
		log.Println("Error marshalling ICE candidate", err)
		return
	}

	client.SocketConnection.WriteMessage(websocket.TextMessage, iceCandidateJson)
}

func handleIceCandidate(client *ConnectedClient, message WebSocketMessage) {
	log.Println("Received ice candidate from client", client.Id)
	var iceCandidate webrtc.ICECandidateInit
	err := json.Unmarshal([]byte(message.Data), &iceCandidate)
	if err != nil {
		log.Println("Error unmarshalling iceCandidate", err)
		return
	}

	peerConnection := client.PeerConnection
	if peerConnection == nil {
		log.Println("No peer connection found for client", client.Id)
		return
	}

	err = peerConnection.AddICECandidate(iceCandidate)
	if err != nil {
		log.Println("Error adding ice candidate", err)
		return
	}

	log.Println("Added new ice candidate")
}
