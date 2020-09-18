# P1_embebidos2

La practica consistió en codificar 11 funciones a partir de un pseudocódigo dado, lo más complejo era comprender ciertas funciones y como estas funcionaban.
Uno de los grandes cambios que se realizaron fue que se hizo la definción de una macro los tipos de tasks, ya que habían unos que en ciertas funciones se llamban
mucho, y era más sencillo usar una macro muy clara de este que asignarlo a una variable o hacer la declaración completa. Esto se puede ver en las lineas 50 y 51,
en las que definimos las macros para tarea nueva y tarea actual.

La primera función era sencilla, solo poner el reloj global en 0, crear la tarea idle y quedarse en un bucle que no hace nada.

La segunda función es un poco más compleja, primero se checa que nTasks no haya superado el limite definido de tareas, en caso de que sí regresa error,
si se pudo crear entonces asigna la prioridad y luego revisa si tiene autostart para definir que modo se inicializara con un operador ternario. Asigna el
stack pointer a partir de las macros definidas. El stack a partir de macros definimos la posición donde irá la dirección de retorno y lo mismo con el PSR.
Al final aumentamos el número de tasks ya que se pudo hacer una exitosa y regresamos este número menos 1 para indicar el indice.

En la tercera simplemente regresamos el valor del global tick.

La cuarta lo que hacemos es poner la tarea actual en WAITING, y modificamos su local_ticks, luego se llamaba al dispatcher mandandole Normal Exec para que fuera
desde la tarea.

En la quinta poniamos la tarea actual en SUSPENDED y llamamos al dispatcher otra vez con Normal Exec.

La sexta es cambiar el estado de la tarea que se indico por READY, esto lo hacemos simplemente accediendo a la posición del arreglo que coincide con la tarea,
una vez hecho esto llamamos al dispatcher en Normal Exec.

El septimo ya es el dispatcher, el cual es una de las tareas esenciales del programa, en esta lo que hicimos fue, a partir de un buce for, hacer un swipe de 
las tareas que estuvieran creadas en el programa y revisar cual es la de mayor prioridad, de tal forma que la siguiente de mayor prioridad fuera la siguiente a
ser ejecutada, siempre y cuando este en READY o RUNNING, una vez encontrada esta tarea, si fuera diferente a la que se esta ejecutando actualmente, se hace el
context switch, si es la misma se sigue ejecutando.

El context switch es la siguiente función, en esta lo que hicimos fue tener una variable de tipo static que nos sirviera para saber si es la primera vez que
se ejecuta esta función o no, en caso de que no fuera la primera entonces cargabamos el stack pointer del procesador al stack pointer de la tarea actual, y
en caso de que fuera Normal Exec o ISR, realizabamos el ajuste del stack pointer adecuado.  Una vez hecho esto, asignabamos la siguiente tarea a realizar, 
según lo obtenido en el dispatcher y la dejabamos en READY

La función Activate Waiting Task es sencilla, lo que hace es por medio de un for hacer un sweep a todas las tareas existentes, y si encuentra una tarea en waiting
la cambia a RUNNING, para que pudiera ser revisada cuando se ejecute el dispatcher. Algo importante de esta función es que se reduce el local tick de la función. 

Systick Handler es sencilla, lo que hace es aumentar el global tick en 1, hecho esto, ejecuta la función de Activate Waiting Tasks y llama al dispatcher, pero
ahora por medio de interrupción del tick.

PendSV lo que hace es almacenar el valor del sp del procesador (que esta en r0) en una variable, entonces el stack pointer de la tarea se carga a esta variable,
de tal forma que el stack pointer del procesador es reemplazado por el de la tarea
