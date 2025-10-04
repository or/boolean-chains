#!/bin/bash

#ARCH=amd64
ARCH=arm64

image_name="boinc-central-boolean-chains-$ARCH"
docker build --platform=linux/$ARCH -t $image_name -f boinc-central/docker/Dockerfile-for-build .

container_name=boinc-central-container-$ARCH
docker rm -f "$container_name" 2>/dev/null || true
docker run --name "$container_name" --platform linux/$ARCH $image_name

container_id="$(docker ps -a | grep $container_name | awk '{print $1}')"
mkdir -p boinc-build
docker cp $container_id:/target/full-search ./boinc-build
docker cp $container_id:/plan.txt ./boinc-build
docker cp $container_id:/target/hungry-search ./boinc-build
