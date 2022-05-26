#!/bin/bash

for N in {1..3}
do
  if [[ $N -gt 40 ]]
  then
    ./client localhost 8080 /file_example_MP4_480_1_5MG.mp4 &
  else
    ./client localhost 8080 /home.html &
  fi
done
wait