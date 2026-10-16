FROM debian:trixie-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY riconoscitore_soluzione.c .
RUN gcc -o riconoscitore riconoscitore_soluzione.c -lpthread

CMD ["./riconoscitore"]
