#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>

typedef struct Thread_queue thread_queue;

struct Thread_queue {
    long value;
    thread_queue *next;
};



//wątek klienta
void *client(void *ptr);

//wątek fryzjera
void *barber();

//wylosuj czas pomiędzy dwoma wartościami
int rand_time(int min,int max);

//przydziel nowy numer klientowi
long grant_new_number();


//dołącz do kolejki
int queue_enqueue(thread_queue *queue, int id);

//usuń z początku kolejki
thread_queue *queue_dequeue(thread_queue *first);

//wypisz kolejkę
void print_queue(thread_queue *first);

//wypisz n wartości z kolejki
void print_firstx_queue(thread_queue *first,int x);

//zwróć długość kolejki
int queue_size(thread_queue *first);

//inicjalizuj kolejkę
thread_queue *queue_init();


volatile int l_czek = 0;
int info_flag = 0;
volatile long rezygnanci = 0;
pthread_mutex_t Czek_mutex,count_mutex,queue_mutex,resign_queue_mutex;
pthread_cond_t client_cond;
pthread_cond_t barber_cond;
sem_t klient, fryzjer; //lobby;
long clientcount = 10;
long clientnr = 11;
int waitingroom_size = 10;
long min_sleep_time = 5;
long max_sleep_time = 10;
 
thread_queue *client_queue, *resign_queue;

