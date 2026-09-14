# TP1 Sistemas Operativos — ChompChamps

Implementación de los binarios `master`, `view` y `player` del juego ChompChamps, comunicados
mediante memoria compartida POSIX, semáforos anónimos y pipes anónimos.

## Integrantes

| Nombre y Apellido | Legajo |
| --- | --- |
| Jesús Gabriel Bastidas Díaz  | 64475 |
| Agustín Uriel Mayor Saavedra | 65630 |
| Alan Gabriel Navarro |  |
| Enzo Canelo |  |

---

## Decisiones de diseño

### Estructura del proyecto

```
tp1-so/
├── Makefile                 genera los 3 binarios en bin/
├── include/                 headers de todos los módulos
│   ├── game_state.h         GameState y Player: lo que vive en /game_state
│   ├── game_sync.h          GameSync: los semáforos de /game_sync
│   ├── sharedMem.h          crear / conectar / destruir memoria compartida
│   ├── board.h              geometría del tablero y las 8 direcciones
│   ├── signals.h            handlers de SIGTERM y SIGUSR1
│   ├── args.h               parseo de los parámetros del máster
│   ├── master.h             constantes generales del máster
│   ├── gameInit.h           armado del tablero y spawn de los hijos
│   ├── gameLoop.h           loop principal y cierre de la partida
│   └── playerAI.h           snapshot del estado y estrategia del jugador
│
└── src/
    ├── board.c              ─┐
    ├── sharedMem.c           │  módulos compartidos por más de un binario
    ├── semaphores.c          │  (implementan game_sync.h)
    ├── signals.c            ─┘
    │
    ├── master/              → bin/master
    │   ├── master.c         crea las shm, inicializa todo, lanza hijos y limpia al final
    │   ├── args.c           getopt de -w -h -d -t -s -v -p
    │   ├── gameInit.c       tablero aleatorio, ubicación de jugadores, pipe+fork+dup2+exec
    │   └── gameLoop.c       pselect sobre los pipes, round-robin, señales, pausa, gameOver
    │
    ├── view/                → bin/view
    │   └── view.c           se conecta a ambas shm e imprime el estado
    │
    └── player/              → bin/player
        ├── player.c         espera su turno, lee el estado y escribe el movimiento por el fd 1
        └── playerAI.c       copia local del estado y elección del movimiento
```

Los módulos de `src/` (sin subcarpeta) son los que se enlazan en más de un binario: `board.c` lo
comparten máster y jugador para que ambos interpreten las 8 direcciones exactamente igual y no se
dupliquen las validaciones; `sharedMem.c` encapsula `shm_open`/`mmap` con `createSharedMem` para el
máster y `connectSharedMem` para vista y jugadores; `semaphores.c` implementa `game_sync.h`, es
decir la inicialización de los semáforos y las primitivas de lectores/escritores.

Cada binario se arma con su propia lista de objetos en el `Makefile`: la vista solo necesita
`view.o`, mientras que el máster suma los cuatro módulos compartidos y el jugador tres de ellos.

### Memoria compartida

`GameState` termina en un flexible array member, por eso se mapea `sizeof(GameState) +
width*height`; esa es la razón por la que vista y jugadores reciben las dimensiones por parámetro.
El máster es el único que abre `/game_state` en modo escritura: la vista lo mapea con `PROT_READ`,
así una escritura accidental aborta en vez de corromper el estado.

### Sincronización

- Máster ↔ vista: señalización bidireccional con `viewSignal`/`viewUpdated`. El delay `-d` se
  aplica recién después de que la vista confirma que terminó de imprimir.
- Máster ↔ jugadores: lectores/escritores con prioridad al escritor. `masterMutex`:
 todo lector pasa por él antes de registrarse, de modo que ninguno se cuela si el
  máster ya está esperando para escribir. `cantReaders` (bajo `readersMutex`) hace que solo el
  primer lector tome y el último libere `gameStateMutex`.
- `playerTurn[i]` comienza en 1 y habilita un movimiento por jugador; el máster lo repone recién
  después de procesar la solicitud.

### Máster

- Los movimientos llegan por el fd 1 de cada jugador (`dup2` sobre el pipe). El extremo de lectura
  se marca `FD_CLOEXEC` para que los hermanos no lo hereden: sino, el pipe nunca daría EOF y un
  jugador muerto jamás se detectaría como bloqueado.
- El loop usa `pselect` sobre los pipes activos con el timeout restante como deadline, sin espera
  activa.
