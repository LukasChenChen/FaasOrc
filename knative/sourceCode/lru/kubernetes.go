// Copyright 2022 Google Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"net/url"
	"strconv"
	"strings"
	"time"
)

var (
	apiHost             = "10.154.0.20:6443"
	bindingsEndpoint    = "/api/v1/namespaces/default/pods/%s/binding/"
	eventsEndpoint      = "/api/v1/namespaces/default/events"
	nodesEndpoint       = "/api/v1/nodes"
	podsEndpoint        = "/api/v1/pods"
	watchPodsEndpoint   = "/api/v1/watch/pods"
	defaultPodsEndPoint = "/api/v1/namespaces/default/pods"
	deploymentEndpoint  = "/apis/apps/v1/namespaces/default/deployments"
	knativeSvcEndpoint  = "/apis/serving.knative.dev/v1/namespaces/default/services"
	// token = "eyJhbGciOiJSUzI1NiIsImtpZCI6IkduSXA4dG9BZExKMmExUXpNcWcwSG9QWHJFc0ZWZFZUTzFVdlVKWkh0OUUifQ.eyJpc3MiOiJrdWJlcm5ldGVzL3NlcnZpY2VhY2NvdW50Iiwia3ViZXJuZXRlcy5pby9zZXJ2aWNlYWNjb3VudC9uYW1lc3BhY2UiOiJkZWZhdWx0Iiwia3ViZXJuZXRlcy5pby9zZXJ2aWNlYWNjb3VudC9zZWNyZXQubmFtZSI6ImRlZmF1bHQtdG9rZW4iLCJrdWJlcm5ldGVzLmlvL3NlcnZpY2VhY2NvdW50L3NlcnZpY2UtYWNjb3VudC5uYW1lIjoiZGVmYXVsdCIsImt1YmVybmV0ZXMuaW8vc2VydmljZWFjY291bnQvc2VydmljZS1hY2NvdW50LnVpZCI6ImQ0ZmE4ZGE2LThkOTktNDc0My1hOTk0LTAxODU3N2JlMGZkMSIsInN1YiI6InN5c3RlbTpzZXJ2aWNlYWNjb3VudDpkZWZhdWx0OmRlZmF1bHQifQ.IXqZ2bfsqN41VWiQWvnRfi3SguSiBBBbYMjrpje4wcXMvnfdRzmJt9DepVFGfsgPEnGVuLGBAjeerStjj5GuREfXb8zlvnp_APLsDQLvZCR8ErsCZuvgR63DVW3p_Cl9K5clKu2ZzQiaTUA3J49b6xRdKKG8WHwuhxpt1hIvHmTRXciTtAdCnMr6DkFRu2WZ2aONzctTTn4LhSv2Ze7_6lAF7VKCUzOT2ZdBbkpB1p510s5vxRGyWkcDmLTFru65kw6prphPlR2DpkzzxGWcDFmyXQ6zEuJH9RPMRqVjJb0EsCzZ4wsP1BBWdM7OyVBv4eQYL7ISfjeXxlfKqKrrmw"
    trafficGenEndpoint = "10.154.0.20:2222"
)

func postEvent(event Event) error {
	var b []byte
	body := bytes.NewBuffer(b)
	err := json.NewEncoder(body).Encode(event)
	if err != nil {
		return err
	}


	request := &http.Request{
		Body:          ioutil.NopCloser(body),
		ContentLength: int64(body.Len()),
		Header:        make(http.Header),
		Method:        http.MethodPost,
		URL: &url.URL{
			Host:   apiHost,
			Path:   eventsEndpoint,
			Scheme: "https",	
		},
	}
	request.Header.Set("Content-Type", "application/json")
	
	bearer := "Bearer " + config_G.Token
	request.Header.Add("Authorization", bearer)
	resp, err := http.DefaultClient.Do(request)


	if err != nil {
		return err
	}
	if resp.StatusCode != 201 {
		log.Println("token %s", config_G.Token)
		return errors.New("Event: Unexpected HTTP status code" + resp.Status)
	}
	return nil
}

func getNodes() (*NodeList, error) {
	var nodeList NodeList

	request := &http.Request{
		Header: make(http.Header),
		Method: http.MethodGet,
		URL: &url.URL{
			Host:   apiHost,
			Path:   nodesEndpoint,
			Scheme: "https",
		},
	}
	request.Header.Set("Accept", "application/json, */*")

	bearer := "Bearer " + config_G.Token
	request.Header.Add("Authorization", bearer)
	resp, err := http.DefaultClient.Do(request)
	if err != nil {
		return nil, err
		log.Println("getNodes failed")
	}

	err = json.NewDecoder(resp.Body).Decode(&nodeList)
	if err != nil {
		log.Println("token %s", config_G.Token)
		return nil, err
	}

	return &nodeList, nil
}

