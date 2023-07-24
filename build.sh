#!/bin/bash

case $1 in
    "-c")
        cbuild mqtt_pub_sub/AWS_MQTT_PubSub_Demo.Release+AVH_MPS3_Corstone-300.cprj -c
        ;;
    *)
	csolution convert -s IoT.csolution.yml -c AWS_MQTT_PubSub_Demo.Release+AVH_MPS3_Corstone-300
	cp mqtt_pub_sub/RTE/RTOS/FreeRTOSConfig.h RTE/RTOS/FreeRTOSConfig.h
        cbuild mqtt_pub_sub/AWS_MQTT_PubSub_Demo.Release+AVH_MPS3_Corstone-300.cprj
        ;;
esac
