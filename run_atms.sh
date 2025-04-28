#!/bin/bash
read -p "Enter number of ATM users to launch: " NUM
for i in $(seq 1 $NUM); do
    gnome-terminal -- bash -c "./atm; exec bash"
done