package main

import (
	"context"
	"log"
	"net"
	"os"
	"os/signal"
	"strings"
	"sync"
	"syscall"
	"time"
)

func LocalAddress() string {

	conn, err := net.Dial("udp", "88.88.88.88:80")
	if err != nil {
		log.Println(err.Error())
		return "127.0.0.1"
	}
	defer func() {
		if err := conn.Close(); err != nil {
			println(err.Error())
		}
	}()
	return strings.Split(conn.LocalAddr().String(), ":")[0]
}

func main() {
	exited := sync.WaitGroup{}
	ctx, exit := context.WithCancel(context.Background())
	pArgs := ProxyArgs{
		Url:       "http://localhost:8080/okran",
		AgentId:   "100",
		Host:      LocalAddress(),
		Port:      8888,
		AgentPort: 18081,
		Domain:    "okran",
		DstDir:    ".",
		SrcDir:    ".",
		LogDir:    "./logs",
		JarFile:   "attach.jar",
		Auto:      false,
		AgentFile: "C:/Users/why76/OneDrive/src/OkranAgent/x64/Debug/OkranAgent.dll",
	}
	if err := StartProxy(ctx, &exited, pArgs); err != nil {
		log.Fatal(err)
	}
	log.Println("start at:" + time.Now().String())
	sig := make(chan os.Signal, 1)
	signal.Notify(sig, syscall.SIGINT, syscall.SIGTERM, syscall.SIGKILL)
	select {
	case <-sig:
		exit()
	}
	exited.Wait()
	log.Println("exit at:" + time.Now().String())
}
