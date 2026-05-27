# Análisis del Código Fuente (FreeRTOS)

Este documento detalla el funcionamiento del esqueleto base para un sistema operativo de tiempo real (**RTOS**) utilizando **FreeRTOS** (con la abstracción de CMSIS-OS). El propósito fundamental de esta variante del código, según se deduce de la arquitectura de archivos actuales (`task_entry_a.c`, `task_exit_a.c` y `task_test.c`), es modelar un entorno de simulación para un sistema de control de accesos (con eventos de entrada/ingreso y salida/egreso) gobernado por un inyector de estímulos de prueba de alta prioridad (`task_test`).

Al igual que en las versiones anteriores, el firmware se encuentra en una **etapa inicial (esqueleto)**: las tareas están declaradas y se registran en el núcleo correctamente, pero las funciones definitivas de sincronización o colas de comunicación entre la tarea de prueba y las de acceso se encuentran simplificadas temporalmente mediante retardos de tiempo.

---

## 1. Análisis por Archivo (.c)

### 📄 `app.c` (Inicialización de la Aplicación)
Este es el punto central de configuración del sistema de tareas.
* **Variables de control globales:** Inicializa variables de diagnóstico como `g_app_cnt` (contador general), `g_task_idle_cnt` (contador de tiempo inactivo), `g_app_tick_cnt` (contador de ticks) y `g_app_stack_overflow_cnt` (contador de desbordamientos de pila).
* **Función `app_init()`:**
  * Imprime en el log el identificador de origen del software educativo (`Source => TA149 - Sistemas Operativos Embebidos`).
  * **Creación de Tareas:** Utiliza la función nativa de FreeRTOS `xTaskCreate` para dar de alta tres hilos de ejecución independientes de forma simultánea: `task_entry_a` ("Task Entry A"), `task_exit_a` ("Task Exit A") y `task_test` ("Task Test").
  * **Prioridades Iniciales:** Todas las tareas se crean inicialmente con el mismo nivel de prioridad base: `(tskIDLE_PRIORITY + 1ul)`. Bajo esta condición, el planificador (*scheduler*) de FreeRTOS alternará su ejecución mediante un esquema equitativo (*Round Robin*) si estuviesen listas al mismo tiempo.
  * **Manejo de errores:** Utiliza la macro `configASSERT(pdPASS == ret)` después de cada creación para detener inmediatamente el microcontrolador si alguna tarea no pudo crearse por falta de memoria libre en el *Heap*.
  * Inicializa las interrupciones de la app (`app_it_init()`) y el contador de ciclos de hardware (`cycle_counter_init()`).

### 📄 `task_entry_a.c` y `task_exit_a.c` (Tareas de Entrada y Salida)
Ambos archivos tienen una estructura e implementación prácticamente idéntica en este momento. Representan a los actores encargados de procesar los eventos de ingreso (Entry) y egreso (Exit) respectivamente en el canal 'A'.
* **Estructura:** Ambas funciones contienen un bucle infinito obligado (`for (;;)`), estándar para tareas en sistemas embebidos operados por RTOS.
* **Comportamiento actual:**
  1. Incrementan su respectivo contador global local (`g_task_entry_a_cnt` y `g_task_exit_a_cnt`) para auditoría de ciclos.
  2. Envían un mensaje al registrador (`LOGGER_INFO`) indicando que van a entrar en espera de 2500 ms.
  3. Liberan el procesador utilizando la función de bloqueo por software **`vTaskDelay(TASK_ENTRY_A_DEL_MAX / TASK_EXIT_A_DEL_MAX)`** (establecido en 2500 milisegundos).
* **Mecanismo de Bloqueo:** Al llamar a `vTaskDelay`, la tarea pasa del estado *Running* (Ejecución) al estado *Blocked* (Bloqueado). Esto le notifica al RTOS que excluya esta tarea de la planificación durante los próximos 2.5 segundos, permitiendo que la CPU quede libre para otros hilos.

