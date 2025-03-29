#!/bin/bash

docker build --platform=linux/amd64 -t boinc-central-boolean-chains -f boinc-central/docker/Dockerfile-for-build .

container_name=boinc-central-container
docker rm -f "$container_name" 2>/dev/null || true
docker run --name "$container_name" --platform linux/amd64 boinc-central-boolean-chains

container_id="$(docker ps -a | grep $container_name | awk '{print $1}')"
mkdir -p boinc-build
docker cp $container_id:/target/full-search ./boinc-build
docker cp $container_id:/plan.txt ./boinc-build
