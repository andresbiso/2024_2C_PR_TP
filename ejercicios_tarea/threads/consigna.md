# Ejercicio

Crear una aplicación de línea de comandos en C que lea del STDIN un keypress e imprima el key code en un thread. Implementar también, la versión multiproceso utilizando fork().

## Ejemplo

Main Thread -> (1) key = read() -> pthread_create(&key) -> pthread_join(threadN) -> (1)

Thread N -> printf(key) -> (end)
