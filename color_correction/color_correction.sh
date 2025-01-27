#!/bin/bash

file=$1
dcraw -4 "$file"
./color_correction "${file/.*/.ppm}" "${file/.*/.jpg}"
