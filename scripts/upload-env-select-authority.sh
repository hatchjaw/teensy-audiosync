#!/bin/bash

# Compiles PTP clock-authority and clock subscriber environments.
# Prompts for a device to which to upload the clock-authority-usb-audio
# environment; uploads the clock subscriber environment to any others present.

if ! command -v tycmd &>/dev/null; then
  echo "Error: requires tytools, which were not found https://github.com/Koromix/tytools" >&2
  echo
  exit 1
fi

environment=$1
if [ -z "$environment" ]; then
  echo "Expected to receive a platformIO environment. Defaulting to 'clock-subscriber'."
  environment="clock-subscriber"
fi

flags=${*:2}

# Build
cd "$(dirname "$(realpath "$0")")"/.. || exit 1
# Run
pio run -e clock-authority-usb-audio $flags
pio run -e $environment $flags
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

    # Filter the array so only non-authorities remain:
    for i in "${!teensies[@]}"; do
      if [ "${teensies[i]}" != "$authority" ]; then
        others+=("${teensies[i]}")
      fi
    done
    break
  done

  for i in "${!others[@]}"; do
      echo "Uploading $environment $((i + 1)) of ${#others[@]}."
      tycmd upload .pio/build/$environment/firmware.hex -B "${others[$i]}"
  done
  echo "Done!"
fi
