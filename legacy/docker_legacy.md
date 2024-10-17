# (Legacy) ¿Cómo usar dockerfile?

- https://stackoverflow.com/questions/34782678/difference-between-running-and-starting-a-docker-container

## Build Docker Image

```
cd ./dockerbuild
docker build -t "ubuntu-redes:v1" .
docker images
```

## Remove Docker Image

```
docker image rm ubuntu-redes:v1
```

## Run Docker Image

- Hacemos también un binding de carpeta del host al container.
- El commando exit nos permite salir de un container.

```
docker run --name ubuntu-redes -v /Users/andres/Documents/repos/2024_2C_PR_TP/:/home/workspace -i -t ubuntu-redes:v1 /bin/bash
docker ps -a
```

## Remove Docker Containers

- https://linuxhandbook.com/remove-docker-containers/

```
docker ps -a
docker stop ubuntu-redes
docker rm ubuntu-redes
docker ps -a
```

## Start/Stop Docker Container

```
docker stop ubuntu-redes
docker start -i ubuntu-redes
```
