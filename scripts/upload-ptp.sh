#!/bin/bash

# Compiles PTP master and slave environments. Uploads the master to the first
# available Teensy, and slave to any others present.

if ! command -v tycmd &>/dev/null; then
  echo "Error: requires tytools, which were not found https://github.com/Koromix/tytools" >&2
  echo
  exit 1
fi

# Build
cd "$(dirname "$(realpath "$0")")"/.. || exit 1
# Run
pio run -e clock-authority
pio run -e clock-subscriber
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
  for i in "${!teensies[@]}"; do
    if [ "$i" -eq 0 ]; then
      echo "Uploading clock-authority to Teensy $((i + 1)) of ${#teensies[@]}."
      tycmd upload .pio/build/clock-authority/firmware.hex -B "${teensies[$i]}"
    else
      echo "Uploading clock-subscriber to Teensy $((i + 1)) of ${#teensies[@]}."
      tycmd upload .pio/build/clock-subscriber/firmware.hex -B "${teensies[$i]}"
    fi
  done
  echo "Done!"
fi
