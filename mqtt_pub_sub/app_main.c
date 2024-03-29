/* -----------------------------------------------------------------------------
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "iot_logging_task.h"
#include "mbedtls/threading.h"
#include "threading_alt.h"
#include "mqtt_agent_task.h"


/* Set logging task as high priority task */
#define LOGGING_TASK_PRIORITY                         (configMAX_PRIORITIES - 1)
#define LOGGING_TASK_STACK_SIZE                       (1440)
#define LOGGING_MESSAGE_QUEUE_LENGTH                  (15)

/** 
 * Set Mutual auth demo task stack size and priority.
 * Task stack size should be sufficient for TLS handshake to succeed.
 */
#define MQTT_MUTUAL_AUTH_DEMO_TASK_STACK_SIZE     ( 4096 )
#define MQTT_MUTUAL_AUTH_DEMO_TASK_PRIORITY       ( tskIDLE_PRIORITY + 1)

/**
 * @brief Subscribe Publish demo tasks configuration.
 * Subscribe publish demo task shows the basic functionality of connecting to an MQTT broker, subscribing
 * to a topic, publishing messages to a topic and reporting the incoming messages on subscribed topic.
 * Number of subscribe publish demo tasks to be spawned is configurable.
 */
#define appmainMQTT_NUM_PUBSUB_TASKS              ( 2 )
#define appmainMQTT_PUBSUB_TASK_STACK_SIZE        ( 2048 )
#define appmainMQTT_PUBSUB_TASK_PRIORITY          ( tskIDLE_PRIORITY + 1 )

/**
 * @brief Stack size and priority for MQTT agent task.
 * Stack size is capped to an adequate value based on requirements from MbedTLS stack
 * for establishing a TLS connection. Task priority of MQTT agent is set to a priority
 * higher than other MQTT application tasks, so that the agent can drain the queue
 * as work is being produced.
 */
#define appmainMQTT_AGENT_TASK_STACK_SIZE         ( 4096 )
#define appmainMQTT_AGENT_TASK_PRIORITY           ( tskIDLE_PRIORITY + 2 )

extern int32_t network_startup (void);

static const osThreadAttr_t app_main_attr = {
  .stack_size = 4096U
};


#if (configAPPLICATION_ALLOCATED_HEAP == 1U)
#if !(defined(configHEAP_REGION0_ADDR) && (configHEAP_REGION0_ADDR != 0U))
static uint8_t heap_region0[configHEAP_REGION0_SIZE] __ALIGNED(8);
#endif

const HeapRegion_t xHeapRegions[] = {
#if defined(configHEAP_REGION0_ADDR) && (configHEAP_REGION0_ADDR != 0U)
 { (uint8_t *)configHEAP_REGION0_ADDR, configHEAP_REGION0_SIZE },
#else
 { (uint8_t *)heap_region0, configHEAP_REGION0_SIZE },
#endif
 { NULL, 0 }
};
#endif


extern void vCoreMQTTMutualAuthDemoTask( void * pvParam );

extern BaseType_t xStartPubSubTasks( uint32_t ulNumPubsubTasks,
                                     configSTACK_DEPTH_TYPE uxStackSize,
                                     UBaseType_t uxPriority );
/*---------------------------------------------------------------------------
 * Application main thread
 *---------------------------------------------------------------------------*/
static void app_main (void *argument) {
  int32_t status;
  BaseType_t xResult;

  (void)argument;
  osThreadAttr_t taskAttr;

  status = network_startup();

  if (status == 0) {
    memset( &taskAttr, 0x00, sizeof( taskAttr ) );

    mbedtls_threading_set_alt( mbedtls_platform_mutex_init,
                               mbedtls_platform_mutex_free,
                               mbedtls_platform_mutex_lock,
                               mbedtls_platform_mutex_unlock );


    /* Uncomment this if you want to run the demo without MQTT agent. */
 
    /**
    taskAttr.name = "MQTTDemo";
    taskAttr.attr_bits = osThreadDetached;
    taskAttr.stack_size = MQTT_MUTUAL_AUTH_DEMO_TASK_STACK_SIZE;
    taskAttr.priority = MQTT_MUTUAL_AUTH_DEMO_TASK_PRIORITY;
    osThreadNew(vCoreMQTTMutualAuthDemoTask, NULL, &taskAttr);
    **/

    /* Start agent task. */
    xResult = xMQTTAgentInit( appmainMQTT_AGENT_TASK_STACK_SIZE, appmainMQTT_AGENT_TASK_PRIORITY );

    //Start demo task once agent task is started.
    if( xResult == pdPASS )
    {
      ( void ) xStartPubSubTasks( appmainMQTT_NUM_PUBSUB_TASKS,
                                  appmainMQTT_PUBSUB_TASK_STACK_SIZE,
                                  appmainMQTT_PUBSUB_TASK_PRIORITY );
    }
    /* Wait such that task is not deleted */
    for (;;) {
      osDelay(osWaitForever);
    }
  }

}

/*---------------------------------------------------------------------------
 * Application initialization
 *---------------------------------------------------------------------------*/
void app_initialize (void) {

#if (configAPPLICATION_ALLOCATED_HEAP == 1U)
  vPortDefineHeapRegions (xHeapRegions);
#endif

  /* Create logging task */
  xLoggingTaskInitialize (LOGGING_TASK_STACK_SIZE,
                          LOGGING_TASK_PRIORITY,
                          LOGGING_MESSAGE_QUEUE_LENGTH);

  osThreadNew(app_main, NULL, &app_main_attr);
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
     * function then they must be declared static - otherwise they will be allocated on
     * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/**
 * @brief This is to provide the memory that is used by the RTOS daemon/time task.
 *
 * If configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetTimerTaskMemory() to provide the memory that is
 * used by the RTOS daemon/time task.
 */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
    /* If the buffers to be provided to the Timer task are declared inside this
     * function then they must be declared static - otherwise they will be allocated on
     * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/
