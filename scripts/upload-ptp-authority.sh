#!/bin/bash

# Compiles PTP clock-authority-with-USB-audio environment.
# Prompts for selection of a device to which to upload.

if ! command -v tycmd &>/dev/null; then
  echo "Error: requires tytools, which were not found https://github.com/Koromix/tytools" >&2
  echo
  exit 1
fi

flags=${*:1}

cd "$(dirname "$(realpath "$0")")"/.. || exit 1
# Build
pio run -e clock-authority-usb-audio $flags
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
    tycmd upload .pio/build/clock-authority-usb-audio/firmware.hex -B "$authority"
    break
  done

  echo "Done!"
fi