func watchUnscheduledPods() (<-chan Pod, <-chan error) {
	log.Println("Starting watchUnscheduledPods...")
	pods := make(chan Pod)
	errc := make(chan error, 1)

	v := url.Values{}
	v.Set("fieldSelector", "spec.nodeName=")

	request := &http.Request{
		Header: make(http.Header),
		Method: http.MethodGet,
		URL: &url.URL{
			Host:     apiHost,
			Path:     watchPodsEndpoint,
			RawQuery: v.Encode(),
			Scheme:   "https",
		},
	}
	request.Header.Set("Accept", "application/json, */*")

	bearer := "Bearer " + config_G.Token
	request.Header.Add("Authorization", bearer)
        
	go func() {
		for {
	            resp, err := http.DefaultClient.Do(request)
			if err != nil {
				errc <- err
				time.Sleep(5 * time.Second)
				continue
			}

			if resp.StatusCode != 200 {
				errc <- errors.New("Invalid status code: " + resp.Status)
				time.Sleep(5 * time.Second)
				continue
			}

			decoder := json.NewDecoder(resp.Body)
			for {
				var event PodWatchEvent
				err = decoder.Decode(&event)
				if err != nil {
					errc <- err
					break
				}

				if event.Type == "ADDED" {
					pods <- event.Object
				}
			}
		}
	}()

	return pods, errc
}

func getUnscheduledPods() ([]*Pod, error) {
	var podList PodList
	unscheduledPods := make([]*Pod, 0)

	v := url.Values{}
	v.Set("fieldSelector", "spec.nodeName=")

	request := &http.Request{
		Header: make(http.Header),
		Method: http.MethodGet,
		URL: &url.URL{
			Host:     apiHost,
			Path:     podsEndpoint,
			RawQuery: v.Encode(),
			Scheme:   "https",
		},
	}
	request.Header.Set("Accept", "application/json, */*")

	bearer := "Bearer " + config_G.Token
	request.Header.Add("Authorization", bearer)
	resp, err := http.DefaultClient.Do(request)
	if err != nil {
		log.Println("token %s", config_G.Token)
		log.Println("getUnscheduledPods failed 1")
		return unscheduledPods, err
	}
	err = json.NewDecoder(resp.Body).Decode(&podList)
	if err != nil {
		log.Println("getUnscheduledPods failed 2")
		return unscheduledPods, err
	}

	for _, pod := range podList.Items {
		if pod.Metadata.Annotations["scheduler.alpha.kubernetes.io/name"] == schedulerName {
			unscheduledPods = append(unscheduledPods, &pod)
		}
	}

	return unscheduledPods, nil
}

func getPods() (*PodList, error) {
	var podList PodList

	v := url.Values{}
	v.Add("fieldSelector", "status.phase=Running")
	v.Add("fieldSelector", "status.phase=Pending")

	request := &http.Request{
		Header: make(http.Header),
		Method: http.MethodGet,
		URL: &url.URL{
			Host:     apiHost,
			Path:     podsEndpoint,
			RawQuery: v.Encode(),
			Scheme:   "https",
		},
	}
	request.Header.Set("Accept", "application/json, */*")

	bearer := "Bearer " + config_G.Token
	request.Header.Add("Authorization", bearer)
	resp, err := http.DefaultClient.Do(request)
	if err != nil {
		log.Println("getPods failed 1")
		return nil, err
	}
	err = json.NewDecoder(resp.Body).Decode(&podList)
	if err != nil {
		log.Println("getPods failed 2")
		return nil, err
	}
	return &podList, nil
}

type ResourceUsage struct {
	CPU int
}

