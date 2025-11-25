#!/bin/bash
mkdir -p shared_data

sudo docker rm -f container1 container2 2>/dev/null

sudo docker run -d --name container1 -v $(pwd)/shared_data:/flock flock-app
sudo docker run -d --name container2 -v $(pwd)/shared_data:/flock flock-app
