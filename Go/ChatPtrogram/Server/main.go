package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"net"
	"sync"
)

// 클라이언트 구조체 정의
type Client struct {
	conn net.Conn
	name string
}

// 메시지 구조체 정의
type Message struct {
	Type    string      `json:"type"`
	Payload interface{} `json:"payload"`
	Sender  string      `json:"sender"`
}

// 클라이언트 이름 설정 페이로드
type SetNamePayload struct {
	Name string `json:"name"`
}

// 채팅 메시지 페이로드
type ChatMessagePayload struct {
	Message string `json:"message"`
}

var clients = make(map[*Client]bool)
var broadcast = make(chan Message)
var mutex = &sync.Mutex{}

func main() {
	listener, err := net.Listen("tcp", ":5000")
	if err != nil {
		fmt.Println("Error starting server:", err)
		return
	}
	defer listener.Close()
	fmt.Println("Server started on port 5000")

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
}

// 연결이 종료될 때까지, 수신
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
	case "SET_NAME":
		var payload SetNamePayload
		data, _ := json.Marshal(msg.Payload)
		json.Unmarshal(data, &payload)
		client.name = payload.Name

		// 아래 코드처럼 해도 되지만, 필드가 여러 개일 경우 위의 방법이 더 편리함.
		// type이 interface{}인 경우, 역직렬화 시 map[string]interface{}로 변환(conversion)된다.
		// client.name = msg.Payload.(map[string]interface{})["name"].(string)

		fmt.Println("Client set name:", client.name)

	case "CHAT_MESSAGE":
		var payload ChatMessagePayload
		data, _ := json.Marshal(msg.Payload)
		json.Unmarshal(data, &payload)
		fmt.Println("Received message from", client.name, ":", payload.Message)
		broadcast <- Message{
			Type: "CHAT_MESSAGE",
			Payload: ChatMessagePayload{
				Message: client.name + ": " + payload.Message,
			},
			Sender: client.name,
		}

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
			if client.name == msg.Sender {
				continue
			}
			_, err := client.conn.Write(data)
			if err != nil {
				fmt.Println("Error sending message to", client.name)
			}
		}
		mutex.Unlock()
	}
}
