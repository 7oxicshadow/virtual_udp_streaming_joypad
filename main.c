#include <linux/uinput.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/time.h>

#include "joydata.h"

#define PORT             1235 
#define MAX_STRUCT_BYTES 40

int sockfd;
char buffer[MAX_STRUCT_BYTES];
struct sockaddr_in servaddr, cliaddr;
struct timeval timeout={2,0}; //set timeout for 2 seconds

unsigned int runme = 1U;

virjoy_un vjdata;
virjoy_un vjdata_prev;

pthread_mutex_t lock;

/* Thread to read UDP events */
void *UDP_Read_Thread(void *vargp) 
{ 
    int len, n;

    len = sizeof(cliaddr);
   
    while(runme)
    {

        //echo "This is a test" > /dev/udp/127.0.0.1/1235 //use this to test from terminal

        n = recvfrom(sockfd, (char *)buffer, MAX_STRUCT_BYTES,  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                &len);
      
        if(n > -1)
        {
            /* lock the data so that the main thread will not read during write */
            pthread_mutex_lock(&lock); 

            memset(&vjdata, 0, sizeof(vjdata));
            memcpy(vjdata.raw, buffer, sizeof(vjdata));

            //printf("UDP data written\n");
         
            //int i;

            //for(i=0; i<sizeof(vjdata.raw); i++)
            //   printf("%d - \n", vjdata.raw[i]);

            /* unlock the data to allow the main thread to resume */
            pthread_mutex_unlock(&lock);
        }
        //else
           //printf("UDP Timeout\n");    
    } 
} 

/* Interrupt handler to close the program */
void intHandler(int dummy) {
    runme = 0U;
}

/* Send a UINPUT event */
static void emit(int fd, int type, int code, int val) {
    struct input_event ie;
 
    ie.type = type;
    ie.code = code;
    ie.value = val;
    //ie.time.tv_sec = 0;
    //ie.time.tv_usec = 0;
 
    write(fd, &ie, sizeof(ie));
}

/* Create the UDP read thread */
void thread_init(pthread_t *thread_id)
{
    pthread_create(thread_id, NULL, UDP_Read_Thread, NULL); 
}

