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

