// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/* SHM-Demoapplication to output values from shared memory to console output
 * 
 * Usage:
 * -o           Reads main output variables from control
 * -i           Reads main input variables to control
 * -a           Reads additional output variables from control
 * -t [value]   Specifies update-period in milliseconds. Default 10 seconds
 * -h           Prints this help message and exits
 * 
 */

#include "../mk_shminterface.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

uint8_t run = 1;
struct demoreader_t {
        struct mk_mainoutput * mainout;
        struct mk_maininput * mainin;
        struct mk_additionaloutput * addout;
        sem_t* sem_mainout;
        sem_t* sem_mainin;
        sem_t* sem_addout;
        uint32_t period;
        bool flagmainout;
        bool flagmainin;
        bool flagaddout;
};

/* Opens a shared memory (Readonly) and created it if necessary */
void* openShM(const char* name, uint32_t size, sem_t** sem)
{
        int fd;
        bool init = false;
        int mpflg = PROT_READ;
        void* shm;
        int semflg = 0;
        fd = shm_open(name, O_RDONLY, 0666);
        if ((fd == -1) && (errno == ENOENT)){
                //shm not available yet -> create and initialize
                init = true;
                fd = shm_open(name,O_RDWR | O_CREAT,0666);
                mpflg = PROT_READ | PROT_WRITE;
                semflg = O_CREAT;
        }
        if (fd == -1) {
                perror("SHM Open failed");
                return(NULL);
        }
        
        ftruncate(fd,size);
        shm = mmap(NULL, size, mpflg, MAP_SHARED | MAP_POPULATE, fd, 0);
        if (MAP_FAILED == shm) {
                perror("SHM Map failed");
                shm = NULL;
                if(init)
                        shm_unlink(name);
        }
        *sem = sem_open(name,semflg,0666,0);
        if (*sem == SEM_FAILED){
                perror("Semaphore open failed");
                munmap(shm, size);
                return(NULL);
        }
        if(init) {
                memset(shm,0,size);
                mprotect(shm,size,PROT_READ);
                sem_post(*sem);
        }
        
        return shm;
}

/* Closes a shared memory */
int closeShM(void** shm, int len, sem_t** sem)
{
        // not doing unlink of the shared memory because this should only be
        // done by the writer, so other reader process can still open it
        int ok;        
        ok = munmap(*shm,len);
        if (ok < 0)
                return ok;
        *shm = NULL;
        ok = sem_close(*sem);
        if (ok < 0)
                return ok;
        *sem = NULL;
        return ok;
}

/* signal handler */
void sigfunc(int sig)
{
        switch(sig)
        {
        case SIGINT:
                if(run)
                        run = 0;
                else
                        exit(0);
                break;
        case SIGTERM:
                run = 0;
                break;        
        }
}

/* Print usage message */
static void usage(char *appname)
{
        fprintf(stderr,
                "\n"
                "Usage: %s [options]\n"
                " -o            Reads main output variables from control\n"
                " -i            Reads main input variables to control\n"
                " -a            Reads additional output variables from control\n"
                " -t [value]    Specifies update-period in milliseconds. Default 10 seconds.\n"
                " -h            Prints this help message and exits\n"
                "\n",
                appname);
}

/* Evaluate CLI-parameters */
void evalCLI(int argc, char* argv[0],struct demoreader_t * reader)
{
        int c;
        char* appname = strrchr(argv[0], '/');
        appname = appname ? 1 + appname : argv[0];
        while (EOF != (c = getopt(argc,argv,"oiaht:"))) {
                switch(c) {
                case 'o':
                        (*reader).flagmainout = true;
                        break;
                case 'i':
                        (*reader).flagmainin = true;
                        break;
                case 'a':
                        (*reader).flagaddout = true;
                        break;
                case 't':
                        (*reader).period = atoi(optarg)*1000;
                        break;
                case 'h':
                default:
                        usage(appname);
                        exit(0);
                        break;
                }
        }
        if (((*reader).flagmainout == false) && ((*reader).flagmainin == false) && ((*reader).flagaddout) == false) {
                printf("At minium, one block of variables needs to be selected\n");
                exit(0);
        };

}

