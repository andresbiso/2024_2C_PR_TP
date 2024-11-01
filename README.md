<p align="center">
    Trabajo Práctico Integrador - PR - UP
    <br>
    2C - 2024
    <br>
</p>

# :pencil: Table of Contents

- [Acerca De](#about)
- [Aplicaciones Utilizadas](#applications)
- [Docker](#docker)
- [Visual Studio Code](#vscode)
- [Levantar el proyecto ](#run_project)
- [Autor](#author)
- [Reconocimientos](#acknowledgement)

# :information_source: Acerca De <a name = "about"></a>

- Repositorio que contiene el trabajo práctico de la materia Programación en Redes de la Universidad de Palermo.

# :hammer: Aplicaciones Utilizadas <a name = "applications"></a>

## Windows

Recomiendo utilizar [chocolatey](https://chocolatey.org/install) para instalar estos paquetes:

- [vscode](https://community.chocolatey.org/packages/vscode)

```
choco install vscode
```

- [googlechrome](https://community.chocolatey.org/packages/googlechrome)

```
choco install googlechrome
```

## macOS

Recomiendo utilizar [homebrew](https://brew.sh/) para instalar estos paquetes:

- [visual-studio-code](https://formulae.brew.sh/cask/visual-studio-code#default)

```
brew install --cask visual-studio-code
```

- [google-chrome](https://formulae.brew.sh/cask/google-chrome#default)

```
brew install --cask google-chrome
```

# :hammer: Docker <a name = "docker"></a>

## macOS

- https://docs.docker.com/desktop/install/mac-install/
- [docker](https://formulae.brew.sh/cask/docker#default)

```
brew install --cask docker
```

## Windows

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

Salir de docker container:

```bash
exit
```

### Start/Stop Docker Container

```
docker stop ubuntu-redes
docker start -i ubuntu-redes
```

## Portainer

- Community Edition: https://docs.portainer.io/v/2.20/start/install-ce/server/docker/wsl

```
username: admin
password: Password1234
```

# :hammer: Visual Studio Code <a name = "vscode"></a>

## Visual Studio Code Extensions

- [vscode.cpptools-extension-pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
- [ms-azuretools.vscode-docker](https://marketplace.visualstudio.com/items?itemName=ms-azuretools.vscode-docker)
- [s-vscode-remote.remote-containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
- [ms-vscode.makefile-tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.makefile-tools)

## Utilizar Visual Studio Code para C con Docker

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

### Recursos

- https://code.visualstudio.com/docs/languages/cpp
- https://code.visualstudio.com/docs/cpp/launch-json-reference
- https://code.visualstudio.com/docs/editor/debugging

# :hammer: Levantar el proyecto <a name = "run_project"></a>

## ¿Cómo levantar el proyecto?

1. Levantamos en vscode el proyecto.
2. Levantamos una terminal que corran nuestra docker image en un docker container.
3. Dentro del docker container compilamos (make build) y corremos nuestra aplicación (./dist/<appname>).

## ¿Cómo exponer un puerto de docker al host?

1. Abrir a compose.yml.
2. Ir a ports.
3. Configurar HOST_PORT:CONTAINER_PORT.

- Más información: https://docs.docker.com/compose/how-tos/networking/

## Comandos útiles

- [netstat](https://linux.die.net/man/8/netstat)
  ```
  netstat -n --inet
  netstat -l -n --inet
  netstat -l -n --inet -p
  netstat -tcp
  ```
- [ps](https://linux.die.net/man/1/ps)
  ```
  ps -fea
  ps -fea | grep netcat
  ```
- [nc/netcat](https://linux.die.net/man/1/nc)
  - Server (listen)
  ```
  netcat -l localhost 8001 [port]
  nc -l localhost 80
  ```
  - Client (Connect)
  ```
  netcat localhost 8001 [port]
  nc localhost 80
  ```
- [strace](https://linux.die.net/man/1/nc)
  ```
  strace -p 14442 [process_id obtenido con ps -fea]
  ```
- [ifconfig](https://linux.die.net/man/8/ifconfig)
  ```
  ifconfig
  ```
- [ss](https://linux.die.net/man/8/ss)
- [watch](https://linux.die.net/man/1/watch)
  ```
  watch -n0 ss -t4ane
  ```
- [ls/ll](https://linux.die.net/man/1/ls)
  ```
  ll is an alias of ls -l
  More info:
  https://unix.stackexchange.com/questions/137703/difference-between-ls-l-and-ll
  ```
- [gcc](https://linux.die.net/man/1/gcc)
  ```
  gcc sockets.c -o socket
  ./socket
  ```
- [make](https://linux.die.net/man/1/make)
  - Si el directorio contiene un Makefile
  ```
  make
  ```
- [nslookup](https://linux.die.net/man/1/nslookup)
  ```
  nslookup yahoo.com
  ```
- [dig](https://linux.die.net/man/1/dig)
- [lsof](https://linux.die.net/man/8/lsof)
- [valgrind](https://linux.die.net/man/1/valgrind)
  - https://valgrind.org/docs/manual/quick-start.html#quick-start.mcrun
  ```
  valgrind --leak-check=full --show-leak-kinds=all ./myprog
  ```

# :speech_balloon: Autor <a name = "author"></a>

- [@andresbiso](https://github.com/andresbiso)

# :tada: Reconocimientos <a name = "acknowledgement"></a>

- https://github.com/dmarafetti/up-redes
- https://github.com/github/gitignore
- https://gist.github.com/rxaviers/7360908 -> github emojis
