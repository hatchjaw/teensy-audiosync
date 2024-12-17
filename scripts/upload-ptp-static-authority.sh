#!/bin/bash

# Compiles PTP master and slave environments. Uploads the master to the first
# available Teensy, and slave to any others present.

if ! command -v tycmd &>/dev/null; then
  echo "Error: requires tytools, which were not found https://github.com/Koromix/tytools" >&2
  echo
  exit 1
fi

flags=${*:1}

# Build
cd "$(dirname "$(realpath "$0")")"/.. || exit 1
# Run
pio run -e clock-authority $flags
pio run -e clock-subscriber $flags
# GTFO if pio exited unhappily
if [ $? -eq 1 ]; then
    exit 1
fi

echo -e "\nLooking for Teensies to upload to."

# Query all connected Teensies
teensies=($(tycmd list | grep -Eo "[0-9]+-Teensy"))

# Upload
if ((${#teensies[@]} == 0)); then
  echo "No Teensies found."
else
  echo "Teensies found: ${#teensies[@]}"

  PS3="Select the device that will act as the clock authority: "

  select t in "${teensies[@]}"
  do
    authority="$t"
    echo "Uploading clock-authority."
    tycmd upload .pio/build/clock-authority/firmware.hex -B "$authority"

    # Filter the array so only subscribers remain:
    for i in "${!teensies[@]}"; do
      if [ "${teensies[i]}" != "$authority" ]; then
        subscribers+=("${teensies[i]}")
      fi
    done
    break
  done

  for i in "${!subscribers[@]}"; do
      echo "Uploading clock-subscriber $((i + 1)) of ${#subscribers[@]}."
      tycmd upload .pio/build/clock-subscriber/firmware.hex -B "${subscribers[$i]}"
  done
  echo "Done!"
fi
