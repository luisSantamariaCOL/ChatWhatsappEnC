// C Program for Message Queue (Writer Process) 
#include <stdio.h> 
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <pthread.h>
#include <string.h>

// structure for message queue 
struct _mbuffer { 
	long mtype; 
	char mtext[100]; 
} message[2]; 

void* th_sendMessage (void* unused);
void* th_receiveMessage(void* unused);

int msgid; //for queue

pid_t pidCliente; //pid CLiente
pthread_t id_th_2;

int main(int argc, char const *argv[])
{
    
    key_t key;

    key = ftok("/tmp/msgid1.txt", 999);
    
    
    msgid = msgget(key, 0666 | IPC_CREAT); //queue creation
    //printf("msgid = %d",msgid);


    
    int  comando;
    while(comando!=0){
    printf("presiona 1 para enviarle una solicitud de conexión al servidor\n");
    scanf("%d",&comando);
        if(comando==1){

           // pthread_t id_th_1;
           

            int length; 
            pidCliente = getpid(); //Guardo el pid de este proceso
            message[0].mtype = 10; //Identificador de la cola del servidor
            sprintf(message[0].mtext, "%d", pidCliente); //Lo convierto 
            length = strlen(message[0].mtext);


            if (message[0].mtext[length-1] == '\n')
                message[0].mtext[length-1] = '\0';

            if( msgsnd(msgid, &message[0], length+1, 0) == -1 ) perror("msgsnd fails: ");
            
            while (1)
            {
                if ( msgrcv(msgid, &message[1], sizeof(message[1].mtext), pidCliente, 0) != -1)
                {
                    //printf("se recibio\n");
                    //printf("\nServer: %s\n",message[1].mtext);
                    if(strncmp(message[1].mtext,"exitoso",7) == 0){
                        // if( pthread_create (&id_th_1,NULL,&th_sendMessage,NULL) == -1) perror ("thread1 creation fails: ");
                        if( pthread_create (&id_th_2,NULL,&th_receiveMessage,NULL) == -1) perror ("thread2 creation fails: ");
                        
                        // if( pthread_join (id_th_1,NULL) == -1) perror ("thread1 join fails: ");
                        // if( pthread_join (id_th_2,NULL) == -1) perror ("thread2 join fails: ");
                        pthread_join (id_th_2,NULL);
                        printf("no se esta esperando el hilo\n");
                        break;
                    }
                    if(strncmp(message[1].mtext,"limite de usuarios excedido",8) == 0){
                        printf("no se pudo conectar");
                        break;
                    }
            
                }
            }
        

        }
    }
     
    return 0;
}

void* th_sendMessage (void* unused){
    int length;
    pidCliente = getpid();

    message[0].mtype = (long) pidCliente;

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
        msgrcv(msgid, &message[1], sizeof(message[1].mtext), pidCliente, 0); 
    
        printf("\n%s\n",message[1].mtext);
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