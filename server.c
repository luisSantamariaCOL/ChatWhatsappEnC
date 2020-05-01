//Server.c
#include <stdio.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <pthread.h> 
#include <string.h>
#include <stdlib.h> //para el atoi
#include <unistd.h>
#include <ctype.h>

#define SIZE 20
#define MESSAGE_LENGTH 280

//Estructura del mensaje 
struct _mbuffer { 
	long mtype; 
	char mtext[MESSAGE_LENGTH]; 
} message[2]; 

int msgid; 
int msgid_desconectar;
int msgid_comunicacion;

int count_users = 0;
char *bufferSalida = "exit";


long arrayPid[SIZE];



void* th_receiveMessage(void* unused);
void* th_receiveSincronization(void* unused);
void* th_disconnectUser(void* unused);
void  cerrarServicios(void);
void broadcast(char *msg);



int main(int argc, char const *argv[])
{
    //CREACIÓN DE LAS COLAS
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
     
    

    //CREACIÓN DE LOS HILOS
    pthread_t id_sincronization;
    pthread_t id_disconnectUser;
    pthread_t id_receiveMessage;
    

    pthread_create (&id_sincronization,NULL,&th_receiveSincronization,NULL);
    pthread_create (&id_disconnectUser,NULL,&th_disconnectUser,NULL);
    pthread_create (&id_receiveMessage,NULL,&th_receiveMessage,NULL);
   

    char bufferComando[4];

    //POR SI QUIERO CERRAR EL GRUPO
    while (1)
    {
        
        printf("escriba exit si desea cerrar el servidor\n");
        scanf("%s",bufferComando);
        if (strncmp(bufferComando,bufferSalida,4) == 0)
        {
            broadcast("grupo cerrado");

            sleep(2);
            
            cerrarServicios();
            break;
            
        }
    }

    pthread_join (id_sincronization,NULL);
    pthread_join (id_disconnectUser,NULL);
    pthread_join (id_receiveMessage,NULL);

    
    cerrarServicios();

    exit(EXIT_SUCCESS);
}


void* th_receiveMessage(void* unused){
    char aux_receiveMessage[MESSAGE_LENGTH];
    struct _mbuffer buffer;
    while (1)
    {

        
        while (1)
        {   
            if( msgrcv(msgid_comunicacion, &buffer, sizeof(buffer.mtext), 10, 0) != -1){
                break;
            }

        }
        
        memset(aux_receiveMessage,0,MESSAGE_LENGTH);
        sprintf(aux_receiveMessage,"%s", buffer.mtext);
        printf("%s\n",aux_receiveMessage);
        broadcast(aux_receiveMessage);
        
    }
    
    return NULL;
}

void* th_receiveSincronization(void* unused){

    int length;
    while (1)
    {
        memset(message[1].mtext,0,MESSAGE_LENGTH); //limpio aux
        while (1) //garantizo que el servidor leyo un mensaje
        {
            
            if( msgrcv(msgid, &message[1], sizeof(message[1].mtext), 11, 0) != -1){
                break;
            }
           
        }
        printf("Nuevo cliente: %s\n",message[1].mtext);

        memset(message[0].mtext,0,MESSAGE_LENGTH); //limpio message
        if (count_users<SIZE)
        {
            count_users++;
            sprintf(message[0].mtext,"%s","exitoso");
            arrayPid[count_users-1] =  (long) atoi(message[1].mtext);

        }
        else{
            sprintf(message[0].mtext,"%s","limite de usuarios excedido");
        }
        

        message[0].mtype = (long) atoi(message[1].mtext);

        length = strlen(message[0].mtext);
        
        if( msgsnd(msgid, &message[0], length+1, 0) == -1 ) perror("msgsnd fails: ");
        if ((count_users-1) < SIZE)
        {
            char aux_receiveSincronization[MESSAGE_LENGTH];
            sprintf(aux_receiveSincronization,"Servidor: Nuevo cliente conectado %ld", arrayPid[count_users-1]);
            broadcast(aux_receiveSincronization);
        }
        
        
    }
    
    return NULL;
}

void* th_disconnectUser(void* unused){
    

    while (1) //garantizo que el servidor leyo un mensaje
        {

            memset(message[1].mtext,0,MESSAGE_LENGTH); //limpio aux
            while (1)
            {
                if( msgrcv(msgid_desconectar, &message[1], sizeof(message[1].mtext), 99, 0) != -1)
                    break;
            }
            
            char bufferUsuario[10];
            memset(bufferUsuario,0,10); //limpio buffer
            for (int i = 0; i < count_users; i++)
            {
                sprintf(bufferUsuario,"%ld",arrayPid[i]);
                strcat(bufferUsuario,"\0");
                if (strcmp(bufferUsuario, message[1].mtext) == 0)
                {
                    int length;
                //Mandar mensaje al usuario: "te has desconectado"

                    memset(message[0].mtext,0,MESSAGE_LENGTH); //limpio aux
                    message[0].mtype = (long) atoi(message[1].mtext);

                    sprintf(message[0].mtext,"%s","Servidor: Salida exitosa. No vuelva");
                    length = strlen(message[0].mtext);
                    if( msgsnd(msgid_desconectar, &message[0], length+1, 0) == -1 ) perror("msgsnd fails: ");

                //Reorganizar array de Pid's
                    for (int j = i; i < (count_users-1); i++)
                    {
                        arrayPid[j] = arrayPid[j+1];
                    }
                    count_users--;
                //Mandar mensaje a todos los usuarios
                    char aux_disconnectUser[MESSAGE_LENGTH];
                    sprintf(aux_disconnectUser,"Servidor: Cliente desconectado: %s", bufferUsuario);
                    broadcast(aux_disconnectUser);
                
                    break;
                }
            }   
        }

    return NULL;
}


void broadcast(char *msg){


    int length;



    sprintf(message[0].mtext,"%s",msg);
    for (int i = 0; i < count_users; i++)
    {   
        message[0].mtype = arrayPid[i];
        length = strlen(message[0].mtext);
        if( msgsnd(msgid_comunicacion, &message[0], length+1, 0) == -1 ) perror("msgsnd fails: ");
    }
    printf("Se mandó el mensaje: \"%s\"\n",message[0].mtext);
    return;
}

void cerrarServicios(void){

    if( (msgctl(msgid, IPC_RMID, NULL)) == -1){
            perror ("msgctl msgid :");
            exit(EXIT_FAILURE);
        } 
    if( (msgctl(msgid_desconectar, IPC_RMID, NULL)) == -1){
        perror ("msgctl msgid_desconectar:");
        exit(EXIT_FAILURE);
    } 
    if( (msgctl(msgid_comunicacion, IPC_RMID, NULL)) == -1){
        perror ("msgctl msgid_comunicacion :");
        exit(EXIT_FAILURE);
    }
    
    printf("Se cerraron exitosamente los servicios del servidor\n");
    exit(EXIT_SUCCESS);
}


