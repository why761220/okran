package main

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"io/ioutil"
	"net"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"
)

type AgentCmd struct {
	Cmd       string `json:"cmd,omitempty"`
	ID        string `json:"id,omitempty"`
	Name      string `json:"name,omitempty"`
	PID       string `json:"pid,omitempty"`
	JarFile   string `json:"jarFile,omitempty"`
	AgentFile string `json:"agentFile,omitempty"`
	ClassName string `json:"className,omitempty"`
	WlsHome   string `json:"wls.home,omitempty"`
	Size      int    `json:"size,omitempty"`
	Bytes     []byte `json:"body,omitempty"`
	Result    int    `json:"result,omitempty"`
	Error     string `json:"error,omitempty"`
	wg        *sync.WaitGroup
}
type Agent struct {
	proxy     *Proxy
	PID       string
	Name      string
	started   bool
	JarFile   string
	AgentFile string
	WlsHome   string
	conn      net.Conn
}

func (this *Proxy) listenLoop(ctx context.Context, exited *sync.WaitGroup) {
	defer exited.Done()
	var err error
	var agent *Agent
	var client net.Conn
	var listener net.Listener
	if listener, err = net.Listen("tcp", ":"+strconv.FormatInt(this.AgentPort, 10)); err != nil {
		println(err.Error())
		return
	}
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()
		for {
			if client, err = listener.Accept(); err != nil {
				println(err.Error())
			} else if agent, err = ConnectAgent(this, client); err != nil {
				println(err.Error())
				_ = client.Close()
			} else {
				this.chanConnected <- agent
			}
		}
	}()
	go func() {
		for {
			select {
			case <-ctx.Done():
				if listener != nil {
					_ = listener.Close()
					wg.Wait()
				}
			}
		}
	}()
}
func ConnectAgent(proxy *Proxy, conn net.Conn) (agent *Agent, err error) {
	cmd := AgentCmd{Cmd: "Heartbeat"}
	j := json.NewDecoder(conn)
	if err = conn.SetWriteDeadline(time.Now().Add(time.Second * 10)); err != nil {
		return
	} else if _, err = conn.Write([]byte(cmd.String())); err != nil {
		return
	}
	if err = conn.SetReadDeadline(time.Now().Add(time.Second * 10)); err != nil {
		return
	} else if err = j.Decode(&cmd); err != nil {
		return
	} else if cmd.Error != "" {
		err = errors.New(cmd.Error)
	} else {
		agent = &Agent{proxy: proxy, Name: cmd.Name, PID: cmd.PID, JarFile: cmd.JarFile, AgentFile: cmd.AgentFile, WlsHome: cmd.WlsHome, conn: conn}
	}
	return
}

func (this *Agent) closed(err error) bool {
	err = errors.Unwrap(err)
	if e1, ok := err.(*os.SyscallError); ok {
		if code, ok := e1.Err.(syscall.Errno); ok {
			if code == 10054 {
				return true
			}
		}
	}
	return false
}

func (this AgentCmd) String() string {
	sb := bytes.NewBuffer(nil)
	sb.WriteString("cmd=" + this.Cmd)
	if this.ID != "" {
		sb.WriteString(";id=" + this.ID)
	}
	if this.ClassName != "" {
		sb.WriteString(";className=" + this.ClassName)
	}

	if len(this.Bytes) > 0 {
		sb.WriteString(";size=" + strconv.FormatInt(int64(len(this.Bytes)), 10))
	}
	sb.WriteString("\r\n\r\n")
	return sb.String()
}
func (this Agent) SendCmd(cmd *AgentCmd) (err error) {
	j := json.NewDecoder(this.conn)
	if err = this.conn.SetWriteDeadline(time.Now().Add(time.Second * 10000)); err != nil {
		return
	} else if _, err = this.conn.Write([]byte(cmd.String())); err != nil {
		return
	}
	if len(cmd.Bytes) > 0 {
		if err = this.conn.SetWriteDeadline(time.Now().Add(time.Second * 10000)); err != nil {
			return
		} else if _, err = this.conn.Write(cmd.Bytes); err != nil {
			return
		}
	}
	if err = this.conn.SetReadDeadline(time.Now().Add(time.Second * 10000)); err != nil {
		return
	} else if err = j.Decode(&cmd); err != nil {
		return
	}
	if cmd.Size > 0 {
		var n, m int
		cmd.Bytes = make([]byte, cmd.Size)
		n, _ = j.Buffered().Read(cmd.Bytes)
		if err = this.conn.SetReadDeadline(time.Now().Add(time.Second * 10)); err != nil {
			return
		} else if m, err = this.conn.Read(cmd.Bytes[n:]); err != nil {
			return
		} else if (n + m) != len(cmd.Bytes) {
			err = errors.New("recv size is error")
		}
	}
	return
}

