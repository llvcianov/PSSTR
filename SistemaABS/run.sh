#!/bin/bash
gnome-terminal -- taskset -c 0 ./controller
gnome-terminal -- taskset -c 0 ./plant
gnome-terminal -- taskset -c 0 ./controller_2
gnome-terminal -- taskset -c 0 ./plant_2



