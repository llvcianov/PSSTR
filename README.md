# Sistemi Real Time

Progetti d'esame per il corso di **Progetto e Sviluppo di Sistemi in Tempo Reale (PSSTR)**, Laurea Triennale in Ingegneria Informatica, Università di Napoli Federico II.

Lavoro di gruppo: Luciano Vitale, Matteo Zazzaro, Gianluca Tuccillo, Carlo Ruzzon.

> **Nota**: progetti universitari risalenti al 2023, pensati a scopo didattico (programmazione concorrente/real-time in C su Linux con POSIX, e su FreeRTOS). Non rispecchiano pratiche di sviluppo moderne di produzione: oggi per sistemi real-time embedded si userebbero toolchain e framework più recenti. Il codice applicativo è rimasto fedele alla versione originale d'esame; sono stati aggiunti solo Docker e piccoli fix di infrastruttura per renderli eseguibili facilmente.

## Progetti

### [SistemaABS/](SistemaABS/)
Simulazione di un sistema ABS (Anti-lock Braking System) su due ruote indipendenti, in C/POSIX: processi `plant`/`controller` per ciascuna ruota, comunicazione tramite code di messaggi POSIX, scheduling real-time (`SCHED_FIFO`), diagnostica via polling server. ~2100 righe di codice.

**Si esegue in Docker** (vedi il [README del progetto](SistemaABS/README.md) per i dettagli):
```bash
cd SistemaABS
docker build -t sistema-abs .
docker run -d --name abs --cap-add=sys_nice sistema-abs
docker exec abs ./reference 50
docker exec abs ./DIAG
```

### [RiconoscitoreSequenza/](RiconoscitoreSequenza/)
Riconoscitore di una sequenza di bit generata da 3 task periodici indipendenti, in due versioni:

- **[C-POSIX/](RiconoscitoreSequenza/C-POSIX/)** — versione su Linux con pthread, **eseguibile in Docker**:
  ```bash
  cd RiconoscitoreSequenza/C-POSIX
  docker build -t riconoscitore .
  docker run -it --rm --cap-add=sys_nice riconoscitore
  # 'q' + invio per uscire
  ```
- **[FreeRTOS/](RiconoscitoreSequenza/FreeRTOS/)** — versione finale consegnata per l'esame (HW2), basata su task FreeRTOS. **Non containerizzata**: il progetto è pensato per il simulatore FreeRTOS-Windows e richiede il kernel FreeRTOS completo per essere compilato; qui è incluso solo come riferimento/lettura del codice.

### [ContatoreOttale/](ContatoreOttale/)
Esercizio FreeRTOS di un contatore modulo 8 (ottale) gestito da più task periodici con mutua esclusione tramite mutex, più un task "Polling Server" che ne stampa il contenuto a richiesta. **Non containerizzato**, stessa limitazione del riconoscitore FreeRTOS (simulatore Windows, kernel non incluso).

## Struttura

```
Progetti/
├── SistemaABS/                    (Docker)
├── RiconoscitoreSequenza/
│   ├── C-POSIX/                   (Docker)
│   └── FreeRTOS/                  (solo lettura)
└── ContatoreOttale/                (solo lettura)
```
