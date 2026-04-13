/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

typedef struct {
    int pin;
    int delay;
} but_t;

QueueHandle_t xQueueButId;

SemaphoreHandle_t xSemaphore_r;
SemaphoreHandle_t xSemaphore_y;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4 && gpio == BTN_PIN_R) { // fall edge
        xSemaphoreGiveFromISR(xSemaphore_r, 0);
    }
    if (events == 0x4 && gpio == BTN_PIN_Y) { // fall edge
        xSemaphoreGiveFromISR(xSemaphore_y, 0);
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int delay = 0;
    but_t msg;

    while (true) {
        if (xQueueReceive(xQueueButId, &msg, 0) == pdTRUE) {
            if (msg.pin == LED_PIN_R) {
                delay = msg.delay;
                // printf("led_r delay: %d\n", delay);
            } else {
                // Não é para mim, devolve para a fila
                xQueueSend(xQueueButId, &msg, 0);
            }
        }

        if (delay > 0) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        } else {
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int delay = 0;
    but_t msg;

    while (true) {
        if (xQueueReceive(xQueueButId, &msg, 0) == pdTRUE) {
            if (msg.pin == LED_PIN_Y) {
                delay = msg.delay;
                // printf("led_y delay: %d\n", delay);
            } else {
                // Não é para mim, devolve para a fila
                xQueueSend(xQueueButId, &msg, 0);
            }
        }

        if (delay > 0) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        } else {
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void btn_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled_with_callback(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);

    bool r_active = false;
    bool y_active = false;

    while (true) {
        if (xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(500)) == pdTRUE) {
            r_active = !r_active;
            but_t msg = {LED_PIN_R, r_active ? 100 : 0};
            // printf("btn_r: %s\n", r_active ? "ON" : "OFF");
            xQueueSend(xQueueButId, &msg, 0);
        }

        if (xSemaphoreTake(xSemaphore_y, pdMS_TO_TICKS(500)) == pdTRUE) {
            y_active = !y_active;
            but_t msg = {LED_PIN_Y, y_active ? 100 : 0};
            // printf("btn_y: %s\n", y_active ? "ON" : "OFF");
            xQueueSend(xQueueButId, &msg, 0);
        }
    }
}

int main() {
    stdio_init_all();
    printf("Start RTOS \n");

    xQueueButId = xQueueCreate(32, sizeof(but_t));
    xSemaphore_r = xSemaphoreCreateBinary();
    xSemaphore_y = xSemaphoreCreateBinary();

    xTaskCreate(led_r_task, "LED_Task R", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Task Y", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}