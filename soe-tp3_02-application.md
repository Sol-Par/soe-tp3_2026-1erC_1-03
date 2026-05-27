# Análisis del Código Fuente (FreeRTOS)

Este documento detalla el funcionamiento del esqueleto base para un sistema operativo de tiempo real (**RTOS**) utilizando **FreeRTOS** (con la abstracción de CMSIS-OS). El propósito principal de este código, es resolver el clásico patrón de sincronización **Lectores-Escritores (Readers-Writers problem)**, basado en el libro *"The Little Book of Semaphores"* de Allen B. Downey.

Cabe destacar que el código provisto se encuentra en una **etapa inicial (esqueleto)**: las tareas están declaradas y se ejecutan, pero aún **no tienen implementada la lógica de comunicación ni de sincronización** (semáforos binarios, contadores o mutex) necesaria para gestionar el acceso concurrente al recurso compartido.

---

## 1. Análisis por Archivo (.c)

### 📄 `app.c` (Inicialización de la Aplicación)
Este es el punto de entrada y configuración central de todo el entorno de hilos de ejecución de la aplicación.
* **Variables de control globales:** Inicializa variables globales de instrumentación diagnóstica como `g_app_cnt` (contador de la app), `g_task_idle_cnt` (contador de tiempo inactivo), `g_app_tick_cnt` (contador de ticks) y `g_app_stack_overflow_cnt` (contador de desbordamientos de pila).
* **Función `app_init()`:**
  * Imprime en los registros de depuración el nombre del trabajo práctico (`seo-tp3_02-application: Readers-Writers`).
  * **Creación de Tareas:** Utiliza la función nativa de FreeRTOS `xTaskCreate` para instanciar dos hilos de ejecución independientes: `task_a` (Task A) y `task_b` (Task B).
  * **Prioridades:** Ambas tareas se crean compartiendo el mismo nivel de prioridad: `(tskIDLE_PRIORITY + 1ul)`. Al tener la misma prioridad, el planificador (*scheduler*) de FreeRTOS conmutará el uso del procesador de manera equitativa (*Round Robin*) mediante la división de tiempo compartido, siempre que ambas estén listas para ejecutar.
  * **Manejo de errores:** Utiliza la macro de aserción `configASSERT(pdPASS == ret)` después de cada creación para congelar el sistema de inmediato si alguna tarea no pudo reservar su espacio de memoria en el *Heap*.
  * Invoca la inicialización de interrupciones propias de la app (`app_it_init()`) y el contador de ciclos por hardware (`cycle_counter_init()`).

### 📄 `task_a.c` y `task_b.c` (Las Tareas)
Ambos archivos tienen una estructura e implementación prácticamente idéntica en este punto. Representan a los actores del sistema que eventualmente asumirán los roles de Lector y Escritor.
* **Estructura:** Ambas funciones contienen un bucle infinito obligado (`for (;;)`), que es el estándar de diseño para hilos de ejecución continuos dentro de un RTOS.
* **Comportamiento actual:**
  1. Incrementan su respectivo contador local (`g_task_a_cnt` y `g_task_b_cnt`) para auditoría de ciclos.
  2. Envían un mensaje al registrador (`LOGGER_INFO`) indicando que van a entrar en espera de 2500 ms.
  3. Liberan el procesador utilizando la función de bloqueo por software **`vTaskDelay(TASK_A_DEL_MAX)`** / **`TASK_B_DEL_MAX`** (equivalente a 2500 milisegundos).
* **Mecanismo de Bloqueo:** Al llamar a `vTaskDelay`, la tarea transiciona del estado *Running* (Ejecución) al estado *Blocked* (Bloqueado). Esto le notifica al núcleo de FreeRTOS que no debe planificar esta tarea durante los próximos 2.5 segundos, dejando la CPU libre para tareas de menor prioridad o para el reposo.

### 📄 `freertos.c` (Funciones de Gancho o Hooks)
Este archivo define funciones "callback" que el núcleo de FreeRTOS llama automáticamente ante eventos globales del sistema operativo:
* **`vApplicationIdleHook()`:** Se ejecuta repetidamente únicamente cuando **ninguna** tarea de la aplicación está en estado Listo (*Ready*) para correr (en este caso, cuando tanto Task A como Task B están bloqueadas en su `vTaskDelay`). Incrementa el contador `g_task_idle_cnt` y es el espacio ideal para colocar al microcontrolador en modo de bajo consumo (*Low Power State*).
* **`vApplicationTickHook()`:** Se ejecuta dentro del contexto de la Interrupción del Tick del sistema (un temporizador de hardware que interrumpe la CPU típicamente cada 1 ms para actualizar los tiempos del RTOS). Incrementa `g_app_tick_cnt`. Al ejecutarse desde una ISR, es sumamente rápida y no bloqueante.
* **`vApplicationStackOverflowHook(...)`:** Si una tarea consume más memoria de la asignada originalmente a su pila (*Stack*), FreeRTOS detecta la anomalía e invoca esta función. Entra en una sección crítica y ejecuta un `configASSERT( 0 )` que congela el microcontrolador en un bucle infinito. Esto permite adjuntar un depurador en tiempo de desarrollo y capturar qué tarea destruyó su pila.

### 📄 `app_it.c` (Interrupciones de la Aplicación)
Este archivo está destinado a configurar y manejar las rutinas de interrupción del hardware específicas de la aplicación.
* En la función `app_it_init()`, actualmente cuenta con líneas en ensamblador embebido (`__asm("CPSID i")` para deshabilitar las interrupciones globales y `__asm("CPSIE i")` para volver a habilitarlas). Esto representa el esqueleto típico empleado para delimitar una **sección crítica** corta en sistemas basados en ARM Cortex-M, garantizando que el código intermedio no sea interrumpido por condiciones de carrera de hardware.

---

## 2. Resumen del Flujo de Ejecución Actual

1. El sistema arranca, se llama a `app_init()`, inicializando los contadores y registrando en el kernel la **Task A** y la **Task B** con prioridad 1.
2. El planificador (*scheduler*) de FreeRTOS toma el control de la CPU. Al notar que ambas tareas tienen prioridad 1, arranca la primera de ellas (por ejemplo, Task A).
3. **Task A** se ejecuta, incrementa su contador, imprime su log y se auto-bloquea por 2500 ms invocando `vTaskDelay`.
4. Inmediatamente, el planificador evalúa que **Task B** está en estado Listo. Le otorga la CPU, ejecuta su ciclo, imprime su respectivo log y también se bloquea por 2500 ms con su propio `vTaskDelay`.
5. Dado que tanto Task A como Task B se encuentran en estado Bloqueado (*Blocked*), FreeRTOS le cede el control del procesador a la **Tarea Idle** (Prioridad 0). El procesador se queda ejecutando en bucle la función `vApplicationIdleHook()`, lo que incrementa el contador de tiempo ocioso `g_task_idle_cnt`.
6. Cada 1 ms, el temporizador de hardware interrumpe el flujo normal para procesar el Tick del sistema, disparando `vApplicationTickHook()` e incrementando `g_app_tick_cnt`.
7. Al cumplirse la ventana de tiempo de 2500 ms, ambas tareas despiertan de forma simultánea pasando a estado Listo, y el ciclo de alternancia vuelve a repetirse indefinidamente.


---

## Implementación y comportamiento observado:
