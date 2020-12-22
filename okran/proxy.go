package main

import (
	"bytes"
	"context"
	"errors"
	"io/ioutil"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"sync"
	"time"
)

type JavaProcess struct {
	PID        string `json:"pid,omitempty"`
	Name       string `json:"name,omitempty"`
	ListenPort int64  `json:"listenPort,omitempty"`
	LogFile    string `json:"logFile,omitempty"`
	JarFile    string `json:"jarFile,omitempty"`
	AgentFile  string `json:"agentFile,omitempty"`
	WlsHome    string `json:"wls.home,omitempty"`
	ExtDirs    string `json:"extDirs,omitempty"`
	//AgentProperties  map[string]string `json:"agentProperties,omitempty"`
	//SystemProperties map[string]string `json:"systemProperties,omitempty"`
}

type ProxyCmd struct {
	Cmd         string
	Dst         string
	JarFile     string
	AgentFile   string
	Force       bool
	JavaProcess []JavaProcess
	ClassBytes  []byte
	err         error
	wg          sync.WaitGroup
}

type ProxyArgs struct {
	Url        string
	AgentId    string
	Dst        string
	JavaServer string
	Port       int64
	Host       string
	AgentPort  int64
	ListenPort int64
	JarFile    string
	AgentFile  string
	Domain     string
	DstDir     string
	SrcDir     string
	LogDir     string
	Auto       bool
}
type Proxy struct {
	ProxyArgs
	Agents        map[string]*Agent
	chanCmd       chan *ProxyCmd
	chanConnected chan *Agent
	chanData      chan map[string][]map[string]interface{}
}

func StartProxy(ctx context.Context, exited *sync.WaitGroup, args ProxyArgs) error {
	var err error
	var wg sync.WaitGroup
	var pattern string
	if err := os.MkdirAll(args.DstDir, os.ModePerm); err != nil {
		return err
	}
	if err := os.MkdirAll(args.SrcDir, os.ModePerm); err != nil {
		return err
	}
	if err := os.MkdirAll(args.LogDir, os.ModePerm); err != nil {
		return err
	}

	this := Proxy{
		ProxyArgs:     args,
		Agents:        make(map[string]*Agent),
		chanCmd:       make(chan *ProxyCmd, 10),
		chanData:      make(chan map[string][]map[string]interface{}, 1000),
		chanConnected: make(chan *Agent),
	}
	mux := http.NewServeMux()
	pattern = "/" + this.Domain + "/traces"
	mux.HandleFunc(pattern, this.recv)
	pattern = "/" + this.Domain + "/list/java/process/"
	mux.Handle(pattern, http.StripPrefix(pattern, FuncHandler{f: this.webListJavaProcess}))
	pattern = "/" + this.Domain + "/attach/"
	mux.Handle(pattern, http.StripPrefix(pattern, FuncHandler{f: this.webAttach}))
	pattern = "/" + this.Domain + "/bytecode"
	mux.HandleFunc(pattern, this.webByteCode)
	pattern = "/" + this.Domain + "/RetransformClass/"
	mux.Handle(pattern, http.StripPrefix(pattern, FuncHandler{f: this.webRetransformClass}))
	srv := http.Server{Addr: ":" + strconv.FormatInt(this.Port, 10), Handler: mux}
	wg.Add(1)
	go func() {
		defer wg.Done()
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			println(err.Error())
		}
	}()
	exited.Add(1)
	go func() {
		defer exited.Done()
		for {
			select {
			case <-ctx.Done():
				if err := srv.Shutdown(context.Background()); err != nil {
					println(err.Error())
				}
				return
			}
		}
	}()
	exited.Add(1)
	go this.mainLoop(ctx, exited)
	exited.Add(1)
	go this.recvLoop(ctx, exited)
	exited.Add(1)
	go this.listenLoop(ctx, exited)
	return err
}

func (this *Proxy) mainLoop(ctx context.Context, exited *sync.WaitGroup) {
	defer exited.Done()
	ti := time.NewTicker(time.Second * 1)
	for {
		select {
		case <-ctx.Done():
			for i := range this.Agents {
				this.Agents[i].Close()
			}
			this.Agents = nil
			return
		case agent := <-this.chanConnected:
			if old, ok := this.Agents[agent.PID]; ok {
				if old.conn != nil {
					old.Close()
				}
			}
			if err := agent.Start(); err == nil {
				this.Agents[agent.PID] = agent
			} else {
				agent.Close()
			}
		case cmd := <-this.chanCmd:
			this.doCmds(cmd)
			cmd.wg.Done()
		case <-ti.C:
			if this.Auto && this.Dst != "" && this.JavaServer != "" {
				if err := this.AutoAttach(this.JavaServer); err != nil {
					println(err.Error())
				}
			}
		}
	}
}

