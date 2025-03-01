#!/bin/bash

docker build --platform=linux/amd64 -t boolean-chains -f batch/docker/Dockerfile .
docker tag boolean-chains:latest $AWS_ACCOUNT.dkr.ecr.us-east-1.amazonaws.com/boolean-chains:latest
docker push $AWS_ACCOUNT.dkr.ecr.us-east-1.amazonaws.com/boolean-chains:latest