/* Initialise UINPUT settings */
int uinput_init(int *fd)
{
    struct uinput_user_dev uidev;

    *fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    if (*fd < 0)
    {
        printf("failed to initialise!\n");
        return -1;
    }

    ioctl(*fd, UI_SET_EVBIT, EV_KEY);
    ioctl(*fd, UI_SET_KEYBIT, BTN_A);
    ioctl(*fd, UI_SET_KEYBIT, BTN_B);
    ioctl(*fd, UI_SET_KEYBIT, BTN_X);
    ioctl(*fd, UI_SET_KEYBIT, BTN_Y);
    ioctl(*fd, UI_SET_KEYBIT, BTN_TL);
    ioctl(*fd, UI_SET_KEYBIT, BTN_TR);
    ioctl(*fd, UI_SET_KEYBIT, BTN_THUMBL);
    ioctl(*fd, UI_SET_KEYBIT, BTN_THUMBR);
    ioctl(*fd, UI_SET_KEYBIT, BTN_DPAD_UP);
    ioctl(*fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);
    ioctl(*fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
    ioctl(*fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
    ioctl(*fd, UI_SET_KEYBIT, BTN_SELECT);
    ioctl(*fd, UI_SET_KEYBIT, BTN_START);
    ioctl(*fd, UI_SET_KEYBIT, BTN_MODE);

    ioctl(*fd, UI_SET_EVBIT, EV_ABS);
    ioctl(*fd, UI_SET_ABSBIT, ABS_X);
    ioctl(*fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(*fd, UI_SET_ABSBIT, ABS_RX);
    ioctl(*fd, UI_SET_ABSBIT, ABS_RY);
    ioctl(*fd, UI_SET_ABSBIT, ABS_BRAKE); //LT
    ioctl(*fd, UI_SET_ABSBIT, ABS_GAS);   //RT
    ioctl(*fd, UI_SET_ABSBIT, ABS_HAT0X);
    ioctl(*fd, UI_SET_ABSBIT, ABS_HAT0Y);

    memset(&uidev, 0, sizeof(uidev));
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1234; /* sample vendor */
    uidev.id.product = 0x5678; /* sample product */
    uidev.absmin[ABS_X] = -32768;
    uidev.absmax[ABS_X] = 32767;
    uidev.absmin[ABS_Y] = -32768;
    uidev.absmax[ABS_Y] = 32767;
    uidev.absmin[ABS_RX] = -32768;
    uidev.absmax[ABS_RX] = 32767;
    uidev.absmin[ABS_RY] = -32768;
    uidev.absmax[ABS_RY] = 32767;
    uidev.absmin[ABS_BRAKE] = 0;
    uidev.absmax[ABS_BRAKE] = 255;
    uidev.absmin[ABS_GAS] = 0;
    uidev.absmax[ABS_GAS] = 255;
    uidev.absmin[ABS_HAT0X] = -1;
    uidev.absmax[ABS_HAT0X] = 1;
    uidev.absmin[ABS_HAT0Y] = -1;
    uidev.absmax[ABS_HAT0Y] = 1;
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "my_virtual_gamepad"); //Name of Gamepad

    /* Write the uinput settings */
    write(*fd, &uidev, sizeof(uidev));

    //ioctl(fd, UI_DEV_SETUP, &usetup);

    /* create the device */
    ioctl(*fd, UI_DEV_CREATE);
}

/* Initialise UDP socket */
void UDP_init(void)
{
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }

    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 

    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    //addr.sin_addr.s_addr = inet_addr(192.168.1.1);
    servaddr.sin_port = htons(PORT); 

    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
}


int main(void)
{
    pthread_t thread_id;
    int fd;
    
    /* init mutex */
    if (pthread_mutex_init(&lock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 

    //clear the virtual joystick data structure
    memset(&vjdata, 0, sizeof(vjdata));

    //init the udp device
    UDP_init();

    //create the UDP processing thread
    thread_init(&thread_id);

    //init the virtual joystick
    if(uinput_init(&fd) < 0)
        exit(EXIT_FAILURE); 

    //register the interrpt handler
    signal(SIGINT, intHandler);

    printf("Running - Press CTRL-C to terminate\n");

    /* start updating the controller */
    while (runme) {

        //check for new data
        if(memcmp(vjdata.raw, vjdata_prev.raw, sizeof(vjdata)) != 0)
        {
            
            /* lock the data so that the UDP thread can not write to it */
            pthread_mutex_lock(&lock);

            emit(fd, EV_KEY, BTN_A, vjdata.virtualjoydata.VIRJOY_BTN_A);
            emit(fd, EV_KEY, BTN_B, vjdata.virtualjoydata.VIRJOY_BTN_B);
            emit(fd, EV_KEY, BTN_X, vjdata.virtualjoydata.VIRJOY_BTN_X);
            emit(fd, EV_KEY, BTN_Y, vjdata.virtualjoydata.VIRJOY_BTN_Y);

            emit(fd, EV_KEY, BTN_TL, vjdata.virtualjoydata.VIRJOY_BTN_TL);
            emit(fd, EV_KEY, BTN_TR, vjdata.virtualjoydata.VIRJOY_BTN_TR);
            emit(fd, EV_KEY, BTN_THUMBL, vjdata.virtualjoydata.VIRJOY_BTN_THUMBL);
            emit(fd, EV_KEY, BTN_THUMBR, vjdata.virtualjoydata.VIRJOY_BTN_THUMBR);

            emit(fd, EV_KEY, BTN_DPAD_UP, vjdata.virtualjoydata.VIRJOY_BTN_DPAD_UP);
            emit(fd, EV_KEY, BTN_DPAD_RIGHT, vjdata.virtualjoydata.VIRJOY_BTN_DPAD_RIGHT);
            emit(fd, EV_KEY, BTN_DPAD_DOWN, vjdata.virtualjoydata.VIRJOY_BTN_DPAD_DOWN);
            emit(fd, EV_KEY, BTN_DPAD_LEFT, vjdata.virtualjoydata.VIRJOY_BTN_DPAD_LEFT);

            emit(fd, EV_KEY, BTN_SELECT, vjdata.virtualjoydata.VIRJOY_BTN_SELECT);
            emit(fd, EV_KEY, BTN_START, vjdata.virtualjoydata.VIRJOY_BTN_START);
            emit(fd, EV_KEY, BTN_MODE, vjdata.virtualjoydata.VIRJOY_BTN_MODE);

            emit(fd, EV_ABS, ABS_X, vjdata.virtualjoydata.VIRJOY_ABS_X);
            emit(fd, EV_ABS, ABS_Y, vjdata.virtualjoydata.VIRJOY_ABS_Y);
            emit(fd, EV_ABS, ABS_RX, vjdata.virtualjoydata.VIRJOY_ABS_RX);
            emit(fd, EV_ABS, ABS_RY, vjdata.virtualjoydata.VIRJOY_ABS_RY);

            emit(fd, EV_ABS, ABS_BRAKE, vjdata.virtualjoydata.VIRJOY_ABS_LT);
            emit(fd, EV_ABS, ABS_GAS, vjdata.virtualjoydata.VIRJOY_ABS_RT);
            //emit(fd, EV_ABS, ABS_HAT0X, vjdata.virtualjoydata.VIRJOY_ABS_HAT0X);
            //emit(fd, EV_ABS, ABS_HAT0Y, vjdata.virtualjoydata.VIRJOY_ABS_HAT0Y);

            emit(fd, EV_SYN, SYN_REPORT, 0);
      
            //printf("%d - %d - %d - %d \n", vjdata.virtualjoydata.VIRJOY_BTN_DPAD_UP
            //                       , vjdata.virtualjoydata.VIRJOY_BTN_DPAD_DOWN
            //                       , vjdata.virtualjoydata.VIRJOY_BTN_DPAD_LEFT
            //                       , vjdata.virtualjoydata.VIRJOY_BTN_DPAD_RIGHT);

            /* update the previous data ready for the next pass */
            memcpy(vjdata_prev.raw, vjdata.raw, sizeof(vjdata));

            /* remove mutex lock */
            pthread_mutex_unlock(&lock);
        }

        /* Wait 10ms before trying again */
        usleep(10000);
    }

    /*
     * Give userspace some time to read the events before we destroy the
     * device with UI_DEV_DESTOY.
     */
    sleep(1);

    printf("\nDestroying device\n");

    pthread_exit(NULL); 

    pthread_mutex_destroy(&lock); 

    ioctl(fd, UI_DEV_DESTROY);
    close(fd);

    return 0;
}