func (this *Agent) Close() {
	if this.conn != nil {
		_ = this.conn.Close()
		this.conn = nil
	}
}
func (this *Agent) Heartbeat() (err error) {
	id := strconv.FormatInt(time.Now().UnixNano(), 10)
	cmd := AgentCmd{Cmd: "Heartbeat", ID: id}
	if err = this.SendCmd(&cmd); err != nil {
		return
	} else if cmd.Result != 200 {
		err = errors.New("heartbeat is error")
	} else {
		this.JarFile = cmd.JarFile
		this.Name = cmd.Name
		this.WlsHome = cmd.WlsHome
	}
	return
}
func (this *Agent) DefineClass(name string, bs []byte) (err error) {
	cmd := AgentCmd{
		Cmd:       "DefineClass",
		ClassName: name,
		Bytes:     bs,
	}
	if err = this.SendCmd(&cmd); err != nil {
		return
	} else if cmd.Result != 200 {
		err = errors.New(cmd.Error)
	}
	return
}
func (this *Agent) RedefineClass(name string, bs []byte) (err error) {
	cmd := AgentCmd{
		Cmd:       "RedefineClass",
		ClassName: strings.ReplaceAll(name, ".", "/"),
		Bytes:     bs,
	}
	if err = this.SendCmd(&cmd); err != nil {
		return
	} else if cmd.Result != 200 {
		err = errors.New(cmd.Error)
	}
	return
}

func (this *Agent) RetransformClass(name string) (bs []byte, err error) {
	cmd := AgentCmd{
		Cmd:       "RetransformClass",
		ClassName: strings.ReplaceAll(name, ".", "/"),
		Bytes:     bs,
	}
	if err = this.SendCmd(&cmd); err != nil {
		return
	} else if cmd.Result != 200 {
		err = errors.New(cmd.Error)
	} else if cmd.Size != len(cmd.Bytes) {
		err = errors.New("bytes length is error")
	} else {
		bs = cmd.Bytes
	}
	return
}
func (this *Agent) DstDir() string {
	a := filepath.Join(".", "classes", "dst", this.PID)
	if b, err := filepath.Abs(a); err == nil {
		a = b
	}
	_ = os.MkdirAll(a, os.ModePerm)
	return a
}
func (this *Agent) SrcDir() string {
	a := filepath.Join(".", "classes", "src", this.PID)
	if b, err := filepath.Abs(a); err == nil {
		a = b
	}
	_ = os.MkdirAll(a, os.ModePerm)
	return a
}
func (this *Agent) Start() (err error) {
	if this.started {
		return nil
	}
	var bs []byte
	var file string
	var classes []string
	jar := JarHelper{JarFile: this.JarFile}
	wlsHome := this.WlsHome
	if t, err := filepath.Abs(wlsHome); err == nil {
		wlsHome = t
	}

	if classes, err = jar.DefineClasses(); err != nil {
		return
	}
	for i := range classes {
		if file, err = jar.Bytecode(classes[i], "", this.DstDir(), this.proxy.AgentId, this.PID, this.proxy.Host); err != nil {
			return
		} else if bs, err = ioutil.ReadFile(file); err != nil {
			return
		} else if err = this.DefineClass(classes[i], bs); err != nil {
			return
		}
	}
	if classes, err = jar.RedefineClasses(); err != nil {
		return
	}

	for i := range classes {
		var dstFile string
		classFile := filepath.Join(this.SrcDir(), classes[i]) + ".class"
		if bs, err = this.RetransformClass(classes[i]); err != nil {
			return
		} else if err = ioutil.WriteFile(classFile, bs, os.ModePerm); err != nil {
			return
		} else if dstFile, err = jar.Bytecode(classes[i], classFile, this.DstDir(), wlsHome, this.proxy.AgentId, this.PID, this.proxy.Host); err != nil {
			return
		} else if bs, err = ioutil.ReadFile(dstFile); err != nil {
			return
		} else if err = this.RedefineClass(classes[i], bs); err != nil {
			return
		}
	}
	this.started = true
	return
}
func (this *Agent) Disconnect() {
	_ = this.SendCmd(&AgentCmd{Cmd: "Disconnect"})
	this.Close()
	return
}

func (this *Agent) Connected() bool {
	if this.conn != nil {
		if err := this.conn.SetReadDeadline(time.Now().Add(time.Second)); err != nil {
			return false
		} else if _, err = this.conn.Read(nil); err != nil {
			if e, ok := err.(net.Error); ok && e.Timeout() {
				return true
			} else {
				return false
			}
		} else {
			return true
		}
	} else {
		return false
	}
}
