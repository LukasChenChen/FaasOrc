#compile code
go buiid

#push to the remote dockerhub
docker buildx build --push --platform linux/amd64,linux/arm64,linux/arm/v7  -t cocc/reponame:vxx .

#main.go main function

#kubernetes.go   get, read resources from api
#scaler.go       scale the pods
#bestprice.go    pod assignment
#types.go        customised data structure       

> The dockerfile will copy current ./config files to . in the docker space
> then the container will read parameters from the configmap of knative
> "kubectl apply -f config-map.yaml" before run scheduler
> in the sample-scheduler.yaml you can see the config is retrived from the configmap