int main(int argc, char* argv[])
{
        struct demoreader_t reader;
        reader.flagaddout = false;
        reader.flagmainout = false;
        reader.flagmainin = false;
        reader.period = 10000000;       // 10 seconds
        time_t now;
        struct tm now_local;

        evalCLI(argc,argv,&reader);

        //register signal handlers
        signal(SIGTERM, sigfunc);
        signal(SIGINT, sigfunc);

        // open and setup shm mapping
        if (reader.flagmainout) {
                reader.mainout = (struct mk_mainoutput *) openShM(MK_MAINOUTKEY,sizeof(struct mk_mainoutput),&reader.sem_mainout);
                if (NULL == reader.mainout)
                        reader.flagmainout = false;
        }
        if (reader.flagmainin) {
                reader.mainin = (struct mk_maininput *) openShM(MK_MAININKEY,sizeof(struct mk_maininput),&reader.sem_mainin);
                if (NULL == reader.mainin)
                        reader.flagmainin = false;
        }
        if (reader.flagaddout) {
                reader.addout = (struct mk_additionaloutput *) openShM(MK_ADDAOUTKEY,sizeof(struct mk_additionaloutput),&reader.sem_addout);
                if (NULL == reader.addout)
                        reader.flagaddout = false;
        }

        // mainloop
        while(run) {
                now = time(NULL);
                now_local = *localtime(&now);
                if (reader.flagmainout){
                        sem_wait(reader.sem_mainout);
                        sem_post(reader.sem_mainout);
                        printf("\n##### Main Output Variables: (at %02d:%02d:%02d) #####\n", now_local.tm_hour, now_local.tm_min, now_local.tm_sec);
                        printf("X-Velocity Setpoint: %f mm/s;        Y-Velocity Setpoint: %f mm/s;        Z-Velocity Setpoint: %f mm/s;        Spindlespeed Setpoint: %f rpm\n",reader.mainout->xvel_set,reader.mainout->yvel_set,reader.mainout->zvel_set,reader.mainout->spindlespeed);
                        printf("X-Axis enabled: %s;             Y-Axis enabled: %s;             Z-Axis enabled: %s;             Spindle enabled: %s\n",reader.mainout->xenable ? "true" : "false",reader.mainout->yenable ? "true" : "false",reader.mainout->zenable ? "true" : "false",reader.mainout->spindleenable ? "true" : "false");
                        printf("Spindlebranke engaged: %s;      Machine on: %s;                 Emergency Stop activated: %s\n",reader.mainout->spindlebrake ? "true" : "false",reader.mainout->machinestatus ? "true" : "false",reader.mainout->estopstatus ? "true" : "false");
                }
                if (reader.flagaddout){
                        sem_wait(reader.sem_addout);
                        sem_post(reader.sem_addout);
                        printf("\n##### Additional Output Variables: (at %02d:%02d:%02d) #####\n", now_local.tm_hour, now_local.tm_min, now_local.tm_sec);
                        printf("X-Position Setpoint: %f mm;         Y-Position Setpoint: %f mm;        Z-Position Setpoint: %f mm;        Feedrate planned: %f mm/s\n",reader.addout->xpos_set,reader.addout->ypos_set,reader.addout->zpos_set,reader.addout->feedrate);
                        printf("X-Axis at home: %s;              Y-Axis at home: %s;             Z-Axis at home: %s;             Feedrate override: %f %%\n",reader.addout->xhome ? "true" : "false",reader.addout->yhome ? "true" : "false",reader.addout->zhome ? "true" : "false",reader.addout->feedoverride);
                        printf("X-Axis at neg Endstop: %s;       Y-Axis at neg Endstop: %s;      Z-Axis at neg Endstop: %s\n",reader.addout->xhardneg ? "true" : "false",reader.addout->yhardneg ? "true" : "false",reader.addout->zhardneg ? "true" : "false");
                        printf("X-Axis at pos Endstop: %s;       Y-Axis at pos Endstop: %s;      Z-Axis at pos Endstop: %s\n",reader.addout->xhardpos ? "true" : "false",reader.addout->yhardpos ? "true" : "false",reader.addout->zhardpos ? "true" : "false");
                        printf("Current Line Number: %d;                                         Tool Number: %d;                Mode: %d\n",reader.addout->lineno,reader.addout->tool,reader.addout->mode);
                }
                if (reader.flagmainin){
                        sem_wait(reader.sem_mainin);
                        sem_post(reader.sem_mainin);
                        printf("\n##### Main Input Variables: (at %02d:%02d:%02d) #####\n", now_local.tm_hour, now_local.tm_min, now_local.tm_sec);
                        printf("X-Position Current: %f mm;         Y-Position Current: %f mm;        Z-Position Current: %f mm;\n",reader.mainin->xpos_cur,reader.mainin->ypos_cur,reader.mainin->zpos_cur);
                        printf("X-Axis faulty: %s;              Y-Axis faulty: %s;             Z-Axis faulty: %s;\n",reader.mainin->xfault ? "true" : "false",reader.mainin->yfault ? "true" : "false",reader.mainin->zfault ? "true" : "false");
                }
                usleep(reader.period);
        }

        // cleanup
        if (reader.flagmainout)
                closeShM((void**)&reader.mainout, sizeof(reader.mainout),&reader.sem_mainout);
        if (reader.flagmainin)
                closeShM((void**)&reader.mainin, sizeof(reader.mainin),&reader.sem_mainin);
        if (reader.flagaddout)
                closeShM((void**)&reader.addout, sizeof(reader.addout),&reader.sem_addout);

        return 0;
}
