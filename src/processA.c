#include "./../include/processA_utilities.h"
#include <bmpfile.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <strings.h> 
#include <sys/socket.h>
#include <unistd.h> 
#include <arpa/inet.h> 

#define WIDTH 1600
#define CENTERW WIDTH/2
#define HEIGHT 600
#define CENTERH HEIGHT/2
#define DEPTH 4
#define DIMFACTOR 20
#define RADIUS 20

#define SEM_PATH "/sem_image_1"  //useful for the semaphore

// variables for shared memory
char *shm_name="/IMAGE";
int size=WIDTH*HEIGHT*sizeof(rgb_pixel_t);
int fd_shm;
rgb_pixel_t *ptr;

bmpfile_t *bmp;   // for bitmap

sem_t *sem1;   //useful for the semaphore

char *master_a="/tmp/master_a";
int fd_ma;

FILE* logfile;
time_t curtime;


int server_fd, client_fd; // file descriptors for server and client

void controller(int function,int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Error: %s Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"\nTIME: %s Error: %s Line: %d\n",ctime(&curtime),strerror(errno),line);
        fflush(logfile);

        fclose(logfile);

        unlink(shm_name);
        close(fd_shm);
        munmap(ptr,size);
        bmp_destroy(bmp);
        sem_close(sem1);
        sem_unlink(SEM_PATH);

        close(server_fd);
        close(client_fd);

        usleep(1000000);

        kill(getppid(),SIGKILL);

        
        exit(EXIT_FAILURE);
    }
}

void sa_function(int sig){

    time(&curtime);

    if(sig==SIGTERM || sig==SIGINT){

        // closing all resources

        fprintf(logfile,"\nTIME: %s Process terminated\n",ctime(&curtime));
        fflush(logfile);

        fclose(logfile);

        unlink(shm_name);
        close(fd_shm);
        munmap(ptr,size);
        bmp_destroy(bmp);
        sem_close(sem1);
        sem_unlink(SEM_PATH);

        close(server_fd);
        close(client_fd);

        usleep(1000000);

        kill(getppid(),SIGKILL);

        exit(EXIT_SUCCESS);

    }
}

void bmp_circle(bmpfile_t *bmp,int w,int h){

    rgb_pixel_t p={0,255,0,0};

    for(int x = -RADIUS; x <= RADIUS; x++) {
        for(int y = -RADIUS; y <= RADIUS; y++) {
        // If distance is smaller, point is within the circle
            if(sqrt(x*x + y*y) < RADIUS) {
                /*
                * Color the pixel at the specified (x,y) position
                * with the given pixel values
                */
                bmp_set_pixel(bmp, w*DIMFACTOR + x, h*DIMFACTOR + y, p);  //multiply the coordinates by 20 to represent the circle on the bitmap
            }
        } 
    }
}

void delete(bmpfile_t *bmp){

    rgb_pixel_t pixel={0,0,255,0};   //BGRA

    for(int i=0;i<HEIGHT;i++)    
        for(int j=0;j<WIDTH;j++)           
            bmp_set_pixel(bmp,j,i,pixel);   //removing all pixels except for the red pixels
}

void static_conversion(bmpfile_t *bmp,rgb_pixel_t *ptr){
    
    rgb_pixel_t* p;

    for(int i=0;i<HEIGHT;i++)   
        for(int j=0;j<WIDTH;j++){   
            p=bmp_get_pixel(bmp,j,i);

            ptr[i+HEIGHT*j].alpha=p->alpha;      //passing the data from the bitmap to the shared memory treating it as an array
            ptr[i+HEIGHT*j].blue=p->blue;
            ptr[i+HEIGHT*j].green=p->green;
            ptr[i+HEIGHT*j].red=p->red;
        }
}

int menu(){

    int choice;

    printf("MENU\n"
    "1. Normal mode\n"
    "2. Client mode\n"
    "3. Server mode\n"
    "4. Exit\n");

    fflush(stdout);
    printf("\nChoose a mode: ");
    scanf("%d",&choice);

    return choice;
}

