// C Program for Message Queue (Writer Process) 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MESSAGE_LENGTH 280

// structure for message queue 
struct _mbuffer { 
	long mtype; 
	char mtext[MESSAGE_LENGTH]; 
} message[2]; 
//message[0] -> Enviar
//message[1] -> Recibir

void* th_sendMessage (void* unused);
void* th_receiveMessage(void* unused);


int msgid; //for queue
int msgid_desconectar;
int msgid_comunicacion;
int activo = 1;

pid_t pidCliente; //pid CLiente
pthread_t id_th_1;
pthread_t id_th_2;


int main(int argc, char const *argv[])
{
    
       

    //CREACIÓN DE COLAS
    key_t key;
    key_t key_desconectar;
    key_t key_comunicacion;

    key = ftok("/tmp/msgid1.txt", 999); //key id
    msgid = msgget(key, 0666 | IPC_CREAT); //queue creation
    
    key_desconectar = ftok("/tmp/msgid2.txt", 998); //key id
    msgid_desconectar = msgget(key_desconectar, 0666 | IPC_CREAT);

    key_comunicacion = ftok("/tmp/msgid3.txt", 997); //key id
    msgid_comunicacion = msgget(key_comunicacion, 0666 | IPC_CREAT);

    pidCliente = getpid(); //Guardo el pid de este proceso

    char valorIngresado[MESSAGE_LENGTH];
    int  conectar = 1;
    int length_VI;

    while(1){
        printf("Ingrese yes si desea conectarse al grupo\n");
        fgets(valorIngresado,MESSAGE_LENGTH,stdin);
        length_VI = strlen(valorIngresado);
    
        if (valorIngresado[length_VI-1] == '\n')
                valorIngresado[length_VI-1] = '\0';
        conectar = strcmp(valorIngresado, "yes");
        if (conectar == 0)
            break;
    }

            int length_m; 
            
            message[0].mtype = 11; //Identificador de la cola del servidor
            memset(message[0].mtext,0,MESSAGE_LENGTH); //limpio aux
            sprintf(message[0].mtext, "%d", pidCliente); //Lo convierto y paso a message[0].mtext. 

            length_m = strlen(message[0].mtext);
            if (message[0].mtext[length_m-1] == '\n')
                message[0].mtext[length_m-1] = '\0';

            //Enviaré el PID del cliente al server. El tipo de mensaje es 10
            
            if( msgsnd(msgid, &message[0], length_m+1, 0) == -1 ) perror("msgsnd fails: "); 
            
            while (1)
            {

                if ( msgrcv(msgid, &message[1], sizeof(message[1].mtext), pidCliente, 0) != -1){
                    break;
                } 
  
            }

            printf("Servidor: %s, se puede conectar.\n",message[1].mtext);
            if(strcmp(message[1].mtext,"exitoso") == 0){
                pthread_create (&id_th_1,NULL,&th_sendMessage,NULL);
                pthread_create (&id_th_2,NULL,&th_receiveMessage,NULL);

                pthread_join (id_th_1,NULL);
                pthread_join (id_th_2,NULL);
    
            } 
            else if(strcmp(message[1].mtext,"limite de usuarios excedido") == 0){
                printf("no se pudo conectar");
                exit(EXIT_FAILURE);
            }
        
    
     
    return 0;
}

