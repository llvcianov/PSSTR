# Sequence Recognizer — C/POSIX version

Three "generator" tasks (independent periodic threads, periods of 10/20/40 ms) each produce a bit in a shared structure. A "recognizer" task reads the three bits on every cycle, combines them into a decimal value, and checks whether the last 4 readings form the target sequence `0 3 6 5`. A "buddy" task periodically prints the current state.

Synchronization via POSIX mutexes (with `PTHREAD_PRIO_INHERIT`), `SCHED_FIFO` scheduling.

This is the preparatory Linux/pthread version of the same problem later solved on FreeRTOS for the exam submission (see [`../FreeRTOS/`](../FreeRTOS/)).

## How to run it (Docker)

```bash
docker build -t riconoscitore .
docker run -it --rm --cap-add=sys_nice riconoscitore
```

The program prints the generated bits and the recognition state on screen every cycle. Press `q` and then enter to quit.

## Note

With the 3 generators' default periods/phases (deterministic, not random), the target sequence might not show up for several seconds — this is a behavior of the original program, not an execution issue.
