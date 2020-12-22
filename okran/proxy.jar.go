package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
)

type JarCmd struct {
	PID         string   `json:"pid,omitempty"`
	DisplayName string   `json:"displayName,omitempty"`
	WlsHome     string   `json:"wls.home,omitempty"`
	Classes     []string `json:"classes,omitempty"`
	Files       []string `json:"files,omitempty"`
	Process     []string `json:"process,omitempty"`

	Result int    `json:"result,omitempty"`
	Error  string `json:"error,omitempty"`
}
type JarHelper struct {
	JarFile string
}

func (this JarHelper) AbsJarFile() string {
	if jarFile, err := filepath.Abs(this.JarFile); err != nil {
		return this.JarFile
	} else {
		return jarFile
	}
}
func (this JarHelper) DefineClasses() (classes []string, err error) {
	var jarFile string
	if jarFile, err = filepath.Abs(this.JarFile); err != nil {
		return
	} else if _, err = os.Stat(jarFile); err != nil {
		return
	}
	cmd := exec.Command("java", "-jar", jarFile, "DefineClasses")
	var out bytes.Buffer
	cmd.Stdout = &out
	cmd.Stderr = &out
	if err = cmd.Start(); err != nil {
		return
	} else if err = cmd.Wait(); err != nil {
		err = errors.New(string(out.Bytes()))
		return
	}
	var ret JarCmd
	if err = json.Unmarshal(out.Bytes(), &ret); err != nil {
		return
	} else if ret.Result != 0 {
		err = errors.New(ret.Error)
	} else {
		classes = ret.Classes
	}
	return
}
func (this JarHelper) RedefineClasses() (classes []string, err error) {
	var jarFile string
	if jarFile, err = filepath.Abs(this.JarFile); err != nil {
		return
	} else if _, err = os.Stat(jarFile); err != nil {
		return
	}
	cmd := exec.Command("java", "-jar", jarFile, "RedefineClasses")
	var out bytes.Buffer
	cmd.Stdout = &out
	cmd.Stderr = &out
	if err = cmd.Start(); err != nil {
		return
	} else if err = cmd.Wait(); err != nil {
		err = errors.New(string(out.Bytes()))
		return
	}
	var ret JarCmd
	if err = json.Unmarshal(out.Bytes(), &ret); err != nil {
		return
	} else {
		classes = ret.Classes
	}
	return
}
func (this JarHelper) Bytecode(className, classFile, outDir string, others ...string) (file string, err error) {
	var jarFile string
	var cmd *exec.Cmd
	var out bytes.Buffer

	if jarFile, err = filepath.Abs(this.JarFile); err != nil {
		return
	} else if _, err = os.Stat(jarFile); err != nil {
		return
	}
	args := make([]string, 0, 10)
	args = append(args, "-jar")
	args = append(args, jarFile)
	args = append(args, "bytecode")
	args = append(args, className)
	args = append(args, classFile)
	args = append(args, outDir)
	args = append(args, others...)
	cmd = exec.Command("java", args...)
	cmd.Stdout = &out
	cmd.Stderr = &out
	if err = cmd.Start(); err != nil {
		return
	} else if err = cmd.Wait(); err != nil {
		return "", errors.New(string(out.Bytes()))
	}
	var ret JarCmd
	if err = json.Unmarshal(out.Bytes(), &ret); err != nil {
		return
	} else {
		file = filepath.Join(outDir, strings.ReplaceAll(className, ".", string(filepath.Separator))) + ".class"
	}
	return
}
func (this JarHelper) Attach(proxy *Proxy, pid, name, wlsHome, agentFile string) (agent *Agent, err error) {

	var logs string
	jarFile := this.AbsJarFile()
	if _, err = os.Stat(jarFile); err != nil {
		return
	} else if agentFile, err = filepath.Abs(agentFile); err != nil {
		return
	} else if _, err = os.Stat(agentFile); err != nil {
		return
	}
	if logs, err = filepath.Abs(proxy.LogDir); err != nil {
		return
	}
	args := "id=" + proxy.AgentId +
		";pid=" + pid +
		";name=" + name +
		";log=" + logs +
		";host=" + proxy.Host +
		";port=" + strconv.FormatInt(proxy.AgentPort, 10) +
		";url.host=" + proxy.Host +
		";url.port=" + strconv.FormatInt(proxy.Port, 10) +
		";url.uri=/" + proxy.Domain + "/traces" +
		";jarFile=" + jarFile +
		";agentFile=" + agentFile +
		";wls.home=" + wlsHome +
		";auth=b2tyYW46MXFhenhzdzI=" +
		";pool.maxsize=10000"
	cmd := exec.Command("java", "-jar", jarFile, "attach", pid, agentFile, args)
	var out bytes.Buffer
	cmd.Stdout = &out
	cmd.Stderr = &out
	if err = cmd.Start(); err != nil {
		return
	} else if err = cmd.Wait(); err != nil {
		err = errors.New(string(out.Bytes()))
		return
	}
	var ret JarCmd
	if err = json.Unmarshal(out.Bytes(), &ret); err != nil {
		err = errors.New("internal error at attach result json unmarshal")
	} else {
		agent = &Agent{
			PID:       ret.PID,
			Name:      ret.DisplayName,
			WlsHome:   ret.WlsHome,
			JarFile:   this.JarFile,
			AgentFile: agentFile,
		}
	}
	return
}
func (this JarHelper) ListJavaProcesses(name string) (process []JavaProcess, err error) {
	var jarFile string
	if jarFile, err = filepath.Abs(this.JarFile); err != nil {
		return
	} else if _, err = os.Stat(jarFile); err != nil {
		return
	}
	cmd := exec.Command("java", "-jar", jarFile, "list", name)
	var out bytes.Buffer
	cmd.Stdout = &out
	cmd.Stderr = &out
	if err = cmd.Start(); err != nil {
		return
	} else if err = cmd.Wait(); err != nil {
		err = errors.New(string(out.Bytes()))
	} else if err = json.Unmarshal(out.Bytes(), &process); err != nil {
		println(string(out.Bytes()))
		return
	}
	return
}