### 📄 `task_test.c` (Tarea de Pruebas e Inyección de Eventos)
Esta tarea actúa como un estimulador de software encargado de simular ráfagas de eventos para validar el comportamiento del microcontrolador.
* **Dinámica de Prioridad Dinámica:** Al iniciar, la tarea consulta su propia prioridad con `uxTaskPriorityGet(NULL)` y se auto-asigna una prioridad superior sumándole dos niveles (`+ 2ul`) mediante la función de la API `vTaskPrioritySet`. Al convertirse dinámicamente en el hilo con mayor prioridad del sistema (Prioridad 3), FreeRTOS suspende temporalmente la ejecución de las tareas de prioridad 1 para asegurar que la inyección de pruebas no sufra demoras (*preemption*).
* **Bucle de Excitación:** Dentro de su propio bucle continuo `for (;;)`, recorre un arreglo estático de eventos predefinidos (`e_task_test_array`) utilizando un índice incremental.
* **Estructura de Control:** Utiliza una sentencia condicional `switch (e_task_test_array[index])` para discernir qué tipo de evento simular en cada ciclo (como emular una señal `Entry_A` o una señal `Exit_A`). En este esqueleto, los bloques `case` están vacíos y finalizan inmediatamente con un `break`, listos para que el desarrollador reemplace esta lógica pasiva por semáforos o colas que despierten activamente a las tareas de entrada y salida.

### 📄 `freertos.c` (Funciones de Gancho o Hooks)
Este archivo define funciones "callback" que el núcleo de FreeRTOS llama automáticamente ante eventos globales del sistema operativo:
* **`vApplicationIdleHook()`:** Se ejecuta repetidamente únicamente cuando **ninguna** tarea de la aplicación está lista para correr (en este caso, cuando todas las tareas de la app están bloqueadas por tiempo). Incrementa `g_task_idle_cnt`, y se usa habitualmente para poner al procesador en modo de bajo consumo (*Low Power Mode*).
* **`vApplicationTickHook()`:** Se ejecuta dentro de la Interrupción del Tick del sistema (un temporizador de hardware que interrumpe la CPU típicamente cada 1 ms para actualizar los tiempos del kernel). Incrementa `g_app_tick_cnt`. Al ser una rutina de servicio de interrupción (ISR), debe ser sumamente veloz y eficiente.
* **`vApplicationStackOverflowHook(...)`:** Si una tarea consume más memoria de la asignada a su pila (*Stack*), esta función atrapa el error. Tiene una llamada crítica `configASSERT( 0 )` que congela la ejecución del microcontrolador en un bucle infinito para que el desarrollador pueda conectar un depurador y diagnosticar qué hilo desbordó sus límites.

### 📄 `app_it.c` (Interrupciones de la Aplicación)
Este archivo está destinado a configurar y manejar las interrupciones del hardware asociadas a la lógica del firmware.
* En la función `app_it_init()`, se muestra código en ensamblador inline (`__asm("CPSID i")` para deshabilitar interrupciones globales y `__asm("CPSIE i")` para volver a habilitarlas) utilizado para delimitar una **sección crítica** corta, garantizando que variables compartidas queden protegidas de condiciones de carrera.
* **`HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)`:** Es la función de callback de la capa de abstracción de hardware (HAL) para procesar interrupciones externas por flanco (como por ejemplo, el botón de usuario físico `BTN_...`).

---

## 2. Resumen del Flujo de Ejecución Actual

1. El sistema arranca, se llama a `app_init()`, inicializando las variables globales y registrando en el kernel las tareas **Task Entry A**, **Task Exit A** y **Task Test** con prioridad inicial 1.
2. El planificador de FreeRTOS toma el control de la CPU. Comienza ejecutando una de las tareas creadas. Al darle el turno a **Task Test**, esta ejecuta de inmediato `vTaskPrioritySet`, elevando dinámicamente su prioridad al nivel 3.
3. Debido a este cambio, **Task Test** se convierte en el hilo dominante. Ejecuta secuencialmente su bucle interno de escaneo sobre el arreglo de eventos (`e_task_test_array`), procesando los estados e incrementando `g_task_test_cnt`.
4. Una vez que la tarea de prueba realiza una acción bloqueante (como un retraso para espaciar los estímulos) o cede el procesador, el planificador evalúa el resto de las tareas que siguen listas en prioridad 1.
5. **Task Entry A** y **Task Exit A** se ejecutan alternadamente compartiendo el tiempo por *Round Robin*; incrementan sus respectivos contadores locales, imprimen su log en consola y se auto-bloquean por 2500 ms invocando `vTaskDelay`.
6. En las ventanas de tiempo donde todas las tareas de la aplicación se encuentran simultáneamente en estado Bloqueado (*Blocked*), FreeRTOS le cede el control a la **Tarea Idle** (Prioridad 0). El procesador se queda ejecutando `vApplicationIdleHook()` e incrementando el contador de tiempo ocioso `g_task_idle_cnt`.
7. Cada 1 milisegundo, el temporizador de hardware genera una interrupción física que ejecuta `vApplicationTickHook()` e incrementa `g_app_tick_cnt`. Al cumplirse la ventana de 2500 ms, las tareas de acceso se despiertan y el ciclo se repite.