- Round-robin arrancando en `lastPlayerPlayed + 1`, para no favorecer a los primeros índices.
- Señales con `sigaction` y handlers que solo escriben una `volatile sig_atomic_t`. Quedan
  bloqueadas durante el loop y se desbloquean atómicamente dentro del `pselect`, lo que elimina la
  ventana entre "chequeo la flag" y "sleep"; además cada iteración abre una ventana explícita
  por si el `pselect` nunca llega a dormirse.
- En pausa el máster duerme en un `pselect` sin descriptores y al reanudar corre hacia adelante la
  marca del último movimiento válido, para que el tiempo pausado no consuma el timeout.
- Al finalizar despierta a los jugadores, cierra los extremos de lectura (si no, uno escribiendo en
  un pipe lleno colgaría el `waitpid`) y recién ahí espera a cada hijo.
- Los jugadores se ubican equiespaciados sobre el perímetro de un rectángulo interior al tablero:
  determinístico y con margen de movimiento parejo.

### Vista

Arma el cuadro completo en un buffer y lo escribe con un único `write`, evitando el parpadeo. No
limpia la pantalla: posiciona el cursor en `\033[H` y borra con `\033[K`/`\033[0J`. Se centra según
el tamaño real de la terminal. Cada jugador tiene un color: tono vivo para su posición actual y el
mismo tono apagado para sus celdas capturadas; las libres muestran su numero. La tabla inferior
expone todos los campos del estado salvo el `pid`.

### Jugador

Copia el estado a un `GameSnapshot` dentro de la sección crítica y decide afuera, para retener el
lock lo menos posible. Espera su turno con `sem_timedwait` en tramos de un segundo, verificando
entre tramos que `getppid()` siga siendo el máster, así no queda colgado si el máster muere. Ignora
`SIGPIPE` para cerrar ordenadamente. Estrategia: la celda adyacente libre de mayor recompensa.

---

## Compilación y ejecución

```bash
docker pull agodio/itba-so-multiarch:3.1
docker run -it --rm -v "$PWD":/root -w /root agodio/itba-so-multiarch:3.1 bash
```

```bash
make          # genera master, view y player en bin/
make clean
./bin/master -w 15 -h 15 -d 150 -t 10 -s 42 -v ./bin/view -p ./bin/player ./bin/player ./bin/player
```

Parámetros, en cualquier orden: `-w` ancho y `-h` alto (default y mínimo 10), `-d` delay en ms
(200), `-t` timeout en segundos (10), `-s` semilla (`time(NULL)`), `-v` ruta de la vista (sin vista
por default), `-p` rutas de los jugadores (1 a 9, obligatorio). `-i` se acepta y se ignora: los
chequeos de consistencia son propios del máster provisto.

`kill -USR1 <pid_master>` pausa y reanuda; `kill -TERM <pid_master>` finaliza ordenadamente.

---

## Rutas para el torneo

- Vista: `bin/view`
- Jugador: `bin/player`

---

//////ver 

## Bonus

** Usuario Juega **, .

```
COmpletar 

```

---

## Limitaciones

- La estrategia del jugador: 

- La vista requiere una terminal con 256 colores y caracteres Unicode de dibujo de cajas. Si la
  terminal es más chica que el tablero, el contenido se corta.
- No se implementan los chequeos de consistencia del máster provisto (el enunciado aclara que no son
  obligatorios).

---

## Problemas encontrados

- **Jugadores colgados al morir el máster:** quedaban bloqueados para siempre en `sem_wait`. Se
  resolvió con `sem_timedwait` en tramos verificando `getppid()`.
- **EOF que nunca llegaba:** los jugadores heredaban los extremos de lectura de los pipes de sus
  hermanos. Se resolvió con `FD_CLOEXEC`.
- **Señales perdidas:** existía una carrera entre chequear la bandera y dormirse. Se resolvió
  bloqueándolas durante el loop y desbloqueándolas dentro del `pselect`.
- **Deadlock al cerrar:** el máster colgaba en `waitpid` con un jugador escribiendo en un pipe
  lleno. Se resolvió cerrando los extremos de lectura antes de esperar a los hijos.
- **Cuadro duplicado al final:** el último cuadro lo manda únicamente `gameOver`.

---

## Citas de fragmentos de código / uso de IA

//////// COMPLETAR

- Fragmentos de terceros: ????
- Uso de IA: .
- Documentación consultada: `shm_overview(7)`, `sem_overview(7)`, `pselect(2)`, `select_tut(2)`,
  `pipe(7)`, `sigaction(2)`.            