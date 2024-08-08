package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"net"
	"sync"
)

type Client struct {
	conn     net.Conn
	playerId int
}

type Message struct {
	Type    string      `json:"type"`
	Payload interface{} `json:"payload"`
}

type ConnectPayload struct {
	PlayerId int `json:"playerId"`
}

type DisconnectPayload struct {
	PlayerId int `json:"playerId"`
}

type PlayerTransformPayload struct {
	PlayerId int       `json:"playerId"`
	Position []float64 `json:"position"`
	Rotation []float64 `json:"rotation"`
}

var (
	clients   = make(map[*Client]bool)
	broadcast = make(chan Message)
	mutex     = &sync.Mutex{}
)

func main() {
	listener, err := net.Listen("tcp", ":5001")
	if err != nil {
		fmt.Println("Error starting server:", err)
		return
	}
	defer listener.Close()
	fmt.Println("Server started on port 5001")

	go sendMessage()

	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("Error accepting connection:", err)
			continue
		}

		go handleConnection(conn)
	}
}

func handleConnection(conn net.Conn) {
	client := &Client{conn: conn}
	mutex.Lock()
	clients[client] = true
	mutex.Unlock()
	fmt.Println("New client connected:", conn.RemoteAddr().String())

	receiveMessage(client, conn)

	mutex.Lock()
	delete(clients, client)
	mutex.Unlock()
	conn.Close()
	fmt.Println("Client disconnected:", conn.RemoteAddr().String())
	broadcast <- Message{
		Type: "DISCONNECT",
		Payload: DisconnectPayload{
			PlayerId: client.playerId,
		},
	}
}

func receiveMessage(client *Client, conn net.Conn) {
	scanner := bufio.NewScanner(conn)
	for scanner.Scan() {
		var msg Message
		fmt.Println(string(scanner.Bytes()))
		err := json.Unmarshal(bytes.TrimPrefix(scanner.Bytes(), []byte{0xEF, 0xBB, 0xBF}), &msg)
		if err != nil {
			fmt.Println("Error parsing message:", err)
			continue
		}

		handleMessage(client, msg)
	}
}

func handleMessage(client *Client, msg Message) {
	switch msg.Type {
	case "CONNECT":
		var payload ConnectPayload
		data, _ := json.Marshal(msg.Payload)
		json.Unmarshal(data, &payload)
		client.playerId = payload.PlayerId
		//fmt.Println("Client set name:", client.playerId)
		broadcast <- Message{
			Type: "CONNECT",
			Payload: ConnectPayload{
				PlayerId: client.playerId,
			},
		}

	case "PLAYER_TRANSFORM":
		broadcast <- msg

	default:
		fmt.Println("Unknown message type:", msg.Type)
	}
}

func sendMessage() {
	for {
		msg := <-broadcast
		data, err := json.Marshal(msg)
		if err != nil {
			fmt.Println("Error marshaling message:", err)
			continue
		}
		data = append(data, '\n')
		mutex.Lock()
		for client := range clients {
			_, err := client.conn.Write(data)
			if err != nil {
				fmt.Println("Error sending message to", client.playerId)
			}
		}
		mutex.Unlock()
	}
}