func fit(pod *Pod) ([]Node, error) {
	nodeList, err := getNodes()
	if err != nil {
		return nil, err
	}

	podList, err := getPods()
	if err != nil {
		return nil, err
	}

	resourceUsage := make(map[string]*ResourceUsage)
	for _, node := range nodeList.Items {
		resourceUsage[node.Metadata.Name] = &ResourceUsage{}
	}

	for _, p := range podList.Items {
		if p.Spec.NodeName == "" {
			continue
		}
		for _, c := range p.Spec.Containers {
			if strings.HasSuffix(c.Resources.Requests["cpu"], "m") {
				milliCores := strings.TrimSuffix(c.Resources.Requests["cpu"], "m")
				cores, err := strconv.Atoi(milliCores)
				if err != nil {
					return nil, err
				}
				ru := resourceUsage[p.Spec.NodeName]
				ru.CPU += cores
			}
		}
	}

	var nodes []Node
	fitFailures := make([]string, 0)

	var spaceRequired int
	for _, c := range pod.Spec.Containers {
		if strings.HasSuffix(c.Resources.Requests["cpu"], "m") {
			milliCores := strings.TrimSuffix(c.Resources.Requests["cpu"], "m")
			cores, err := strconv.Atoi(milliCores)
			if err != nil {
				return nil, err
			}
			spaceRequired += cores
		}
	}

	for _, node := range nodeList.Items {
		var allocatableCores int
		var err error
		if strings.HasSuffix(node.Status.Allocatable["cpu"], "m") {
			milliCores := strings.TrimSuffix(node.Status.Allocatable["cpu"], "m")
			allocatableCores, err = strconv.Atoi(milliCores)
			if err != nil {
				return nil, err
			}
		} else {
			cpu := node.Status.Allocatable["cpu"]
			cpuFloat, err := strconv.ParseFloat(cpu, 32)
			if err != nil {
				return nil, err
			}
			allocatableCores = int(cpuFloat * 1000)
		}

		freeSpace := (allocatableCores - resourceUsage[node.Metadata.Name].CPU)
		if freeSpace < spaceRequired {
			m := fmt.Sprintf("fit failure on node (%s): Insufficient CPU", node.Metadata.Name)
			fitFailures = append(fitFailures, m)
			continue
		}
		nodes = append(nodes, node)
	}

	if len(nodes) == 0 {
		// Emit a Kubernetes event that the Pod was scheduled successfully.
		timestamp := time.Now().UTC().Format(time.RFC3339)
		event := Event{
			Count:          1,
			Message:        fmt.Sprintf("pod (%s) failed to fit in any node\n%s", pod.Metadata.Name, strings.Join(fitFailures, "\n")),
			Metadata:       Metadata{GenerateName: pod.Metadata.Name + "-"},
			Reason:         "FailedScheduling",
			LastTimestamp:  timestamp,
			FirstTimestamp: timestamp,
			Type:           "Warning",
			Source:         EventSource{Component: "my-scheduler"},
			InvolvedObject: ObjectReference{
				Kind:      "Pod",
				Name:      pod.Metadata.Name,
				Namespace: "default",
				Uid:       pod.Metadata.Uid,
			},
		}

		postEvent(event)
	}

	return nodes, nil
}

func bind(pod *Pod, node Node) error {
	binding := Binding{
		ApiVersion: "v1",
		Kind:       "Binding",
		Metadata:   Metadata{Name: pod.Metadata.Name},
		Target: Target{
			ApiVersion: "v1",
			Kind:       "Node",
			Name:       node.Metadata.Name,
		},
	}

	var b []byte
	body := bytes.NewBuffer(b)
	err := json.NewEncoder(body).Encode(binding)
	if err != nil {
		return err
	}

	request := &http.Request{
		Body:          ioutil.NopCloser(body),
		ContentLength: int64(body.Len()),
		Header:        make(http.Header),
		Method:        http.MethodPost,
		URL: &url.URL{
			Host:   apiHost,
			Path:   fmt.Sprintf(bindingsEndpoint, pod.Metadata.Name),
			Scheme: "https",
		},
	}
	request.Header.Set("Content-Type", "application/json")

	bearer := "Bearer " + config_G.Token
	request.Header.Add("Authorization", bearer)
	resp, err := http.DefaultClient.Do(request)
	if err != nil {
		return err
	}
	if resp.StatusCode != 201 {
		return errors.New("Binding: Unexpected HTTP status code" + resp.Status)
	}

	// Emit a Kubernetes event that the Pod was scheduled successfully.
	message := fmt.Sprintf("Successfully assigned %s to %s", pod.Metadata.Name, node.Metadata.Name)
	timestamp := time.Now().UTC().Format(time.RFC3339)
	event := Event{
		Count:          1,
		Message:        message,
		Metadata:       Metadata{GenerateName: pod.Metadata.Name + "-"},
		Reason:         "Scheduled",
		LastTimestamp:  timestamp,
		FirstTimestamp: timestamp,
		Type:           "Normal",
		Source:         EventSource{Component: "my-scheduler"},
		InvolvedObject: ObjectReference{
			Kind:      "Pod",
			Name:      pod.Metadata.Name,
			Namespace: "default",
			Uid:       pod.Metadata.Uid,
		},
	}
	log.Println(message)
	return postEvent(event)
}
