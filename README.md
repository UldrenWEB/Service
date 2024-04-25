## Explicacion 

Este algoritmo algo complejo de entender, se encarga de poder instalar un servicio, ejecutarlo si esta instalado, asi mismo buscar un archivo por nombre especifico o por un patron de bytes especificos y enviar via socket dichas archivos encontrados o mensajes relevantes del computador que lo ejecute, esto a un servidor especificado.

La idea de este algoritmo es poder demostrar lo facil y rapido que se puede crear un virus, que logrando que se ejecute en el computador victima como administrador puede causar fallas de filtraciones de algun tipo de archivo importante de dicha maquina.

A su vez este encripta lo archivos encontrados haciendo uso de la libreria **OpenSSL** que se pueda implementar de manera estatica para que pueda ser ejecutado en cualquier computador sin la necesidad de tenerlo instalado en su computador

## Uso

Para poder compilar este archivo y convirtiendo las librerias dinamicas como **OpenSSL** y haciendo que sean estaticas para que este ejecutable pueda estar disponible y en correcto funcionamiento sin la necesidad de algun paquete externo en cualquier computador, deben tener los siguientes paquetes.

### Necesario

- Vcpkg (_Para instalar las librerias estaticas_)
    - Clonar **vcpkg** de su repositorio, [Repositorio vcpkg](https://github.com/Microsoft/vcpkg.git)
    - Entrar en el directorio clonado y ejecutar el `boostrap\vcpkg.bat`
    - Luego instalar la version estatica de OpenSSL, asi:
        - `vcpkg install openssl:x64-windows-static`

- Compilador VS (_Para compilar el archivo .cpp_)
    - Es necesario tener instalado el **Compilador de Visual Studio** para poder compilar y recibir el ejecutable de manera estatica

## Comando para la compilacion

`cl /EHsc /I "C:/turuta/vcpkg/installed/x64-windows-static/include" fileName.cpp /link /LIBPATH:"C:/turuta/vcpkg/installed/x64-windows-static/lib" /out:outputFile.exe libssl.lib libcrypto.lib gdi32.lib crypt32.lib ws2_32.lib Advapi32.lib User32.lib Winspool.lib /MACHINE:X64`

### Explicacion del comando

En este comando se indica lo siguiente: 
- Se indica con el argumento **_/I_** la ruta donde estaran los headers de la libreria
- Luego se indica el archivo con el codigo fuente
- A su vez, esta el argumento **_/link_** para asi indicar luego de este argumento que iran los distintos argumentos de compilacion
- El argumento **_/LIBPATH:_**, aqui se indica el directorio donde estara el codigo fuente o los archivos _.lib_ _.a_ de la libreria 
- El argumento **_out:_** se usa para indicar cual sera y donde se posicionara el ejecutable que se cree de salida
- El argumento **_/MACHINE:X64_**, indica que la arquitectura con la que se va a compilar es de **64bits**
- Luego de eso, se indican los archivos necesarios de la libreria y otros necesarios para que funcione el ejecutable en window
    - Las librerias estaticas que vienen por defecto en window son: 
        - gdi32.lib 
        - crypt32.lib
        - Advapi32.lib 
        - User32.lib
        - Winspool.lib
    - Las librerias necesarias para OpenSSL: 
        - libssl.lib 
        - libcrypto.lib
    - Libreria para utilizar **winsock2** (Manejo de sockets)
