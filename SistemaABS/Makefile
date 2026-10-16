all: plant plant_2 controller controller_2 reference DIAG

plant: plant.o rt-lib.o
	gcc -o plant plant.o rt-lib.o -lrt -pthread

plant_2: plant_2.o rt-lib.o
	gcc -o plant_2 plant_2.o rt-lib.o -lrt -pthread

controller: controller.o rt-lib.o
	gcc -o controller controller.o rt-lib.o -lrt -pthread

controller_2: controller_2.o rt-lib.o
	gcc -o controller_2 controller_2.o rt-lib.o -lrt -pthread

reference: reference.c parameters.h
	gcc -o reference reference.c -lrt

DIAG: DIAG.o rt-lib.o
	gcc -o DIAG DIAG.o rt-lib.o -lrt -pthread

plant.o: plant.c rt-lib.h parameters.h
	gcc -c plant.c

plant_2.o: plant_2.c rt-lib.h parameters.h
	gcc -c plant_2.c

controller.o: controller.c rt-lib.h parameters.h
	gcc -c controller.c

controller_2.o : controller_2.c rt-lib.h parameters.h
	gcc -c controller_2.c

rt-lib.o: rt-lib.c rt-lib.h
	gcc -c rt-lib.c

DIAG.o: DIAG.c rt-lib.h parameters.h
	gcc -c DIAG.c

clean:
	rm *.o plant plant_2 controller controller_2 reference DIAG
