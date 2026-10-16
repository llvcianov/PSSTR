# Sistema ABS

Simulazione di un sistema ABS (Anti-lock Braking System) su due ruote indipendenti, sviluppata per il corso di Progetto e Sviluppo di Sistemi in Tempo Reale (PSSTR).

## Architettura

Per ciascuna delle due ruote gira un processo `plant` (simula sensore e attuatore della ruota) e un processo `controller` (acquisisce il dato dal sensore, decide l'azione di frenata/decelerazione, comunica con l'altra ruota per coordinare l'intervento ABS). I due controller si scambiano informazioni tramite code di messaggi POSIX per evitare che entrambe le ruote frenino bruscamente insieme. Un processo `reference` imposta il valore di riferimento (velocità/posizione target) per entrambe le ruote, e un processo `DIAG` interroga entrambi i controller per ottenere statistiche di diagnostica (tempi di esecuzione dei thread, stato dei buffer).

Tutta la comunicazione avviene tramite code di messaggi POSIX con nome, scheduling `SCHED_FIFO` real-time.

## Come eseguirlo (Docker)

```bash
docker build -t sistema-abs .
docker run -d --name abs --cap-add=sys_nice sistema-abs
```

Questo compila il progetto e avvia in background i 4 processi principali (`plant`, `plant_2`, `controller`, `controller_2`) dentro al container. `--cap-add=sys_nice` è necessario per permettere lo scheduling real-time (`SCHED_FIFO`).

A questo punto puoi interagire con la simulazione:

```bash
docker exec abs ./reference 50     # imposta il riferimento per entrambe le ruote
docker exec abs ./DIAG             # richiede una diagnostica a entrambe le ruote
docker exec -it abs bash           # entra nel container per esplorare liberamente
```

### Come osservare l'intervento dell'ABS

Il controller calcola `errore = reference - valore_sensore` e decide l'azione di conseguenza (accelera/decelera leggermente, o nessuna azione). **Solo `reference = 0` esatto** attiva lo scenario di frenata di emergenza:

```bash
docker exec abs ./reference 100   # valore vicino al sensore iniziale: nessuna azione, "crociera"
docker exec abs ./reference 0     # frenata di emergenza
docker exec abs ./DIAG            # ripeti ogni tanto per osservare l'evoluzione
```

Con `reference = 0` il controller comanda una frenata forte e continua. Una frenata prolungata (oltre ~10 cicli) ha una probabilità di far slittare la ruota (si blocca, il sensore smette di aggiornarsi — il bloccaggio che l'ABS deve evitare nella realtà). Quando il controller rileva lo slittamento, interviene l'ABS vero e proprio: riduce la frenata da forte a leggera e avvisa anche il controller dell'altra ruota tramite la coda di coordinamento, così anche lui allenta la frenata. Nell'output di `./DIAG` si osserva il campo "Comando di controllo" passare da `4` (frenata forte) a `2` (frenata leggera, intervento ABS) quando/se lo slittamento viene rilevato — è probabilistico, potrebbe non avvenire subito.

Per fermare tutto:
```bash
docker rm -f abs
```

## Differenze rispetto alla versione d'esame

La versione originale (`run.sh`) apriva 4 finestre di terminale separate (`gnome-terminal`) per i 4 processi — non praticabile in un container headless. Qui i processi vengono avviati in background dentro allo stesso container (necessario perché condividono le code di messaggi POSIX, visibili solo all'interno dello stesso namespace IPC) tramite [`start.sh`](start.sh), che è l'unica aggiunta rispetto al codice originale. Nessuna modifica alla logica applicativa.
