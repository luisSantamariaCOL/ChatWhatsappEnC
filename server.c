// C Program for Message Queue (Writer Process) 
#include <stdio.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <pthread.h> 
#include <string.h>
#include <stdlib.h> //para el atoi

#define SIZE 20
#define MESSAGE_LENGTH 280

// structure for message queue 
struct _mbuffer { 
	long mtype; 
	char mtext[MESSAGE_LENGTH]; 
} message[2]; 

int msgid; 
int count_users = 0;
char *bufferSalida = "exit";

long arrayPid[SIZE];


void* th_sendMessage (void* unused);
void* th_receiveMessage(void* unused);
void* th_receiveSincronization(void* unused);
void  cerrarServicio(void);
void broadcast(char *message);


int main(int argc, char const *argv[])
{
    
    key_t key;

    key = ftok("/tmp/msgid1.txt", 999); //key id 1
    
    
    msgid = msgget(key, 0666 | IPC_CREAT); //queue creation
    

    pthread_t id_sincronization;
     //pthread_t id_th_1;
     //pthread_t id_th_2;
    
    if( pthread_create (&id_sincronization,NULL,&th_receiveSincronization,NULL) == -1) perror ("thread1 creation fails: ");
    
    // if( pthread_create (&id_th_1,NULL,&th_sendMessage,NULL) == -1) perror ("thread1 creation fails: ");
    // if( pthread_create (&id_th_2,NULL,&th_receiveMessage,NULL) == -1) perror ("thread2 creation fails: ");
   
    if( pthread_join (id_sincronization,NULL) == -1) perror ("thread1 join fails: ");
    // if( pthread_join (id_th_1,NULL) == -1) perror ("thread1 join fails: ");
    // if( pthread_join (id_th_2,NULL) == -1) perror ("thread2 join fails: ");

    char bufferComando[4];
    while (1)
    {
        
        printf("escriba exit para salir\n");
        scanf("%s",bufferComando);
        if (strncmp(bufferComando,bufferSalida,4) == 0)
        {

            cerrarServicio();
            break;
            
        }
        
    }

    return 0;
}

void* th_sendMessage (void* unused){
    int length;
    

    message[0].mtype = message[1].mtype;

    while (1)
    {
        if( fgets(message[0].mtext, sizeof(message[0].mtext), stdin ) != NULL)
        {
            length = strlen(message[0].mtext);
            if (message[0].mtext[length-1] == '\n')
                message[0].mtext[length-1] = '\0';
            msgsnd(msgid, &message[0], length+1, 0);
        }
    }
    

    return NULL;
}

void* th_receiveMessage(void* unused){

    while (1)
    {
    msgrcv(msgid, &message[1], sizeof(message[1].mtext), 10, 0); 
  
    printf("\nClient: %s",message[1].mtext);
    }
    
    return NULL;
}

void* th_receiveSincronization(void* unused){
    int length;
    while (1)
    {

        while (1) //garantizo que el servidor leyo un mensaje
        {
            if( msgrcv(msgid, &message[1], sizeof(message[1].mtext), 10, 0) != -1){
                break;
            }
        }
        printf("Client: %s\n",message[1].mtext);

        if (count_users<SIZE)
        {
            count_users++;
            sprintf(message[0].mtext,"%s","exitoso");
            arrayPid[count_users-1] =  (long) atoi(message[1].mtext);
            char aux[MESSAGE_LENGTH];
            sprintf(aux,"Server:\nNuevo cliente conectado %ld", arrayPid[count_users-1]);
            broadcast(aux);
        }
        else{
            sprintf(message[0].mtext,"%s","limite de usuarios excedido");
        }
        

        message[0].mtype = (long) atoi(message[1].mtext);
        //printf("Variable %d\n", (int)message[0].mtype);
        length = strlen(message[0].mtext);
        if( msgsnd(msgid, &message[0], length+1, 0) == -1 ) perror("msgsnd fails: ");
        
    }
        //solo compilo este hilo
    //recibe el mensaje
    //verifica si lo puede dejar conectar
    //guardo el pid
    //Enviarle un mensaje a todos los procesos diciendo que alguien se conecto
    //responderle exito o fracaso
    return NULL;
}

void cerrarServicio(){

        if(msgctl(msgid, IPC_RMID, NULL) == -1) perror ("msfctl");
        //y demas servicios

        return;
}

void broadcast(char *message){
    int length;
    struct _mbuffer buffer;
    sprintf(buffer.mtext,"%s",message);
    for (int i = 0; i < count_users; i++)
    {
        buffer.mtype = arrayPid[i];
        length = strlen(buffer.mtext);
        if( msgsnd(msgid, &buffer, length+1, 0) == -1 ) perror("msgsnd fails: ");
    }

    return;
}