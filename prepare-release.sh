#!/bin/bash

# Script to compile all released enviroments and copy build artifacts to project root for easy upload to github releases. 

# firmware for active buzzer
pio run -e LGTISP_LGTDUDE_active
cp .pio/build/LGTISP_LGTDUDE_active/firmware.hex lgtisp_lgtdude_active_buzzer.hex


# firmware for passive buzzer
pio run -e LGTISP_LGTDUDE_passive
cp .pio/build/LGTISP_LGTDUDE_passive/firmware.hex lgtisp_lgtdude_passive_buzzer.hex

ls -la *.hex
echo "Done" 
