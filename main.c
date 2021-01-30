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
#include <getopt.h> /* getopt */

#include "joydata.h"

#define DEFAULT_PORT     1235 
#define MAX_STRUCT_BYTES 40

int sockfd;
char buffer[MAX_STRUCT_BYTES];
struct sockaddr_in servaddr, cliaddr;
struct timeval timeout={2,0}; //set timeout for 2 seconds

typedef enum
{
    ANA_DIG_DPAD,
    DIG_ONLY_DPAD,
    ANA_ONLY_DPAD

}dpad_type_e;


unsigned int runme = 1U;
unsigned int data_received = 0U;
dpad_type_e required_dpad = ANA_DIG_DPAD;
unsigned int target_port = DEFAULT_PORT;
unsigned int num_virt_joypad = 1U;

virjoy_un vjdata_p1;
virjoy_un vjdata_p2;
virjoy_un vjdata_p3;
virjoy_un vjdata_p4;

virjoy_un vjdata_prev_p1;
virjoy_un vjdata_prev_p2;
virjoy_un vjdata_prev_p3;
virjoy_un vjdata_prev_p4;

pthread_mutex_t lock_p1;
pthread_mutex_t lock_p2;
pthread_mutex_t lock_p3;
pthread_mutex_t lock_p4;

/* Prototypes */
void process_received_data(virjoy_un *data, virjoy_un *data_prev, int *filedesc, pthread_mutex_t *lock);

/* Very basic checksum. Does not deal with swapped bytes or other simple failures but more than good enough
   for the purpose of this project. If the checksum is included at the end of the passed structure it will
   return zero if the checksum matches. If the checksum is not passed at the end of the structure it will
   return the calculated checksum value for the passed data.*/
unsigned char simple_checksum (unsigned char *ptr, size_t sz) 
{
    unsigned char chk = 0;
    while (sz-- != 0)
        chk -= *ptr++;
    return chk;
}