void normal_mode(){

    int first_resize = TRUE;
    init_console_ui();

    int cmd=0;

    while(cmd!='e'){

        // Get input in non-blocking mode
        cmd = getch();
                // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
                }
            else {
                reset_console_ui();
                }
            }

        // Else, if user presses print button...
        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    time(&curtime);
                    bmp_save(bmp,"./screenshot/imageA.bmp");
                    fprintf(logfile,"\nTIME: %s Screenshot of the bitmap\n",ctime(&curtime));
                    fflush(logfile);
                }
            }
        }

                // If input is an arrow key, move circle accordingly...
        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {

            move_circle(cmd);

            draw_circle();

            delete(bmp); //clean the bitmap

            time(&curtime);
            fprintf(logfile,"\nTIME: %s Cleaning of the bitmap\n",ctime(&curtime));
            fflush(logfile);

            bmp_circle(bmp,circle.x,circle.y);  //drawing the circle in the image with the coordinates of the konsole circle

            controller(sem_wait(sem1),__LINE__);  //wait sem

            static_conversion(bmp,ptr);

            time(&curtime);
            fprintf(logfile,"\nTIME: %s Conversion for shared memory\n",ctime(&curtime));
            fflush(logfile);

            controller(sem_post(sem1),__LINE__);  //signal sem
        }
    
        mvprintw(LINES - 1, 1, "Press 'e' to exit from normal mode\n");
    }
    return ;
}

void client_mode(int sfd){

    int first_resize = TRUE;
    init_console_ui();

    int cmd=0;

    char send[32];

    while(cmd!='e'){

        cmd=getch();  //get command 

        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
                }
            else {
                reset_console_ui();
                }
            }

        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {

                    sprintf(send,"%d",cmd);  // conversion in char

                    controller(write(sfd,send,sizeof(send)),__LINE__);  //send command

                    time(&curtime);
                    bmp_save(bmp,"./screenshot/imageA.bmp");
                    fprintf(logfile,"\nTIME: %s Screenshot of the bitmap\n",ctime(&curtime));
                    fflush(logfile);
                }
            }
        }

        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {

            sprintf(send,"%d",cmd);  // conversion in char

            controller(write(sfd,send,sizeof(send)),__LINE__);   //send command
            time(&curtime);
            fprintf(logfile,"\nTIME: %s Command sent to the server\n",ctime(&curtime));
            fflush(logfile);

            move_circle(cmd);

            draw_circle();

            delete(bmp); //clean the bitmap

            time(&curtime);
            fprintf(logfile,"\nTIME: %s Cleaning of the bitmap\n",ctime(&curtime));
            fflush(logfile);

            bmp_circle(bmp,circle.x,circle.y);  //drawing the circle in the image with the coordinates of the konsole circle

            controller(sem_wait(sem1),__LINE__);  //wait sem

            static_conversion(bmp,ptr);

            time(&curtime);
            fprintf(logfile,"\nTIME: %s Conversion for shared memory \n",ctime(&curtime));
            fflush(logfile);

            controller(sem_post(sem1),__LINE__);  //signal sem
        }
        mvprintw(LINES - 1, 1, "Press 'e' to exit from client mode\n");
    }
    return ;
}

