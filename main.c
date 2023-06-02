#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <getopt.h>

void *client(void *ptr);
void *barber();
int rand_time(int min,int max);

volatile int l_czek = 0;
volatile long rezygnanci = 0;
pthread_mutex_t Czek_mutex;
sem_t klient, fryzjer;
volatile long clientcount = 10;
volatile long min_sleep_time = 5;
volatile long max_sleep_time = 10;

int main(int argc, char *argv[])
{
    openlog("loglog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    int c;
    while ((c = getopt(argc,argv,"c:m:x:")) != -1){
        switch (c)
        {
        case 'c': //client amount argument
            clientcount = atoi(optarg);
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

    sem_init(&klient,0,0);
    sem_init(&fryzjer,0,0);

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
    while (1)
    {
        pthread_mutex_lock(&Czek_mutex);
        if (l_czek < 10)
        {
            l_czek = l_czek + 1;
            sem_post(&klient);
            pthread_mutex_unlock(&Czek_mutex);
            sem_wait(&fryzjer);
            syslog(LOG_INFO, "Client %ld is worked on by the barber", nr);
            printf("rezygnacja:%ld Poczekalnia:%d/%d [Fotel:%ld]\n",rezygnanci,l_czek,10,nr);
        }
        else
        {
            rezygnanci += 1;
            pthread_mutex_unlock(&Czek_mutex);
        }
        sleep(rand_time(min_sleep_time,max_sleep_time));
    }
}

void *barber()
{
    while (1)
    {
        sem_wait(&klient);

        pthread_mutex_lock(&Czek_mutex);
            l_czek = l_czek - 1;
            syslog(LOG_INFO, "Barber started working");
            sem_post(&fryzjer);
        pthread_mutex_unlock(&Czek_mutex);

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