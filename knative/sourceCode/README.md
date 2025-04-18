Edit for test
## note the cluster ip is hardcoded
#How to run the exp
1. cd /traffic-gen/config    modify the config-map.yaml, kubectl apply -f config-map.yaml
2. kubectl apply -f sample-scheduler.yaml

#Source code for k8s scheduler, scaler and routing to manage knative services.

#1. generate a token for the default service account
kubectl apply -f gen-secret.yaml

#2. bind the default service account to cluster-admin
kubectl apply -f bind-role.yaml

#3. enbale multiple scheduler for knative
kubectl edit configmap -n knative-serving config-features -oyaml

#change it to kubernetes.podspec-schedulername: enabled

#4. get the account token
kubectl describe secret default-token | grep -E '^token'

#5. access the  API server
curl api-server-ip:port/api/v1/nodes --header "Authorization: Bearer token" --insecure

#6. access the API in the same way in pod using https request. 
# checklist: api-server ip, port. serviceaccount token and clusterrole. disable https cert verify. enable multiple scheduler. deploy knatvie service with my-scheduler.

#kubectl api-resources check api endpoints and version
#kubectl config view  check api config
# knative service https://10.154.0.20:6443/apis/serving.knative.dev/v1/services 

#anthor way to get a certain api resource other than curl, kubectl get --raw api/path
kubectl get --raw "/apis/serving.knative.dev/v1/services"

#list the object in json format

kubectl get --raw /apis/serving.knative.dev/v1/services | python3 -m json.tool

#build container across platform
#build docker image

docker buildx build --push --platform linux/amd64,linux/arm64,linux/arm/v7  -t cocc/scheduler-demo:v3.0 .

#knative/serving/pkg/client/clientset/versioned/client.go also provide interface to access API, not used

--------------access service from outside

## 1. configure kourier to NodePort type

kubectl patch svc -n kourier-system kourier -p '{"spec": {"type": "NodePort"}}'

## 2. configure DNS
   ### https://knative.dev/docs/install/yaml-install/serving/install-serving-with-yaml/#configure-dns

   kubectl patch configmap/config-domain \
      --namespace knative-serving \
      --type merge \
      --patch '{"data":{"example.com":""}}'

# 3. get node ip and port of kourier

node ip is the ip of the physical node that host the kourier svc

kubectl get pods --all-namespaces

kubectl get svc -n kourier-system

## 4. get wanted service_url

kubectl get routes

## 5. feed traffic to service is, nodeip is ip of kourier pod, port is port of kourier

curl nodeip:nodeport -H 'Host: service_url', this is using host header


# curl api resources is relative path which is ip/relativeurl'