void server_mode(int cfd){   

    int first_resize = TRUE;
    init_console_ui();

    int quit=0,cmd;

    char receive[32];

    while(quit!='e'){
        
        mvprintw(LINES - 1, 1, "Press 'e' to exit from server mode after the client exited\n");
        
        quit=getch();

        controller(read(cfd,receive,sizeof(receive)),__LINE__);

        time(&curtime);
        fprintf(logfile,"\nTIME: %s Command received by the client\n",ctime(&curtime));
        fflush(logfile);

        cmd=atoi(receive);
        
        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
                }
            else {
                reset_console_ui();
                }
            }

        // Else, if user presses print button...
        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    time(&curtime);
                    bmp_save(bmp,"./screenshot/imageA.bmp");
                    fprintf(logfile,"\nTIME: %s Screenshot of the bitmap\n",ctime(&curtime));
                    fflush(logfile);
                }
            }
        }

                // If input is an arrow key, move circle accordingly...
        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {

            move_circle(cmd);

            draw_circle();

            delete(bmp); //clean the bitmap

            time(&curtime);
            fprintf(logfile,"\nTIME: %s Cleaning of the bitmap\n",ctime(&curtime));
            fflush(logfile);

            bmp_circle(bmp,circle.x,circle.y);  //drawing the circle in the image with the coordinates of the konsole circle

            controller(sem_wait(sem1),__LINE__);  //wait sem

            static_conversion(bmp,ptr);

            time(&curtime);
            fprintf(logfile,"\nTIME: %s Conversion for shared memory \n",ctime(&curtime));
            fflush(logfile);

            controller(sem_post(sem1),__LINE__);  //signal sem
        }
    }
    return ;
}


