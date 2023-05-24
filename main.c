#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

void *client(void *ptr);
void *barber();

volatile int l_czek = 0;
pthread_mutex_t Czek_mutex;
sem_t klient, fryzjer;
unsigned int usecs = 1000000;

int main(int argc, char *argv[])
{
    openlog("loglog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    int clientcount = 10;
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
        }
        else
        {
            pthread_mutex_unlock(&Czek_mutex);
        }
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
        if(usleep(usecs)==-1){
            syslog(LOG_ERR, "Usleep died");
        };
        syslog(LOG_INFO, "Barber stopped working");
    }
}