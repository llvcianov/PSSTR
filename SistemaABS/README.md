# ABS System

Simulation of an ABS (Anti-lock Braking System) across two independent wheels, developed for the Real-Time Systems Design and Development course (PSSTR).

## Architecture

For each of the two wheels there is a `plant` process (simulates the wheel's sensor and actuator) and a `controller` process (reads the sensor data, decides the braking/deceleration action, and communicates with the other wheel to coordinate the ABS intervention). The two controllers exchange information via POSIX message queues to prevent both wheels from braking hard at the same time. A `reference` process sets the target value (target speed/position) for both wheels, and a `DIAG` process queries both controllers for diagnostic statistics (thread execution times, buffer state).

All communication happens via named POSIX message queues, with `SCHED_FIFO` real-time scheduling.

## How to run it (Docker)

```bash
docker build -t sistema-abs .
docker run -d --name abs --cap-add=sys_nice sistema-abs
```

This builds the project and starts the 4 main processes (`plant`, `plant_2`, `controller`, `controller_2`) in the background inside the container. `--cap-add=sys_nice` is required to allow real-time scheduling (`SCHED_FIFO`).

At this point you can interact with the simulation:

```bash
docker exec abs ./reference 50     # set the target reference for both wheels
docker exec abs ./DIAG             # request diagnostics from both wheels
docker exec -it abs bash           # enter the container to explore freely
```

### How to observe the ABS kicking in

The controller computes `error = reference - sensor_value` and decides the action accordingly (slight acceleration/deceleration, or no action). **Only an exact `reference = 0`** triggers the emergency braking scenario:

```bash
docker exec abs ./reference 100   # value close to the initial sensor reading: no action, "cruising"
docker exec abs ./reference 0     # emergency braking
docker exec abs ./DIAG            # repeat this from time to time to observe how it evolves
```

With `reference = 0` the controller commands strong, continuous braking. Prolonged hard braking (beyond ~10 cycles) has a chance of making the wheel skid (it locks up, the sensor stops updating — the exact lock-up that ABS is meant to prevent in real life). When the controller detects the skid, the actual ABS logic kicks in: it eases the braking from strong to light and also notifies the other wheel's controller via the coordination queue, so it eases off too. In the `./DIAG` output, you'll see the "Comando di controllo" (control command) field switch from `4` (hard braking) to `2` (light braking, ABS intervention) once/if the skid is detected — this is probabilistic and might not happen right away.

To stop everything:
```bash
docker rm -f abs
```

## Differences from the exam version

The original version (`run.sh`) opened 4 separate terminal windows (`gnome-terminal`) for the 4 processes — not practical in a headless container. Here the processes are started in the background inside the same container (necessary because they communicate via named POSIX message queues, which are only visible within the same IPC namespace) via [`start.sh`](start.sh), which is the only addition compared to the original code. No changes were made to the application logic.
