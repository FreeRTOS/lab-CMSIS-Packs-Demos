#!/bin/bash

buildtype=Release
image=AWS_MQTT_PubSub_Demo.$buildtype+AVH_MPS3_Corstone-300.axf
target=./out/AWS_MQTT_PubSub_Demo/AVH_MPS3_Corstone-300/$buildtype/$image

VHT_MPS3_Corstone_SSE-300 -C mps3_board.visualisation.disable-visualisation=1 \
-C mps3_board.telnetterminal0.start_telnet=0 \
-C mps3_board.uart0.out_file=- \
-a $target
