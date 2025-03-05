#!/bin/bash

docker build --platform=linux/amd64 -t hungry-search -f batch/hungry-search-docker/Dockerfile .
docker tag hungry-search:latest $AWS_ACCOUNT.dkr.ecr.us-east-1.amazonaws.com/hungry-search:latest
docker push $AWS_ACCOUNT.dkr.ecr.us-east-1.amazonaws.com/hungry-search:latest
