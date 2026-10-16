#!/bin/bash
mkdir -p /var/log/abs
echo "Avvio plant, plant_2, controller, controller_2 in background..."
./plant      > /var/log/abs/plant.log      2>&1 &
./plant_2    > /var/log/abs/plant_2.log    2>&1 &
sleep 1
./controller   > /var/log/abs/controller.log   2>&1 &
./controller_2 > /var/log/abs/controller_2.log 2>&1 &
sleep 1
echo ""
echo "Tutti i processi sono partiti. Log in /var/log/abs/*.log"
echo "Esempi:"
echo "  ./reference 50     # imposta il riferimento per entrambe le ruote"
echo "  ./DIAG             # richiede una diagnostica a entrambe le ruote"
echo "  tail -f /var/log/abs/controller.log"
echo ""
echo "Entra nel container con: docker exec -it <container> bash"
exec tail -f /dev/null
