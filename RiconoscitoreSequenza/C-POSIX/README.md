# Riconoscitore di sequenza — versione C/POSIX

Tre task "generatori" (thread periodici indipendenti, periodi 10/20/40 ms) producono ciascuno un bit in una struttura condivisa. Un task "riconoscitore" legge i tre bit a ogni ciclo, li combina in un valore decimale e verifica se le ultime 4 letture formano la sequenza target `0 3 6 5`. Un task "buddy" stampa periodicamente lo stato corrente.

Sincronizzazione tramite mutex POSIX (con `PTHREAD_PRIO_INHERIT`), scheduling `SCHED_FIFO`.

Questa è la versione preparatoria su Linux/pthread dello stesso problema poi risolto su FreeRTOS per la consegna d'esame (vedi [`../FreeRTOS/`](../FreeRTOS/)).

## Come eseguirlo (Docker)

```bash
docker build -t riconoscitore .
docker run -it --rm --cap-add=sys_nice riconoscitore
```

Il programma stampa a schermo i bit generati e lo stato del riconoscimento ad ogni ciclo. Premi `q` e poi invio per terminare.

## Nota

Con i periodi/fasi di default dei 3 generatori (deterministici, non casuali), la sequenza target potrebbe non presentarsi per diversi secondi — è un comportamento del programma originale, non un problema di esecuzione.
