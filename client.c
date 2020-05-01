//Client.c
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

// Estructura para los mensajes de las queues
struct _mbuffer { 
	long mtype; 
	char mtext[MESSAGE_LENGTH]; 
} message[2]; 

//message[0] -> Para Enviar
//message[1] -> Para Recibir

void* th_sendMessage (void* unused);
void* th_receiveMessage(void* unused);
void cerrarServicios();

//Ids de las queues
int msgid; 
int msgid_desconectar;
int msgid_comunicacion;
int activo = 1;

//Ids de los hilos
pid_t pidCliente; 
pthread_t id_th_1;
pthread_t id_th_2;


int main(int argc, char const *argv[])
{
    
       

    //CREACIÓN DE LAS LLAVES
    key_t key;
    key_t key_desconectar;
    key_t key_comunicacion;

    system("touch /tmp/msgid1.txt");
    if ( (key = ftok("/tmp/msgid1.txt", 999)) == -1)
    {
        perror("ftok error: ");
        exit(EXIT_FAILURE);
    }

    system("touch /tmp/msgid2.txt");
    if ( (key_desconectar = ftok("/tmp/msgid2.txt", 998)) == -1)
    {
        perror("ftok_desconectar error: ");
        exit(EXIT_FAILURE);
    }

    system("touch /tmp/msgid3.txt");
    if ( (key_comunicacion = ftok("/tmp/msgid3.txt", 997)) == -1)
    {
        perror("ftok_comunicacion error: ");
        exit(EXIT_FAILURE);
    }

    //CREACIÓN DE LAS COLAS
    if( (msgid = msgget(key, 0666 | IPC_CREAT)) == -1){
        perror("msgid error: ");
        exit(EXIT_FAILURE);
    }
    if( (msgid_desconectar = msgget(key_desconectar, 0666 | IPC_CREAT)) == -1){
        perror("msgid_desconectar error: ");
        exit(EXIT_FAILURE);
    }
    if( (msgid_comunicacion = msgget(key_comunicacion, 0666 | IPC_CREAT)) == -1){
        perror("msgid_comunicacion error: ");
        exit(EXIT_FAILURE);
    }
     

    //IDENTIFICADOR DEL PROCESO
    pidCliente = getpid();

    char valorIngresado[MESSAGE_LENGTH];
    int  conectar = 1;
    int desconectar = 1;
    int length_VI;

        while(desconectar != 0){
            printf("¿Desea conectarse al grupo? (yes/no)\n");
            fgets(valorIngresado,MESSAGE_LENGTH,stdin);
            length_VI = strlen(valorIngresado);
        
            if (valorIngresado[length_VI-1] == '\n')
                    valorIngresado[length_VI-1] = '\0';
            conectar = strcmp(valorIngresado, "yes");
            desconectar = strcmp(valorIngresado, "no");
            if (conectar == 0){
                int length_m; 
                
                message[0].mtype = 11; //Identificador del mensaje para conectarse al servidor
                memset(message[0].mtext,0,MESSAGE_LENGTH); //Limpieza del mensaje
                sprintf(message[0].mtext, "%d", pidCliente); //Convierto el pidCliente de tipo int y lo paso a message[0].mtext. 

                //Eliminar línea ('\n')
                length_m = strlen(message[0].mtext);
                if (message[0].mtext[length_m-1] == '\n')
                    message[0].mtext[length_m-1] = '\0';



                //Enviaré como mensaje al server el Identificador del cliente             
                if( msgsnd(msgid, &message[0], length_m+1, 0) == -1 ) perror("msgsnd fails: "); 
                
                while (1)
                {
                    //Recibiré del Servidor la respuesta de confirmación
                    if ( msgrcv(msgid, &message[1], sizeof(message[1].mtext), pidCliente, 0) != -1){
                        break;
                    } 
    
                }

                

                
                 //Si el servidor acepta...
                if(strcmp(message[1].mtext,"exitoso") == 0){
                   
                    printf("Servidor: %s. Bienvenido al grupo!!!\n",message[1].mtext);
                    printf("NOTA: Puede escribir \"salir\" si desea salirse\n");

                    //Se crean los hilos
                    pthread_create (&id_th_1,NULL,&th_sendMessage,NULL);
                    pthread_create (&id_th_2,NULL,&th_receiveMessage,NULL);

                    pthread_join (id_th_1,NULL);
                    pthread_join (id_th_2,NULL);
        
                } 
                else if(strcmp(message[1].mtext,"limite de usuarios excedido") == 0){
                    printf("no se pudo conectar");
                    cerrarServicios();
                    exit(EXIT_FAILURE);
                }

               
            }
        }
     
    cerrarServicios();

    EXIT_SUCCESS;
}