int main(int argc, char *argv[])
{   
    // variables useful for client-server mode
    int connection_fd, client_size,client_port,server_port,choice;
    struct sockaddr_in serv_addr, cli_addr;
    char ip[16];

    logfile=fopen("./logfiles/ProcessA_logfile","w");  // open logfile

    // sigaction useful for handling SIGTERM AND SIGINT signals
    struct sigaction sa;
    
    memset(&sa,0,sizeof(sa));
    sa.sa_flags=SA_RESTART;
    sa.sa_handler=&sa_function;
    controller(sigaction(SIGTERM,&sa,NULL),__LINE__);
    controller(sigaction(SIGINT,&sa,NULL),__LINE__);

    // pid of this process
    pid_t pid_A=getpid();

    bmp = bmp_create(WIDTH, HEIGHT, DEPTH);   //creating bitmap

    if(bmp==NULL){
        exit(EXIT_FAILURE);
    }
            
    controller(fd_shm=shm_open(shm_name, O_CREAT|O_RDWR,0666),__LINE__);   //open the file descriptor of the shared memory

    controller(ftruncate(fd_shm,size),__LINE__);  //ftruncate for this shared memory

    ptr=(rgb_pixel_t*)mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd_shm,0);   //mmap
    if(ptr==MAP_FAILED){
        fprintf(stderr,"Error %s Line: %d\n",strerror(errno),__LINE__);
        exit(EXIT_FAILURE);
    }

    sem1=sem_open(SEM_PATH,O_CREAT,S_IRUSR | S_IWUSR,1);   // open the semaphore
    if(sem1==SEM_FAILED){
        fprintf(stderr,"Error: %s Line: %d\n",strerror(errno),__LINE__);
        fflush(stderr);
        unlink(shm_name);
        munmap(ptr,size);
        bmp_destroy(bmp);
        sem_unlink(SEM_PATH);
        exit(EXIT_FAILURE);
    }

    controller(sem_init(sem1,1,1),__LINE__);   //initialize the semaphore

    //opening pipe, writing and closing

    controller(fd_ma=open(master_a,O_WRONLY),__LINE__);

    controller(write(fd_ma,&pid_A,sizeof(pid_A)),__LINE__);

    close(fd_ma);
    //ending

    while(TRUE){

        time(&curtime);
        delete(bmp); //clean the bitmap

        fprintf(logfile,"\nTIME: %s Cleaning of the bitmap\n",ctime(&curtime));
        fflush(logfile);

        bmp_circle(bmp,CENTERW/DIMFACTOR,CENTERH/DIMFACTOR);  //drawing a circle in the center of the image
            
        controller(sem_wait(sem1),__LINE__);  //wait sem

        static_conversion(bmp,ptr);    //passing data from the bitmap to the shared memory

        time(&curtime);
        fprintf(logfile,"\nTIME: %s Conversion for shared memory \n",ctime(&curtime));
        fflush(logfile);

        controller(sem_post(sem1),__LINE__);  //signal sem

        choice=menu();
        
        switch(choice){

            case 1:
                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Normal mode choosen\n",ctime(&curtime));
                    fflush(logfile);

                    normal_mode();

                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Normal mode closed\n",ctime(&curtime));
                    fflush(logfile);

                    break;
            case 2:
                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Client mode choosen\n",ctime(&curtime));
                    fflush(logfile);
                    
                    controller(client_fd=socket(AF_INET, SOCK_STREAM, 0),__LINE__);  //create socket

                    bzero((char *)&serv_addr, sizeof(serv_addr));  //initialise serv_addr

                    //asking for ip and port of the server
                    printf("IP Address: ");
                    fflush(stdout);
                    scanf("%s",ip);
                    fflush(stdin);
                    printf("Port: ");
                    fflush(stdout);
                    scanf("%d",&client_port);
                    fflush(stdin);

                    // parameters
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_addr.s_addr = inet_addr(ip);
                    serv_addr.sin_port = htons(client_port); 

                    controller(connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)),__LINE__); //connect to the server
                        
                     
                    printf("Connected to the server\n");
                    fflush(stdout);

                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Connected to the server\n",ctime(&curtime));
                    fflush(logfile);

                    usleep(1000000);  //useful for reading the result of the connect

                    client_mode(client_fd);   // execute the core of the client mode

                    close(client_fd);  // close client socket

                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Client mode closed\n",ctime(&curtime));
                    fflush(logfile);

                    break;
            case 3:
                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Server mode choosen\n",ctime(&curtime));
                    fflush(logfile);
                    
                    client_size = sizeof(cli_addr);

                    controller(server_fd=socket(AF_INET, SOCK_STREAM, 0),__LINE__);

                    bzero((char *)&serv_addr, sizeof(serv_addr)); // initialize serv_addr

                    // asking for ip and port
                    printf("Port: ");
                    fflush(stdout);
                    scanf("%d",&server_port);
                    fflush(stdin);

                    // parameters
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(server_port);
                    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                        
                    // bind
                    controller(bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)),__LINE__);

                    // server is listening
                    controller(listen(server_fd, 5),__LINE__);
                    printf("Server is listening\n");
                    fflush(stdout);

                    // accepting
                    controller(connection_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &client_size),__LINE__);
                    printf("Connection accepted\n");
                    fflush(stdout);

                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Connection accepted\n",ctime(&curtime));
                    fflush(logfile);

                    usleep(1000000);  //useful for reading the result of the accept

                    server_mode(connection_fd);  //execute the core of the server mode

                    close(server_fd);   // close server socket

                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Server mode closed\n",ctime(&curtime));
                    fflush(logfile);

                    break;
            case 4:
                    printf("Exiting!\n");
                    fflush(stdout);

                    // close all resources when exit is choosen using the SIGTERM signal handler
                    
                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s Exit choosen\n",ctime(&curtime));
                    fflush(logfile);
                    
                    controller(kill(pid_A,SIGTERM),__LINE__);

                    exit(EXIT_SUCCESS);
            default:
                    printf("\nThe choice is not correct\n");
                    fflush(stdout);

                    usleep(1000000); 

                    time(&curtime);
                    fprintf(logfile,"\nTIME: %s The choice is not correct\n",ctime(&curtime));
                    fflush(logfile);

                    break;
        }
        
        //useful to reset console and delete the print of the menu
        reset_console_ui(); 
        endwin();
        system("clear");
    
    } 

    // close all resources 
        
    unlink(shm_name);
    close(fd_shm);
    controller(munmap(ptr,size),__LINE__);
    bmp_destroy(bmp);

    controller(sem_close(sem1),__LINE__);
    controller(sem_unlink(SEM_PATH),__LINE__);

    time(&curtime);
    fprintf(logfile,"\nTIME: %s Process terminated \n",ctime(&curtime));
    fflush(logfile);

    fclose(logfile);


    return 0;
}
