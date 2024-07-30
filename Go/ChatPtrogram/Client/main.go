package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"net"
	"os"
	"strings"
)

// 메시지 구조체 정의 (서버 코드와 동일)
type Message struct {
	Type    string      `json:"type"`
	Payload interface{} `json:"payload"`
}

// 클라이언트의 이름 설정 페이로드
type SetNamePayload struct {
	Name string `json:"name"`
}

// 채팅 메시지 페이로드
type ChatMessagePayload struct {
	Message string `json:"message"`
}

func main() {
	// 서버에 연결
	conn, err := net.Dial("tcp", "localhost:8080")
	if err != nil {
		fmt.Println("Error connecting to server:", err)
		return
	}
	defer conn.Close()

	// 클라이언트 이름 설정
	fmt.Print("Enter your name: ")
	reader := bufio.NewReader(os.Stdin)
	name, _ := reader.ReadString('\n')
	name = strings.TrimSpace(name)
	setNameMessage := Message{
		Type: "SET_NAME",
		Payload: SetNamePayload{
			Name: name,
		},
	}
	sendMessage(conn, setNameMessage)

	// 메시지 수신 및 전송 고루틴 시작
	go receiveMessages(conn)

	// 사용자 입력 처리
	for {
		fmt.Print("Enter message (or /quit to exit): ")
		msgText, _ := reader.ReadString('\n')
		msgText = strings.TrimSpace(msgText)

		if msgText == "/quit" {
			break
		}

		chatMessage := Message{
			Type: "CHAT_MESSAGE",
			Payload: ChatMessagePayload{
				Message: msgText,
			},
		}
		sendMessage(conn, chatMessage)
	}
}

// 메시지를 서버로 전송
func sendMessage(conn net.Conn, msg Message) {
	data, err := json.Marshal(msg)
	if err != nil {
		fmt.Println("Error marshaling message:", err)
		return
	}
	data = append(data, '\n')
	_, err = conn.Write(data)
	if err != nil {
		fmt.Println("Error sending message:", err)
	}
}

// 서버로부터 메시지를 수신
func receiveMessages(conn net.Conn) {
	scanner := bufio.NewScanner(conn)
	for scanner.Scan() {
		var msg Message
		err := json.Unmarshal(scanner.Bytes(), &msg)
		if err != nil {
			fmt.Println("Error parsing received message:", err)
			continue
		}
		handleReceivedMessage(msg)
	}
}

// 수신된 메시지 처리
func handleReceivedMessage(msg Message) {
	switch msg.Type {
	case "CHAT_MESSAGE":
		var payload ChatMessagePayload
		data, _ := json.Marshal(msg.Payload)
		json.Unmarshal(data, &payload)
		fmt.Println(payload.Message)
	default:
		fmt.Println("Unknown message type:", msg.Type)
	}
}
