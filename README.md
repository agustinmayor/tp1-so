# TP1 Sistemas Operativos — ChompChamps

Implementación de los binarios `master`, `view` y `player` del juego ChompChamps, comunicados
mediante memoria compartida POSIX, semáforos anónimos y pipes anónimos.

## Integrantes

| Nombre y Apellido | Legajo |
| --- | --- |
| Jesús Gabriel Bastidas Díaz  | 64475 |
| Agustín Uriel Mayor Saavedra | 65630 |
| Alan Gabriel Navarro | 63330 |
| Enzo Canelo | 65732 |

---

## Decisiones de diseño
-Se utilizan dos regiones de memoria compartida. La primera almacena el estado global del juego y la información de los jugadores (game_state.h), mientras que la segunda contiene los mecanismos de sincronización mediante semáforos POSIX (game_sync.h).

-Se implementa un algoritmo de planificación Round Robin para distribuir de manera equitativa las oportunidades de juego entre los jugadores y evitar que un único jugador monopolice la ejecución mediante múltiples jugadas consecutivas.

-Para la comunicación y sincronización entre procesos se utilizan semáforos POSIX y pipes, permitiendo coordinar la ejecución de los distintos procesos y transmitir información entre ellos.

-Se utilizan dos señales para controlar el ciclo de vida del juego: SIGTERM, para solicitar la finalización del juego, y SIGUSR1, para realizar la pausa de la ejecución.

-La implementación se encuentra organizada en tres módulos principales: Master, Player y View, facilitando la separación de responsabilidades y la legibilidad del código. A su vez, cada módulo se divide en múltiples archivos .c para evitar la concentración excesiva de funcionalidades en un único archivo y mejorar la mantenibilidad.

-El proceso View utiliza un buffer de salida para construir previamente la representación del estado que debe mostrarse por pantalla. Una vez finalizada la construcción, el contenido se imprime de manera conjunta, evitando múltiples operaciones de salida durante la actualización de la interfaz.

-El proceso Master utiliza una estructura de datos intermedia propia para representar y validar la información antes de persistirla en la memoria compartida. De esta manera, se evita almacenar directamente datos que no hayan pasado previamente por las validaciones correspondientes.

-Durante el ciclo principal de ejecución del Master, las señales se mantienen bloqueadas para evitar que sean procesadas en momentos inconsistentes del flujo de ejecución. Estas señales se habilitan y gestionan exclusivamente dentro de la llamada a pselect, que permite esperar eventos y señales de manera controlada.

-Se optó por implementar una IA para el proceso PlayerIA basada en una estrategia en la cual elige la celda adyacente con mayor puntuación.

Del Bonus
-Para evitar la duplicación de código entre PlayerIA y PlayerBonus, se incorpora el módulo playerUtils, que concentra las funciones y funcionalidades comunes a ambos tipos de jugador.

-Se modifica el Master original de forma mínima, incorporando únicamente la validación necesaria para restringir el registro de jugadores al modo manual, manteniendo el resto de la lógica existente sin modificaciones significativas.

-Para la lectura de los inputs del jugador, se configura la terminal en modo raw, deshabilitando el line buffering y el echo de caracteres. Esto permite procesar las entradas de forma inmediata, sin esperar a la pulsación de Enter y sin mostrar automáticamente los caracteres ingresados en pantalla.

-Solo se permite el movimiento con WASD lo cual no permite el movimiento en diagonal.
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
-Se permite el uso del teclado (WASD) para mover manualmente un jugador. 

-En la ejecución se cambia uno de los ./bin/player por ./bin/bonusPlayer

## Limitaciones

- La estrategia del jugador no ve mas allá del siguiente paso
  
- La vista requiere una terminal con 256 colores y caracteres Unicode de dibujo de cajas. Si la
  terminal es más chica que el tablero, el contenido se corta.

- No se implementan los chequeos de consistencia del máster provisto 

Del Bonus

-Solo se permite el movimiento con WASD lo cual no permite el movimiento en diagonal.

-Master nuestro no se comporta igual que el normal: agrega una validacion para que sea maximo un bonusPlayer, lo demas anda igual


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

 Del Bonus
-Al implementar el bonus para escenarios con una cantidad elevada de jugadores, se observó una degradación en la capacidad de procesamiento del BonusPlayer,    provocando demoras en la selección y ejecución de movimientos.

 -Se detectaron situaciones en las que el BonusPlayer quedaba inmovilizado en determinadas posiciones del tablero, aun cuando el estado del juego no indicaba formalmente que la posición se encontrara bloqueada. Esto se debía a que el algoritmo del BonusPlayer no contemplaba movimientos diagonales, reduciendo el conjunto de movimientos posibles y generando falsos bloqueos.
 
 -Se identificó un problema en la gestión del timeout del BonusPlayer: ante una situación de inmovilización, el proceso no era finalizado correctamente. Debido a la ausencia de movimientos válidos contemplados por la estrategia, el proceso podía permanecer ejecutándose indefinidamente en lugar de ser terminado mediante el mecanismo de timeout establecido.
 

---

## Citas de fragmentos de código / uso de IA
- Se utilizaron modelos de IA como asistencia para tareas de debugging, e implementación de funciones 
- Toda implementación de IA fue revisada y adaptada manualmente.
- Documentación consultada: `shm_overview(7)`, `sem_overview(7)`, `pselect(2)`, `select_tut(2)`,
  `pipe(7)`, `sigaction(2)`.            