void* th_sendMessage (void* unused){

    int length_SM;
    pidCliente = getpid();
    message[0].mtype = 10;

    
    while (activo == 1)
    {
        //longitud del mensaje
        length_SM = strlen(message[0].mtext);
        //limpieza del mensaje
        memset(message[0].mtext,0,length_SM);

        //buffer que almacena el identificador del cliente
        char identificadorUsuario[MESSAGE_LENGTH];
        sprintf(identificadorUsuario,"Cliente %d: ", pidCliente);

        //INGRESANDO MENSAJE POR CONSOLA
        while (activo == 1)
        {
            if( fgets(message[0].mtext, sizeof(message[0].mtext), stdin ) != NULL){
                length_SM = strlen(message[0].mtext);
                if (message[0].mtext[length_SM-1] == '\n')
                    message[0].mtext[length_SM-1] = '\0';
                break;
                }
        }

        //COMPARACION: SALIR DEL PROGRAMA
        if( strncmp(message[0].mtext,"salir",5) == 0){
            
            printf("¿En serio desea salirse? (yes/no): ");
            char deseaSalir[MESSAGE_LENGTH];
            int salir = 1;
            int nosalir = 1;
            int length_VI;

            
            fgets(deseaSalir,MESSAGE_LENGTH,stdin);
            length_VI = strlen(deseaSalir);
        
            if (deseaSalir[length_VI-1] == '\n')
                    deseaSalir[length_VI-1] = '\0';

            salir = strcmp(deseaSalir, "yes");
            nosalir= strcmp(deseaSalir, "no");

            if(nosalir == 0 || salir !=0) continue;
            else{
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
           
        }
        
        //COMPARACION: MANDAR ARCHIVO 
        if ( strncmp(message[0].mtext,"put ", 4) == 0)
        {
            length_SM = strlen(message[0].mtext);
            char archivo[15];
            for(int i = 0; i < (length_SM-3); i++)
            archivo[i] = message[0].mtext[i+4];
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
            msgsnd(msgid_comunicacion, &message[0], length_SM+1, 0);
        }

        //MANDAR MENSAJE
        else if( length_SM > 1)
        {
 
            length_SM = strlen(message[0].mtext);
            strncat(identificadorUsuario, message[0].mtext,length_SM); //
            sprintf(message[0].mtext,"%s",identificadorUsuario);
            length_SM = strlen(message[0].mtext);
            msgsnd(msgid_comunicacion, &message[0], length_SM+1, 0);

        }
    }
    
    
    return NULL;
}

void* th_receiveMessage(void* unused){ 

    while (activo == 1)
    {
        //Recibiendo el mensaje
        while (1)
        {
            if ( msgrcv(msgid_comunicacion, &message[1], sizeof(message[1].mtext), pidCliente, 0)  != -1) break;  
        }

        //Si el cliente recibe el mensaje "grupo cerrado", es porque el servidor cerró el chat.
        if (strncmp(message[1].mtext,"grupo cerrado",13) == 0 )
        {
            printf("Servidor: El grupo se ha cerrado. Por favor no vuelva.\n");
            cerrarServicios();
            exit(EXIT_SUCCESS);
        }
        
        printf("%s\n",message[1].mtext);
    }
    
    return NULL;
}


void cerrarServicios(){ //Cerrar todas las colas

    if( (msgctl(msgid, IPC_RMID, NULL)) == -1){
        perror("msgid error: ");
        exit(EXIT_FAILURE);
    } 
    if( (msgctl(msgid_desconectar, IPC_RMID, NULL)) == -1){
        perror("msgid_desconectar error: ");
        exit(EXIT_FAILURE);
    } 
    if( (msgctl(msgid_comunicacion, IPC_RMID, NULL)) == -1){
        perror("msgid_comunicacion error: ");
        exit(EXIT_FAILURE);
    } 
    printf("Servicios de cliente exitosamente cerrados\n");
    exit(EXIT_SUCCESS);
}
