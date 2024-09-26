<h1 align="center">
  <br>
  <a href="https://www.palermo.edu"><img src="https://www.palermo.edu/images/header/logo@2x.png" alt="up logo" width="130"></a>
  <br>
  Programación en Redes 
</h1>
<br>

***

[![forthebadge](https://forthebadge.com/images/badges/docker-container.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/made-with-c.svg)](https://forthebadge.com)
[![forthebadge](http://forthebadge.com/images/badges/built-with-love.svg)](http://forthebadge.com)

<br>

Repositorio oficial de la materia Programación en Redes. Departamento de Ingeniería - UP  
**Profesor Ing Diego Marafetti**

# Contenidos

- [Requerimientos](#Requerimientos)
- [Entorno de desarrollo](#installation)
  - [Docker](#docker)
- [Configuracion recomendada](#recommended-configurations)
- [Contributing](#contributing)
- [Contacto](#contacto)


## Requerimientos 🚀

- Descargar e instalar [Visual Studio Code](https://code.visualstudio.com/Download)
- Descargar e instalar [Docker](https://www.docker.com/). Esto puede variar según el sistema operativo.
- Instalar las siguiente extensiones de VSCode
  - C/C++
  - C/C++ Extension Pack
  - CMake
  - CMake Tools
  - Docker
  - Dev Containers

## Entornos de Desarrollo

> [!NOTE]
> Se recomienda la configuración utilizando Docker.

### Docker

Se provee un docker container con todas las tools necesarias para compilación y debugging de aplicaciónes. Para instalar el entorno ejecutar el docker compose detached en el root del proyecto. 

```bash
 docker compose up --build -d
```

Una vez finalizado se puede conectarse por TTY al container con el siguiente comando:

```bash
 docker exec -it ubuntu-redes bash
```



## Contributing

[(Back to top)](#contenidos)

Contributions are always welcome!

See `contributing.md` for ways to get started.

Please adhere to this project's `code of conduct`.




## Contacto

[(Back to top)](#contenidos)

Diego Marafetti - [@email](mailto:dmaraf@palermo.edu)
