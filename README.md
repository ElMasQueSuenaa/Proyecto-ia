# Pintura Generativa

Para compilar este archivo se necesita:
    - Debes tener instalado un compilador que soporte C++17 o superior.
    - Tener instalado GNU Make.

Para elegir la imagen objetivo en el archivo testCall.cpp en la linea 240 se debe cambiar el argumento entre comillas y escribir el nombre de la imagen.

Adentro de la carpeta "Proyecto-IA" se debe abrir una terminal de linux o una terminal y escribir el comando:
    - "make"
Para ejecutar el programa:
    - "make run"
Para limpiar los archivos que se crearon:
    - "make clean"

Una vez terminada la ejecución se podrá ver la imagen en el archivo "output.png"
Además se creará un archivo con nombre "default_fecha" que guarda los parámetros utilizados y como evoluciona el MSE en cada iteración y además el tiempo que se demora cada iteración.

El archivo "plot_mse_time.py" en la linea 69 se coloca la dirección del archivo que se desea crear un gráfico MSE vs tiempo, la ejecución de este programa generará el gráfico de titulo
igual al archivo seleccionado.