func (this Proxy) doCmds(cmd *ProxyCmd) {
	var jar JarHelper
	switch cmd.Cmd {
	case "Attach":
		var list []JavaProcess
		if jar, cmd.err = this.JarHelper(); cmd.err != nil {
			return
		} else if list, cmd.err = jar.ListJavaProcesses(cmd.Dst); cmd.err != nil {
			return
		}
		for _, jvm := range list {
			if agent, ok := this.Agents[jvm.PID]; ok {
				continue
			} else if agent, cmd.err = jar.Attach(&this, jvm.PID, jvm.Name, jvm.WlsHome, this.AgentFile); cmd.err != nil {
				return
			} else {
				this.Agents[agent.PID] = agent
			}
		}
		return
	case "list":
		if jar, cmd.err = this.JarHelper(); cmd.err == nil {
			cmd.JavaProcess, cmd.err = jar.ListJavaProcesses(cmd.Dst)
		}
		return
	case "RetransformClass":
		if agent, ok := this.Agents[cmd.Dst]; ok {
			cmd.ClassBytes, cmd.err = agent.RetransformClass(cmd.Dst)
			return
		} else {
			for k := range this.Agents {
				if this.Agents[k].Name == cmd.Dst {
					cmd.ClassBytes, cmd.err = agent.RetransformClass(cmd.Dst)
					return
				}
			}
			cmd.err = errors.New("agent for " + cmd.Dst + " not found")
			return
		}
	default:
		cmd.err = errors.New("unrecognized command")
	}
	return
}

func (this Proxy) JavaVersion() (string, error) {
	cmd := exec.Command("java", "-version")
	var out bytes.Buffer
	cmd.Stdout = &out
	cmd.Stderr = &out
	if err := cmd.Start(); err != nil {
		return "", errors.New(out.String())
	} else if err = cmd.Wait(); err != nil {
		return "", errors.New(out.String())
	} else {
		return out.String(), nil
	}
}
func (this Proxy) JarHelper() (jar JarHelper, err error) {
	var bs []byte
	var resp *http.Response
	if jar.JarFile = this.JarFile; jar.JarFile == "" {
		jar.JarFile = "attach.jar"
	}
	if jar.JarFile, err = filepath.Abs(jar.JarFile); err != nil {
		return
	} else if _, err = os.Stat(jar.JarFile); err == nil {
		return
	}
	if resp, err = http.Get(this.Url + "/files/" + filepath.Base(jar.JarFile)); err != nil {
		return
	}
	defer resp.Body.Close()
	if bs, err = ioutil.ReadAll(resp.Body); err != nil {
		return
	} else if resp.StatusCode != 200 {
		err = errors.New(string(bs))
		return
	} else if err = ioutil.WriteFile(jar.JarFile, bs, os.ModePerm); err != nil {
		return
	}
	return
}
func (this Proxy) GetAgentFile(name string) (agentFile string, err error) {
	if agentFile = this.AgentFile; agentFile == "" {
		switch runtime.GOOS {
		case "linux":
			agentFile = name + ".linux." + runtime.GOARCH + ".so"
		case "arm":
			agentFile = name + ".arm." + runtime.GOARCH + ".so"
		case "windows":
			agentFile = name + ".windows." + runtime.GOARCH + ".dll"
		default:
			return "", errors.New("unknown os name")
		}
	}
	if agentFile, err = filepath.Abs(agentFile); err != nil {
		return
	} else if _, err = os.Stat(name); err == nil {
		return
	}
	var bs []byte
	var resp *http.Response
	if resp, err = http.Get(this.Url + "/files/" + filepath.Base(agentFile)); err != nil {
		return
	}
	defer resp.Body.Close()
	if bs, err = ioutil.ReadAll(resp.Body); err != nil {
		return
	} else if resp.StatusCode != http.StatusOK {
		err = errors.New(string(bs))
	} else if err = ioutil.WriteFile(agentFile, bs, os.ModePerm); err != nil {
		return
	}
	return
}

func (this *Proxy) AutoAttach(server string) (err error) {
	var jar JarHelper
	var agentFile string
	var list []JavaProcess
	if jar, err = this.JarHelper(); err != nil {
		return
	} else if list, err = jar.ListJavaProcesses(server); err != nil {
		return
	}
	for _, jvm := range list {
		if agent, ok := this.Agents[jvm.PID]; !ok {
			if agentFile, err = this.GetAgentFile(server); err != nil {
				return
			} else if agent, err = jar.Attach(this, jvm.PID, jvm.Name, jvm.WlsHome, agentFile); err != nil {
				return
			} else {
				this.Agents[agent.PID] = agent
			}
		}
	}
	return
}