void* th_sendMessage (void* unused){
    int length_SM;
    pidCliente = getpid();
    message[0].mtype = 10;
    
    
    while (activo == 1)
    {
        length_SM = strlen(message[0].mtext);
        memset(message[0].mtext,0,length_SM);

        char identificadorUsuario[MESSAGE_LENGTH];
        sprintf(identificadorUsuario,"Cliente %d: ", pidCliente);

        while (activo == 1)
        {
            if( fgets(message[0].mtext, sizeof(message[0].mtext), stdin ) != NULL){
                length_SM = strlen(message[0].mtext);
                if (message[0].mtext[length_SM-1] == '\n')
                    message[0].mtext[length_SM-1] = '\0';
                break;
                }
        }

        if( strncmp(message[0].mtext,"salir",5) == 0){
            message[0].mtype = 99;
            sprintf(message[0].mtext, "%d", pidCliente); //Lo convierto y paso a message[0].mtext. 
            length_SM = strlen(message[0].mtext);
            msgsnd(msgid_desconectar, &message[0], length_SM+1, 0);
                while (activo == 1)
                {
                //pidCliente es el tipo de mensaje que me mandan
                    if ( msgrcv(msgid, &message[1], sizeof(message[1].mtext), pidCliente, 0) != -1)
                    {
                        printf("\"%s\"\n",message[1].mtext);
                        activo = 0;
                        return NULL;
                    }
                }
        }

        //Si el usuario escribe "put "" 
        if ( strncmp(message[0].mtext,"put ", 4) == 0)
        {
            length_SM = strlen(message[0].mtext);
            char archivo[15];
            for(int i = 0; i < (length_SM-3); i++)
            archivo[i] = message[0].mtext[i+4];
            //printf("%d %d\n", (int) strlen(message[0].mtext), (int) strlen(archivo));
            FILE *Fin = fopen(archivo, "r");
            if (Fin == NULL) {
                perror("error Leer archivo: ");
            }
            char linea[50];
            sprintf(message[0].mtext,"%s envia ARCHIVO --> %s,\n Contenido: \n",identificadorUsuario,archivo);
            while(fgets(linea,sizeof(linea),Fin)!=NULL){
                strcat(message[0].mtext,linea);
            }
            fclose(Fin);
            strcat(message[0].mtext,"\n Fin del archivo\n");
            length_SM = strlen(message[0].mtext);
            //printf("%s \n",buffer[0].mtext);
            msgsnd(msgid_comunicacion, &message[0], length_SM+1, 0);
        }

        else if( length_SM > 1)
        {
 
            length_SM = strlen(message[0].mtext);
            strncat(identificadorUsuario, message[0].mtext,length_SM); //
            sprintf(message[0].mtext,"%s",identificadorUsuario);
            length_SM = strlen(message[0].mtext);
            msgsnd(msgid_comunicacion, &message[0], length_SM+1, 0);

            //memset(message[0].mtext,0,length_SM);  
        }
    }
    
    
    return NULL;
}

void* th_receiveMessage(void* unused){

    while (activo == 1)
    {
        //memset(message[1].mtext,0,MESSAGE_LENGTH); //limpio aux
        while (1)
        {
            if ( msgrcv(msgid_comunicacion, &message[1], sizeof(message[1].mtext), pidCliente, 0)  != -1) break;  
        }
        if (strncmp(message[1].mtext,"desconectarse",13) == 0 )
        {
            printf("Servidor: El chat ha terminado. No vuelva.\n");
            activo = 0;
            return NULL;
        }
        
        printf("%s\n",message[1].mtext);
    }
    
    return NULL;
}

/*
el hilo de eliminar usuario recibe un mensaje por esa cola, si recibe mensaje, elimina el pid que recibo.
¿como elimino el pid?
encontrar el pid y eliminarlo. 
Y a los otros pid reordenarlos.
Crear una cola, recibir un mensaje igual al de arriba (while hasta que lo recibe).
Luego reorganizo un usuario y pongo un mensaje que diga que se desconectó.
y devolverle un mensaje al cliente.
*/

/*
 FUNCIÓN ARCHIVOS:
 1. en la funcion de if que compara con salir. Pongo otro if y hago un strcmp.  //"put nombreArchivo"
 2. Ese archivo lo leo linea a linea y se lo concateno a buffer. (se puede hacer con una función)


REVISAR DEL TODO.
1. Conectar más de 3 clientes.
2. Enviar mensajes
3. Desconectar un cliente
4. Conectar otro cliente
5. Enviar más mensajes
6. Enviar un archivo
7. desconectar un cliente
8. Enviar más mensajes
9. Cerrar el servidor
 
*/

