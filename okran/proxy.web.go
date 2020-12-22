package main

import (
	"encoding/json"
	"io/ioutil"
	"net/http"
	"strconv"
	"strings"
)

type FuncHandler struct {
	f http.HandlerFunc
}

func (f FuncHandler) ServeHTTP(writer http.ResponseWriter, request *http.Request) {
	if f.f != nil {
		f.f(writer, request)
	} else {
		http.NotFound(writer, request)
	}
}

func (this Proxy) webListJavaProcess(w http.ResponseWriter, r *http.Request) {
	var java []JavaProcess
	var jar JarHelper
	var bs []byte
	var err error
	name := r.URL.Path
	if bs, err = ioutil.ReadAll(r.Body); err != nil {
		println(err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
	} else if jar, err = this.JarHelper(); err != nil {
		println(err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
	} else if java, err = jar.ListJavaProcesses(name); err != nil {
		println(err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
	} else if bs, err = json.Marshal(java); err != nil {
		println(err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
	w.Header().Add("Content-Type", "application/json")
	w.Header().Add("Content-Length", strconv.FormatInt(int64(len(bs)), 10))
	if _, err = w.Write(bs); err != nil {
		println(err.Error())
	}
}

func (this Proxy) webAttach(w http.ResponseWriter, r *http.Request) {
	var dst, jarFile, agentFile string
	args := strings.Split(r.URL.Path, "/")
	switch len(args) {
	case 1:
		dst = args[0]
	case 2:
		dst = args[0]
		jarFile = args[1]
	case 3:
		dst = args[0]
		jarFile = args[1]
		agentFile = args[2]
	default:
		http.Error(w, "args count is error!", http.StatusInternalServerError)
		return
	}

	cmd := &ProxyCmd{Cmd: "Attach", Dst: dst, JarFile: jarFile, AgentFile: agentFile}
	cmd.wg.Add(1)
	this.chanCmd <- cmd
	cmd.wg.Wait()
	if cmd.err != nil {
		http.Error(w, cmd.err.Error(), http.StatusInternalServerError)
		return
	}
}

func (this Proxy) webByteCode(w http.ResponseWriter, r *http.Request) {
	cmd := &ProxyCmd{Cmd: "bytecode"}
	cmd.wg.Add(1)
	this.chanCmd <- cmd
	cmd.wg.Wait()
	if cmd.err != nil {
		http.Error(w, cmd.err.Error(), http.StatusInternalServerError)
		return
	}
}
func (this Proxy) webRetransformClass(w http.ResponseWriter, r *http.Request) {
	cmd := &ProxyCmd{Cmd: "RetransformClass", Dst: r.URL.Path}
	cmd.wg.Add(1)
	this.chanCmd <- cmd
	cmd.wg.Wait()
	if cmd.err != nil {
		http.Error(w, cmd.err.Error(), http.StatusInternalServerError)
		return
	} else if len(cmd.ClassBytes) > 0 {
		_, _ = w.Write(cmd.ClassBytes)
	}
}
