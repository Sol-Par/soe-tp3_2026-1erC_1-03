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

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"

/********************** macros and definitions *******************************/
#define G_TASK_B_CNT_INI	0ul

#define TASK_B_DEL_ZERO		(pdMS_TO_TICKS(0ul))
#define TASK_B_DEL_MAX		(pdMS_TO_TICKS(2500ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_b_wait_2500mS		= "   ==> Task B       - Wait:   2500mS";

/********************** external data declaration ****************************/
uint32_t g_task_b_cnt;

extern uint32_t readers;
extern SemaphoreHandle_t h_lectores_mutex;
extern SemaphoreHandle_t h_habitacion_disponible_bin_sem;
extern SemaphoreHandle_t h_molinete_mutex;

/********************** external functions definition ************************/
/* Task thread */
void task_b(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_b_cnt = G_TASK_B_CNT_INI;

	char *nombre = (char *)parameters;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
    {
		/* Update Task Counter */
		g_task_b_cnt++;

		/* 1. Pasa por el molinete. Si hay un escritor esperando, se bloquea aquí. */
		xSemaphoreTake(h_molinete_mutex, portMAX_DELAY);
		xSemaphoreGive(h_molinete_mutex);

		/* 2. Lógica estándar del Lector (Proteger contador) */
		xSemaphoreTake(h_lectores_mutex, portMAX_DELAY);
			readers++;
			if (readers == 1) {
				xSemaphoreTake(h_habitacion_disponible_bin_sem, portMAX_DELAY);
			}
		xSemaphoreGive(h_lectores_mutex);

		/* ---> INICIO SECCIÓN CRÍTICA <--- */
		LOGGER_INFO("%s: Leyendo datos... (Lectores activos: %lu)", nombre, readers);
		vTaskDelay(pdMS_TO_TICKS(500));
		/* ---> FIN SECCIÓN CRÍTICA <--- */

		/* 3. Lógica de salida (Proteger contador) */
		xSemaphoreTake(h_lectores_mutex, portMAX_DELAY);
			readers--;
			if (readers == 0) {
				xSemaphoreGive(h_habitacion_disponible_bin_sem);
			}
		xSemaphoreGive(h_lectores_mutex);
	}
}

/********************** end of file ******************************************/
