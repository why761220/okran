package main

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"sync"
)

func (this Proxy) recv(w http.ResponseWriter, r *http.Request) {
	var err error
	var bs []byte
	var data map[string][]map[string]interface{}
	if bs, err = ioutil.ReadAll(r.Body); err != nil {
		println(err.Error())
	} else if err = json.Unmarshal(bs, &data); err != nil {
		println(err.Error())
	} else {
		this.chanData <- data
	}
}

func (this Proxy) recvLoop(ctx context.Context, exited *sync.WaitGroup) {
	defer exited.Done()
	for {
		select {
		case <-ctx.Done():
			return
		case traces := <-this.chanData:
			for k := range traces {
				for i := range traces[k] {
					if bs, err := json.Marshal(traces[k][i]); err == nil {
						println(string(bs))
					}
				}
			}
		}
	}
}
