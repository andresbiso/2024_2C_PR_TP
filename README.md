<p align="center">
    Trabajo Práctico Integrador - PR - UP
    <br>
    2C - 2024
    <br>
</p>

# :pencil: Table of Contents

- [Acerca De](#about)
- [Herramientas Utilizadas](#built_using)
- [Autor](#author)
- [Reconocimientos](#acknowledgement)

# :information_source: Acerca De <a name = "about"></a>

- Repositorio que contiene el trabajo práctico de la materia Programación en Redes de la Universidad de Palermo.

# :hammer: Herramientas Utilizadas <a name = "built_using"></a>

## Herramientas

### Windows

Recomiendo utilizar [chocolatey](https://chocolatey.org/install) para instalar estos paquetes:

- [vscode](https://community.chocolatey.org/packages/vscode)

```
choco install vscode
```

- [googlechrome](https://community.chocolatey.org/packages/googlechrome)

```
choco install googlechrome
```

### macOS

Recomiendo utilizar [homebrew](https://brew.sh/) para instalar estos paquetes:

- [visual-studio-code](https://formulae.brew.sh/cask/visual-studio-code#default)

```
brew install --cask visual-studio-code
```

- [google-chrome](https://formulae.brew.sh/cask/google-chrome#default)

```
brew install --cask google-chrome
```

## Docker

### macOS

- https://docs.docker.com/desktop/install/mac-install/
- [docker](https://formulae.brew.sh/cask/docker#default)

```
brew install --cask docker
```

### Windows

- https://docs.docker.com/desktop/install/windows-install/

## ¿Cómo usar dockerfile?

Se provee un docker container con todas las tools necesarias para compilación y debugging de aplicaciónes. Para instalar el entorno ejecutar el docker compose detached en el root del proyecto.

```bash
 docker compose up --build -d
```

Una vez finalizado se puede conectarse por TTY al container con el siguiente comando:

```bash
 docker exec -it ubuntu-redes bash
```

## (Legacy) ¿Cómo usar dockerfile?

- https://stackoverflow.com/questions/34782678/difference-between-running-and-starting-a-docker-container

### Build Docker Image

```
cd ./dockerbuild
docker build -t "ubuntu-redes:v1" .
docker images
```

### Remove Docker Image

```
docker image rm ubuntu-redes:v1
```

### Run Docker Image

- Hacemos también un binding de carpeta del host al container.
- El commando exit nos permite salir de un container.

```
docker run --name ubuntu-redes -v /Users/andres/Documents/repos/2024_2C_PR_TP/:/home/workspace -i -t ubuntu-redes:v1 /bin/bash
docker ps -a
```

### Start/Stop Docker Container

```
docker stop ubuntu-redes
docker start -i ubuntu-redes
```

### Remove Docker Containers

- https://linuxhandbook.com/remove-docker-containers/

```
docker ps -a
docker stop ubuntu-redes
docker rm ubuntu-redes
docker ps -a
```

## Portainer

- Community Edition: https://docs.portainer.io/v/2.20/start/install-ce/server/docker/wsl

```
username: admin
password: Password1234
```

## Visual Studio Code Extensions

- [vscode.cpptools-extension-pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
- [ms-azuretools.vscode-docker](https://marketplace.visualstudio.com/items?itemName=ms-azuretools.vscode-docker)
- [s-vscode-remote.remote-containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
- [ms-vscode.makefile-tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.makefile-tools)

### Utilizar Visual Studio Code para C con Docker

1. Levantar el container que vamos a utilizar haciendo el mapeo de carpeta local a carpeta dentro de la /home del container.
2. En Visual Studio abrir Remote Window y seleccionar "Attach to Running Container". Indicar el que levantamos en el paso anterior.
   2.1. Ir en esta ventana a la parte de extensiones del Visual Studio y asegurarse de que estén todas habilitadas.
3. Configurar la configuration de debugging del launch.json para que en el program vaya a la ruta del archivo.
   3.1. Asegurarse de configurar el "preLaunchTask" para que levante la task de build primero.
4. Ir al archivo que queremos hacer debug, ir a la sección run and debug del vscode y seleccionar la configuration.

Recomiendo:

- Cargar los ejemplos o proyectos de código de a uno dentro del container.
- Que cada carpeta con un proyecto tenga su propio .vscode con su launch.json y su tasks.json.
- Que cada proyecto tenga su propio Makefile.

#### Recursos

- https://code.visualstudio.com/docs/languages/cpp
- https://code.visualstudio.com/docs/cpp/launch-json-reference
- https://code.visualstudio.com/docs/editor/debugging

# :speech_balloon: Autor <a name = "author"></a>

- [@andresbiso](https://github.com/andresbiso)

# :tada: Reconocimientos <a name = "acknowledgement"></a>

- https://github.com/dmarafetti/up-redes
- https://github.com/github/gitignore
- https://gist.github.com/rxaviers/7360908 -> github emojis