int main(int argc, char *argv[])
{
    openlog("loglog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    int c;
    while ((c = getopt(argc,argv,"ic:m:x:")) != -1){
        switch (c)
        {
        case 'i': //info flag switch to 1
            info_flag = 1;
            syslog(LOG_INFO,"info flag switched to 1");
            break;
        case 'c': //client amount argument
            clientcount = atoi(optarg);
            clientnr = clientcount + 1;
            syslog(LOG_INFO,"client count set to %ld",clientcount);
            break;
        case 'm': //mini time argument
            min_sleep_time = atoi(optarg);
            syslog(LOG_INFO,"minimum sleep time set to %ld seconds",min_sleep_time);
            break;
        case 'x': //maximum time argument
            max_sleep_time = atoi(optarg);
            syslog(LOG_INFO,"maximum sleep time set to %ld seconds",max_sleep_time);
            break;
        case '?': //Failopt
            if(optopt == 'c' || optopt == 'm' || optopt == 'x')
                syslog(LOG_INFO, "Option -%c requires argument. ",optopt);
            else if(isprint(optopt))
                syslog(LOG_INFO, "Unknown opt '-%c'.",optopt);
            else
                syslog(LOG_INFO, "unknown opt char '\\x%x'",optopt);
            exit(EXIT_FAILURE);
        default:
            exit(EXIT_FAILURE);
        }
    }


    pthread_t barbers;
    pthread_t clients[clientcount];
    int iret1;

    pthread_cond_init(&barber_cond,NULL);
    pthread_cond_init(&client_cond,NULL);

    sem_init(&klient,0,0);
    sem_init(&fryzjer,0,0);
    //sem_init(&lobby,0,10);

    client_queue = queue_init();
    resign_queue = queue_init();

    iret1=pthread_create(&barbers,NULL,barber,NULL);
    if(iret1){
            printf("client thread creation error");
            syslog(LOG_ERR,"ERR- Pthread_create() retcode: %d",iret1);
            exit(EXIT_FAILURE);
        }

    long i;
    for ( i = 0; i < clientcount; i++)
    {
        iret1=pthread_create(&clients[i],NULL,client,(void*)i);
        if(iret1){
            printf("client thread creation error");
            syslog(LOG_ERR,"ERR- Pthread_create() retcode: %d",iret1);
            exit(EXIT_FAILURE);
        }
    }
    while (1)
    {
        //run
    }
    exit(EXIT_SUCCESS);
}

void *client(void *ptr)
{
    long nr = (long)ptr;
    long temp;
    while (1)
    {
        pthread_mutex_lock(&Czek_mutex);
        if (l_czek < waitingroom_size)
        {
            pthread_mutex_lock(&queue_mutex);
            queue_enqueue(client_queue,nr);
            pthread_mutex_unlock(&queue_mutex);

            l_czek = l_czek+1;
            sem_post(&klient);

            pthread_mutex_unlock(&Czek_mutex);

            while (client_queue->value!=nr){}; //wait


            pthread_cond_wait(&barber_cond,&Czek_mutex);

            syslog(LOG_INFO, "Client %ld is worked on by the barber", nr);
            printf("rezygnacja:%ld Poczekalnia:%d/%d [Fotel:%ld]\n",rezygnanci,l_czek,waitingroom_size,nr);
            
            if (info_flag ==1)
            {
                if (queue_size(client_queue)>0)
                {
                    print_queue(client_queue);    
                }
                
                if (queue_size(resign_queue)>0)
                {
                    print_queue(resign_queue);   
                }
            }
        }
        else
        {
            rezygnanci += 1;
            pthread_mutex_unlock(&Czek_mutex);

            pthread_mutex_lock(&resign_queue_mutex);
            queue_enqueue(resign_queue,nr);
            if(queue_size(resign_queue)>9){
                resign_queue = queue_dequeue(resign_queue);
            }

            pthread_mutex_unlock(&resign_queue_mutex);
        }
        sleep(rand_time(min_sleep_time,max_sleep_time));
        
        //temp = nr;
        nr = grant_new_number();

        //printf("klient %ld zmienia numer na %ld\n",temp,nr);
    }
}

void *barber()
{
    while (1)
    {
        sem_wait(&klient);

            pthread_cond_broadcast(&barber_cond);
            l_czek = l_czek - 1;

            pthread_mutex_lock(&queue_mutex);
            client_queue = queue_dequeue(client_queue);
            pthread_mutex_unlock(&queue_mutex);

            syslog(LOG_INFO, "Barber started working");

        sleep(rand_time(min_sleep_time,max_sleep_time));
        syslog(LOG_INFO, "Barber stopped working");
    }
}

int rand_time(int min,int max){
    srand(time(NULL));
    int secs;
    if(max > min){
        secs = (rand()%max)+min;
    }else if(max == min){
        secs = max;
    }else{
        secs = (rand()%min)+max;
    }
    return secs;
}

long grant_new_number(){
    pthread_mutex_lock(&count_mutex);
    long temp = clientnr;
    clientnr +=1;
    pthread_mutex_unlock(&count_mutex);
    return temp;
}


int queue_enqueue(thread_queue *queue, int value){
    while(queue->next != NULL){
        queue = queue->next;
    }
    queue->next = malloc(sizeof(thread_queue));

    queue = queue->next;
    queue->next = NULL;
    queue->value = value;
    return value;
}

thread_queue *queue_dequeue(thread_queue *first){
    if(first->next==NULL){
        first->value =-1;
        return first;
    }
    thread_queue *second = first->next;
    //pthread_cond_destroy(first->data.cond);
    free(first);
    return second;
}

void print_queue(thread_queue *first){
    thread_queue *queue = first;
    printf("queue : [");
    while (queue != NULL)
    {
        if(queue->value != -1){

            if(queue->next!=NULL)
            {printf("%ld,",queue->value);}
            else
            {printf("%ld",queue->value);}   
        }        
        queue = queue->next;   
    }
    printf("]\n");
}

void print_firstx_queue(thread_queue *first,int x){
    thread_queue *queue = first;
    int temp = 0;
    printf("queue : [");
    while (queue != NULL || temp == x)
    {
        if(queue->next!=NULL)
        {printf("%ld,",queue->value);}
        else
        {printf("%ld",queue->value);}
        queue = queue->next;   
    }
    printf("]\n");
}

int queue_size(thread_queue *first){
    int size = 0;
    while(first->next != NULL){
        first = first->next;
        size++;
    }
    return size;
}

thread_queue *queue_init(){
    thread_queue *queue = malloc(sizeof(thread_queue));
    queue->next = NULL;
    queue->value = -1;
    return queue;
}
