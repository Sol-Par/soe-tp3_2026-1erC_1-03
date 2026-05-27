# Análisis del Código Fuente (FreeRTOS)

Este documento detalla el funcionamiento del esqueleto base para un sistema operativo de tiempo real (**RTOS**) utilizando **FreeRTOS** (con la abstracción de CMSIS-OS). El propósito principal del ejercicio, es resolver el clásico patrón de sincronización **Productor-Consumidor** (basado en el libro *"The Little Book of Semaphores"* de Allen B. Downey).

Cabe destacar que el código provisto se encuentra en una **etapa inicial (esqueleto)**: las tareas están declaradas y se ejecutan, pero aún **no tienen implementada la lógica de comunicación** (colas/queues) ni de sincronización (semáforos o mutex).

---

## 1. Análisis por Archivo (.c)

### 📄 `app.c` (Inicialización de la Aplicación)
Este es el punto central de configuración del sistema de tareas.
* **Variables de control globales:** Inicializa variables de diagnóstico como `g_app_cnt` (contador general), `g_task_idle_cnt` (contador de tiempo inactivo) y `g_app_stack_overflow_cnt` (contador de desbordamientos de pila).
* **Función `app_init()`:** * Imprime en el log el nombre del TP (`seo-tp3_01-application: Producer-Consumer`).
  * **Creación de Tareas:** Utiliza la función nativa de FreeRTOS `xTaskCreate` para dar de alta dos hilos de ejecución independientes: `task_a` (Task A) y `task_b` (Task B).
  * **Prioridades:** Ambas tareas se crean con la misma prioridad: `(tskIDLE_PRIORITY + 1ul)`. Al tener la misma prioridad, el planificador (*scheduler*) de FreeRTOS alternará su ejecución de manera equitativa (*Round Robin*) si ambas están listas para ejecutar.
  * **Manejo de errores:** Utiliza `configASSERT(pdPASS == ret)` para detener inmediatamente el microcontrolador si alguna tarea no pudo crearse por falta de memoria (*Heap*).
  * Inicializa las interrupciones de la app (`app_it_init()`) y un contador de ciclos de hardware (`cycle_counter_init()`).

### 📄 `task_a.c` y `task_b.c` (Las Tareas)
Ambos archivos tienen una estructura prácticamente idéntica en este momento. Representan a los actores del sistema (que idealmente serán el Productor y el Consumidor).
* **Estructura:** Ambas funciones contienen un bucle infinito (`for (;;)`), que es el estándar para tareas en sistemas embebidos operados por RTOS.
* **Comportamiento actual:**
  1. Incrementan su respectivo contador local (`g_task_a_cnt` y `g_task_b_cnt`) para auditoría.
  2. Envían un mensaje al registrador (`LOGGER_INFO`) indicando que van a entrar en espera.
  3. Liberan el procesador utilizando la función **`vTaskDelay(TASK_A_DEL_MAX)`** (establecido en 2500 milisegundos).
* **Mecanismo de Bloqueo:** Al llamar a `vTaskDelay`, la tarea pasa del estado *Running* (Ejecución) al estado *Blocked* (Bloqueado). Esto le dice al RTOS que no intente ejecutar esta tarea durante los próximos 2.5 segundos, permitiendo que la CPU descanse o ejecute tareas de menor prioridad.

### 📄 `freertos.c` (Funciones de Gancho o Hooks)
Este archivo define funciones "callback" que FreeRTOS llama automáticamente ante eventos específicos del sistema operativo:
* **`vApplicationIdleHook()`:** Se ejecuta repetidamente únicamente cuando **ninguna** tarea de la aplicación está lista para correr (en este caso, cuando tanto Task A como Task B están bloqueadas en su `vTaskDelay`). Actualmente solo incrementa `g_task_idle_cnt`, pero suele usarse para poner al procesador en modo de bajo consumo (*Low Power Mode*).
* **`vApplicationTickHook()`:** Se ejecuta dentro de la Interrupción del Tick del sistema (un temporizador de hardware que interrumpe la CPU típicamente cada 1 ms). Incrementa `g_app_tick_cnt`. *Nota: Al ser una interrupción (ISR), debe ser sumamente rápida.*
* **`vApplicationStackOverflowHook(...)`:** Si una tarea consume más memoria de la asignada a su pila (*Stack*), esta función atrapa el error. Tiene un `configASSERT( 0 )` que congela la ejecución del microcontrolador en un bucle infinito para que el desarrollador pueda conectar un depurador (*debugger*) y encontrar qué tarea falló.

### 📄 `app_it.c` (Interrupciones de la Aplicación)
Este archivo está destinado a configurar y manejar las interrupciones del hardware propias de la aplicación.
* En la función `app_it_init()`, actualmente solo se muestra un ejemplo de código en ensamblador inline para deshabilitar (`CPSID i`) y volver a habilitar (`CPSIE i`) las interrupciones globales, una técnica común para crear **secciones críticas** cortas y proteger recursos compartidos contra condiciones de carrera.

---

## 2. Resumen del Flujo de Ejecución Actual

1. El sistema arranca, se llama a `app_init()`, y se registran **Task A** y **Task B**.
2. El planificador de FreeRTOS toma el control. Como ambas tareas tienen prioridad 1, arranca una de ellas (por ejemplo, Task A).
3. **Task A** se ejecuta, imprime su log, y se bloquea por 2500 ms con `vTaskDelay`.
4. Inmediatamente, el planificador ve que **Task B** está lista. La ejecuta, esta imprime su log y también se bloquea por 2500 ms.
5. Como ambas tareas están bloqueadas, FreeRTOS le da el control a la **Tarea Idle** (Prioridad 0). El procesador se queda ejecutando `vApplicationIdleHook()` e incrementando el contador de tiempo ocioso.
6. Cada 1 ms, el temporizador del sistema genera una interrupción, ejecuta `vApplicationTickHook()` e incrementa el contador de ticks de la app.
7. Al cumplirse los 2500 ms, ambas tareas se despiertan y el ciclo se repite.
