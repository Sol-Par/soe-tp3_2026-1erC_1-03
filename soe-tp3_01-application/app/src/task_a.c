/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

// TAREA PRODUCTORA.

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Other includes */
#include <stdlib.h>
#include <stdint.h>

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"

/********************** macros and definitions *******************************/
#define G_TASK_A_CNT_INI	0ul

#define TASK_A_DEL_MIN		(pdMS_TO_TICKS(0ul))
#define TASK_A_DEL_MAX		(pdMS_TO_TICKS(2500ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_a_wait_2500mS		= "   ==> Task A       - Wait:   2500mS";

/********************** external data declaration ****************************/
uint32_t g_task_a_cnt;
uint16_t dato;

/********************** external functions definition ************************/
/* Task thread */
void task_a(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_a_cnt = G_TASK_A_CNT_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_a_cnt++;

		/* Esperar espacio. */
		xSemaphoreTake(h_spaces_bin_sem, portMAX_DELAY);

		/* Tomar el mutex. */
		xSemaphoreTake(h_buffer_mutex, portMAX_DELAY);

		/* Producir 5 números aleatorios. */
		for (int i = 0; i < 5; i++) {
			dato = (uint16_t)(rand() % 65536);
			xQueueSend(h_buffer_queue, &dato, 0);
		}

		/* Avisar que hay items por procesar. */
		xSemaphoreGive(h_items_bin_sem);

		LOGGER_INFO("\n");
		LOGGER_INFO("PRODUCER - Datos enviados exitósamente a la cola.");

		/* Liberar el mutex. */
		xSemaphoreGive(h_buffer_mutex);

		/* Ejecutar (producir) cada 'TASK_A_DEL_MAX' milisegundos. */
		vTaskDelay(TASK_A_DEL_MAX);
	}
}

/********************** end of file ******************************************/
