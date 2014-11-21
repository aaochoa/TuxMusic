El proyecto es una implementación de un cliente de música online (Cliente servidor) en lenguaje C/C++ y hace uso de las siguientes librerias:
->ZMQ 4.1.0
->CZMQ 2.2.2
->SFML 2.1

Para ejecutar el Servidor:  
./Server [Ip del servidor] [Ip del broker] [puerto del broker] [puerto del cliente]

[puerto del broker] = Puerto por el cual el Servidor se comunica con el Broker 
[puerto del cliente] = Puerto por el cual el servidor escucha a los clientes

Para ejecutar el Broker:
./Broker [puerto del servidor] [puerto del cliente] 

[puerto del servidor] = Tiene que coincidir con [puerto del broker] utilizado en el Servidor
[puerto del cliente]  = Tiene que coincidir con [puerto del broker] utilizado en el Cliente

Para ejecutar el Cliente
./Client [Ip del broker] [Puerto del broker] Temp/Temp_data.ogg

==================================================================

Este es el menú que mostrará el cliente cuando sea ejecutado:
::::::::::::::::::::::::::::::::::::::::
::::::::::::::::::::::::::::::::::::::::
1) B  Parámetro --> Busca según el parámetro
2) R Nombre de la cancion --> Reproduce 
3) A Nombre de la cancion ---> Añade a Playlist
4) V Playlist  ---> Muestra la Playlist
5) S salir  ----> Salir
6) Play song   ----> Play
7) Pause song   ----> Pause
8) Stop song   ----> Stop
9) Next song   ----> Next
10) Prev song   ----> Prev
::::::::::::::::::::::::::::::::::::::::
::::::::::::::::::::::::::::::::::::::::