/* Thread to read UDP events */
void *UDP_Read_Thread(void *vargp) 
{ 
    int len, n;
    virjoy_st *joyptr;

    len = sizeof(cliaddr);
   
    while(runme)
    {

        //echo "This is a test" > /dev/udp/127.0.0.1/1235 //use this to test from terminal

        n = recvfrom(sockfd, (char *)buffer, MAX_STRUCT_BYTES,  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                &len);
      
        if(n > -1)
        {
            if(data_received == 0U)
            {
                data_received = 1U;
                printf("First Packet Received\n");
            }
            
            if ( simple_checksum(buffer, sizeof(virjoy_st)) == 0)
            {
                joyptr = (virjoy_st*)buffer;
                
                switch(joyptr->VIRJOY_PLAYER)
                {
                    case 0U:
                    {
                        pthread_mutex_lock(&lock_p1); 
                        memset(&vjdata_p1, 0, sizeof(virjoy_st));
                        memcpy(vjdata_p1.raw, buffer, sizeof(virjoy_st));
                        pthread_mutex_unlock(&lock_p1);
                    }
                    break;
                    
                    case 1U:
                    {
                        pthread_mutex_lock(&lock_p2); 
                        memset(&vjdata_p2, 0, sizeof(virjoy_st));
                        memcpy(vjdata_p2.raw, buffer, sizeof(virjoy_st));
                        pthread_mutex_unlock(&lock_p2);
                    }
                    break;
                    
                    case 2U:
                    {
                        pthread_mutex_lock(&lock_p3); 
                        memset(&vjdata_p3, 0, sizeof(virjoy_st));
                        memcpy(vjdata_p3.raw, buffer, sizeof(virjoy_st));
                        pthread_mutex_unlock(&lock_p3);
                    }
                    break;
                    
                    case 3U:
                    {
                        pthread_mutex_lock(&lock_p4); 
                        memset(&vjdata_p4, 0, sizeof(virjoy_st));
                        memcpy(vjdata_p4.raw, buffer, sizeof(virjoy_st));
                        pthread_mutex_unlock(&lock_p4);
                    }
                    break;
                    
                    default:
                    {
                        printf("Requested joystick out of range\n");
                    }
                    break;
                }
            }
            else
            {
                printf("Checksum Failed\n");
            }
        }
        //else
           //printf("UDP Timeout\n");    
    } 
    
    pthread_exit(NULL); 
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
    //ioctl(*fd, UI_SET_KEYBIT, BTN_DPAD_UP);
    //ioctl(*fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);
    //ioctl(*fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
    //ioctl(*fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
    ioctl(*fd, UI_SET_KEYBIT, BTN_SELECT);
    ioctl(*fd, UI_SET_KEYBIT, BTN_START);
    ioctl(*fd, UI_SET_KEYBIT, BTN_MODE);
    ioctl(*fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY1); //dpad left
    ioctl(*fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY2); //dpad right
    ioctl(*fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY3); //dpad up
    ioctl(*fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY4); //dpad down

    ioctl(*fd, UI_SET_EVBIT, EV_ABS);
    ioctl(*fd, UI_SET_ABSBIT, ABS_X);
    ioctl(*fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(*fd, UI_SET_ABSBIT, ABS_Z); //LT
    ioctl(*fd, UI_SET_ABSBIT, ABS_RX);
    ioctl(*fd, UI_SET_ABSBIT, ABS_RY);
    ioctl(*fd, UI_SET_ABSBIT, ABS_RZ);   //RT
    ioctl(*fd, UI_SET_ABSBIT, ABS_HAT0Y);
    ioctl(*fd, UI_SET_ABSBIT, ABS_HAT0X);

    memset(&uidev, 0, sizeof(uidev));
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x045e; /* sample vendor */
    uidev.id.product = 0x0291; /* sample product */
    uidev.absmin[ABS_X] = -32767;
    uidev.absmax[ABS_X] = 32767;
    uidev.absmin[ABS_Y] = -32767;
    uidev.absmax[ABS_Y] = 32767;
    uidev.absmin[ABS_RX] = -32767;
    uidev.absmax[ABS_RX] = 32767;
    uidev.absmin[ABS_RY] = -32767;
    uidev.absmax[ABS_RY] = 32767;
    uidev.absmin[ABS_Z] = -32767;
    uidev.absmax[ABS_Z] = 32767;
    uidev.absmin[ABS_RZ] = -32767;
    uidev.absmax[ABS_RZ] = 32767;
    uidev.absmin[ABS_HAT0X] = -32767;
    uidev.absmax[ABS_HAT0X] = 32767;
    uidev.absmin[ABS_HAT0Y] = -32767;
    uidev.absmax[ABS_HAT0Y] = 32767;
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Xbox 360 Wireless Receiver (XBOX)"); //Name of Gamepad

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
    servaddr.sin_port = htons(target_port); 

    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    printf("Using Port : %d\n", target_port);
}

void process_received_data(virjoy_un *data, virjoy_un *data_prev, int *filedesc, pthread_mutex_t *lock)
{
    if(memcmp(data->raw, data_prev->raw, sizeof(data)) != 0)
    {
        /* lock the data so that the UDP thread can not write to it */
        pthread_mutex_lock(lock);

        emit(*filedesc, EV_KEY, BTN_A, data->virtualjoydata.VIRJOY_BTN_A);
        emit(*filedesc, EV_KEY, BTN_B, data->virtualjoydata.VIRJOY_BTN_B);
        emit(*filedesc, EV_KEY, BTN_X, data->virtualjoydata.VIRJOY_BTN_X);
        emit(*filedesc, EV_KEY, BTN_Y, data->virtualjoydata.VIRJOY_BTN_Y);

        emit(*filedesc, EV_KEY, BTN_TL, data->virtualjoydata.VIRJOY_BTN_TL);
        emit(*filedesc, EV_KEY, BTN_TR, data->virtualjoydata.VIRJOY_BTN_TR);
        emit(*filedesc, EV_KEY, BTN_THUMBL, data->virtualjoydata.VIRJOY_BTN_THUMBL);
        emit(*filedesc, EV_KEY, BTN_THUMBR, data->virtualjoydata.VIRJOY_BTN_THUMBR);

        //emit(*filedesc, EV_KEY, BTN_DPAD_UP, data->virtualjoydata.VIRJOY_BTN_DPAD_UP);
        //emit(*filedesc, EV_KEY, BTN_DPAD_RIGHT, data->virtualjoydata.VIRJOY_BTN_DPAD_RIGHT);
        //emit(*filedesc, EV_KEY, BTN_DPAD_DOWN, data->virtualjoydata.VIRJOY_BTN_DPAD_DOWN);
        //emit(*filedesc, EV_KEY, BTN_DPAD_LEFT, data->virtualjoydata.VIRJOY_BTN_DPAD_LEFT);
        
        if((required_dpad == DIG_ONLY_DPAD) ||
            (required_dpad == ANA_DIG_DPAD))
        {
            emit(*filedesc, EV_KEY, BTN_TRIGGER_HAPPY1, data->virtualjoydata.VIRJOY_BTN_DPAD_LEFT); //xpad uses these for DPAD
            emit(*filedesc, EV_KEY, BTN_TRIGGER_HAPPY2, data->virtualjoydata.VIRJOY_BTN_DPAD_RIGHT);
            emit(*filedesc, EV_KEY, BTN_TRIGGER_HAPPY3, data->virtualjoydata.VIRJOY_BTN_DPAD_UP);
            emit(*filedesc, EV_KEY, BTN_TRIGGER_HAPPY4, data->virtualjoydata.VIRJOY_BTN_DPAD_DOWN);
        }
        
        emit(*filedesc, EV_KEY, BTN_SELECT, data->virtualjoydata.VIRJOY_BTN_SELECT);
        emit(*filedesc, EV_KEY, BTN_START, data->virtualjoydata.VIRJOY_BTN_START);
        emit(*filedesc, EV_KEY, BTN_MODE, data->virtualjoydata.VIRJOY_BTN_MODE);

        emit(*filedesc, EV_ABS, ABS_X, data->virtualjoydata.VIRJOY_ABS_X);
        emit(*filedesc, EV_ABS, ABS_Y, data->virtualjoydata.VIRJOY_ABS_Y);
        emit(*filedesc, EV_ABS, ABS_RX, data->virtualjoydata.VIRJOY_ABS_RX);
        emit(*filedesc, EV_ABS, ABS_RY, data->virtualjoydata.VIRJOY_ABS_RY);

        emit(*filedesc, EV_ABS, ABS_Z, data->virtualjoydata.VIRJOY_ABS_LT);
        emit(*filedesc, EV_ABS, ABS_RZ, data->virtualjoydata.VIRJOY_ABS_RT);
        
        if((required_dpad == ANA_ONLY_DPAD) ||
            (required_dpad == ANA_DIG_DPAD))
        {
            emit(*filedesc, EV_ABS, ABS_HAT0X, data->virtualjoydata.VIRJOY_ABS_HAT0X);
            emit(*filedesc, EV_ABS, ABS_HAT0Y, data->virtualjoydata.VIRJOY_ABS_HAT0Y);
        }
        
        emit(*filedesc, EV_SYN, SYN_REPORT, 0);

        /* update the previous data ready for the next pass */
        memcpy(data_prev->raw, data->raw, sizeof(data));

        /* remove mutex lock */
        pthread_mutex_unlock(lock);
    }
}

int main( int argc, char *argv[] )
{
    pthread_t thread_id_p1;
    pthread_t thread_id_p2;
    pthread_t thread_id_p3;
    pthread_t thread_id_p4;
    int fd_p1 = -1;
    int fd_p2 = -1;
    int fd_p3 = -1;
    int fd_p4 = -1;
    
    static const struct option longopts[] = {
        {.name = "ana-dpad-only", .has_arg = no_argument, .val = 'a'},
        {.name = "digi-dpad-only", .has_arg = no_argument, .val = 'd'},
        {.name = "joypad-count", .has_arg = required_argument, .val = 'j'},
        {.name = "port", .has_arg = required_argument, .val = 'p'},
        {.name = "help", .has_arg = no_argument, .val = '?'},
        {},
    };
    
    for (;;) {
        /* DONT FORGET ':' after each parameter that expects an argument */
        int opt = getopt_long(argc, argv, "adj:p:?", longopts, NULL);
        if (opt == -1)
            break;
        switch (opt) {
        case 'a':
            required_dpad = ANA_ONLY_DPAD;
            printf("ANALOGUE DPAD ONLY ACTIVE\n");
            break;
        case 'd':
            required_dpad = DIG_ONLY_DPAD;
            printf("DIGITAL DPAD ONLY ACTIVE\n");
            break;
        case 'j':
            num_virt_joypad = atoi(optarg);
            break;
        case 'p':
            target_port = atoi(optarg);
            break;
        case '?':
        default:
            printf("\n              --------- UDP Joypad Help ---------\n\n");
            printf("     -a                   Dpad press gives analogue events only.\n");
            printf("     --ana-dpad-only\n\n");
            printf("     -d                   Dpad press gives digital events only.\n");
            printf("     --digi-dpad-only\n\n");
            printf("     -j                   Number of virtual joypads to create.\n");
            printf("     --joypad-count\n\n");
            printf("     -p                   Override default UDP port.\n");
            printf("     --port\n\n");
            printf("     -?                   This help.\n");
            printf("     --help\n\n");            
                        
            /* Unexpected option */
            return 1;
            
            break;
        }
    }

    /* Create the virtual joypad(s) 
       Do this in reverse to take advantage of fall through. */
    switch(num_virt_joypad)
    {
        case 4U:
        {
            /* init mutex */
            if (pthread_mutex_init(&lock_p4, NULL) != 0) { 
                printf("\n mutex init has failed\n"); 
                return 1; 
            } 
            memset(&vjdata_p4, 0, sizeof(vjdata_p4));
            if(uinput_init(&fd_p4) < 0)
                exit(EXIT_FAILURE);
            printf("Created Joypad 4\n");
        }
        
        case 3U:
        {
            /* init mutex */
            if (pthread_mutex_init(&lock_p3, NULL) != 0) { 
                printf("\n mutex init has failed\n"); 
                return 1; 
            } 
            memset(&vjdata_p3, 0, sizeof(vjdata_p3));
            if(uinput_init(&fd_p3) < 0)
                exit(EXIT_FAILURE);
            printf("Created Joypad 3\n");
        }

        case 2U:
        {
            /* init mutex */
            if (pthread_mutex_init(&lock_p2, NULL) != 0) { 
                printf("\n mutex init has failed\n"); 
                return 1; 
            } 
            memset(&vjdata_p2, 0, sizeof(vjdata_p2));
            if(uinput_init(&fd_p2) < 0)
                exit(EXIT_FAILURE);
            printf("Created Joypad 2\n");
        }
        
        case 1U:
        {
            /* init mutex */
            if (pthread_mutex_init(&lock_p1, NULL) != 0) { 
                printf("\n mutex init has failed\n"); 
                return 1; 
            } 
            //clear the virtual joystick data structure
            memset(&vjdata_p1, 0, sizeof(vjdata_p1));
            if(uinput_init(&fd_p1) < 0)
                exit(EXIT_FAILURE);
            printf("Created Joypad 1\n");
        }
        break;

        default:
        {
            /* out of range */
            printf("Error: Joypad out of range : %d\n", num_virt_joypad);
            return 1;
        }
        break;
        
    }

    //init the udp device
    UDP_init();

    //create the UDP processing thread
    thread_init(&thread_id_p1);
    //thread_init(&thread_id_p2);
    //thread_init(&thread_id_p3);
    //thread_init(&thread_id_p4);


    //register the interrpt handler
    signal(SIGINT, intHandler);

    printf("Running - Press CTRL-C to terminate\n");

    /* start updating the controller */
    while (runme) {

        if(fd_p1 > -1)
            process_received_data(&vjdata_p1, &vjdata_prev_p1, &fd_p1, &lock_p1);
        if(fd_p2 > -1)
            process_received_data(&vjdata_p2, &vjdata_prev_p2, &fd_p2, &lock_p2);
        if(fd_p3 > -1)
            process_received_data(&vjdata_p3, &vjdata_prev_p3, &fd_p3, &lock_p3);
        if(fd_p4 > -1)
            process_received_data(&vjdata_p4, &vjdata_prev_p4, &fd_p4, &lock_p4);

        /* Wait 10ms before trying again */
        usleep(10000);
    }

    /*
     * Give userspace some time to read the events before we destroy the
     * device with UI_DEV_DESTOY.
     */
    sleep(1);

    printf("\nDestroying device\n");
    
    if(fd_p1 > -1){
        pthread_mutex_destroy(&lock_p1);
        ioctl(fd_p1, UI_DEV_DESTROY);
        close(fd_p1);
        printf("Closed Joypad 1\n");
    }
    
    if(fd_p2 > -1){
        pthread_mutex_destroy(&lock_p2);
        ioctl(fd_p2, UI_DEV_DESTROY);
        close(fd_p2);
        printf("Closed Joypad 2\n");
    }

    if(fd_p3 > -1){
        pthread_mutex_destroy(&lock_p3);
        ioctl(fd_p3, UI_DEV_DESTROY);
        close(fd_p3);
        printf("Closed Joypad 3\n");
    }
    
    if(fd_p4 > -1){
        pthread_mutex_destroy(&lock_p4);
        ioctl(fd_p4, UI_DEV_DESTROY);
        close(fd_p4);
        printf("Closed Joypad 4\n");
    }

    return 0;
}
