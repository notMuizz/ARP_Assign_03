#include "./../include/processB_utilities.h"
#include <bmpfile.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <signal.h>

#define WIDTH 1600
#define HEIGHT 600
#define DEPTH 4
#define DIMFACTOR 20
#define RADIUS 20

#define SEM_PATH "/sem_image_1"

char *shm_name="/IMAGE";
int size=WIDTH*HEIGHT*sizeof(rgb_pixel_t);
int fd_shm;
rgb_pixel_t *ptr;

bmpfile_t *bmp;

sem_t *sem1;
sem_t *sem2;

char *master_b="/tmp/master_b";
int fd_mb;

time_t curtime;

FILE *logfile;

typedef struct {
    int x;
    int y;
}position;


void controller(int function,int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Error: %s Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"\nTime: %s Error: %s Line: %d\n",ctime(&curtime),strerror(errno),line);
        fflush(logfile);
        fclose(logfile);

        //munmap(ptr,size);
        unlink(shm_name);
        sem_close(sem1);
        sem_unlink(SEM_PATH);
        bmp_destroy(bmp);

        exit(EXIT_FAILURE);
    }
}

void sa_function(int sig){

    if(sig==SIGTERM || sig==SIGINT){
        unlink(shm_name);
        //munmap(ptr,size);
        bmp_destroy(bmp);
        sem_close(sem1);
        sem_unlink(SEM_PATH);

        fprintf(logfile,"\nTIME: %s Process terminated\n",ctime(&curtime));
        fflush(logfile);

        fclose(logfile);

        exit(EXIT_SUCCESS);
    }
    
}

void conversion(bmpfile_t *bmp,rgb_pixel_t *ptr){

    rgb_pixel_t p;

    for(int i=0;i<HEIGHT;i++)
        for(int j=0;j<WIDTH;j++){
            p=ptr[i+HEIGHT*j];          //passing the data from the shared memory to the bitmap treating it as an array

            bmp_set_pixel(bmp,j,i,p);   
        }
}

position get_center(bmpfile_t *bmp){

    position pose;
    rgb_pixel_t *p;
    int max=0,count=0,width,height;

    for(int i=0;i<HEIGHT;i++){   //  x
        for(int j=0;j<WIDTH;j++){    // y
            p=bmp_get_pixel(bmp,j,i);
            if(p->green && p->alpha==0 && p->blue==0 && p->red==0){   // only green
               count++;  //current radius
            }
            if(count==RADIUS){   //in this case we are in centre of the circle
                width=j;
                height=i;
            }
        }      
        if(count==RADIUS)
            break;
    }

    pose.x=height/DIMFACTOR;  // dividing by 20 because I have to draw zeros in the konsole
    pose.y=width/DIMFACTOR;

    return pose;
}

void delete(bmpfile_t *bmp){

    rgb_pixel_t pixel={0,0,255,0};   //BGRA

    for(int i=0;i<HEIGHT;i++)
        for(int j=0;j<WIDTH;j++)
            bmp_set_pixel(bmp,j,i,pixel);   //removing all pixels except for the red pixels
}

int main(int argc, char const *argv[]){

    logfile=fopen("./logfiles/ProcessB_logfile","w");

    position p;

    pid_t b=getpid();


    struct sigaction sa;
    
    memset(&sa,0,sizeof(sa));
    //sigemptyset(&stop_reset.sa_mask);
    sa.sa_flags=SA_RESTART;
    sa.sa_handler=&sa_function;
    controller(sigaction(SIGTERM,&sa,NULL),__LINE__);
    controller(sigaction(SIGINT,&sa,NULL),__LINE__);




    bmp = bmp_create(WIDTH, HEIGHT, DEPTH);  //creating bitmap
    if(bmp==NULL){
        exit(EXIT_FAILURE);
    }
        
    controller(fd_shm=shm_open(shm_name, O_CREAT|O_RDWR,0666),__LINE__);  //open the file descriptor of the shared memory

    //controller(ftruncate(fd_shm,size),__LINE__);

    // not necessary to use ftruncate, I used it in process A

    ptr=(rgb_pixel_t*)mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd_shm,0);
    if(ptr==MAP_FAILED){
        exit(EXIT_FAILURE);
    }
    
    sem1=sem_open(SEM_PATH,0);   // open the semaphore
    if(sem1==SEM_FAILED){
        fprintf(stderr,"Error: %s Line: %d\n",strerror(errno),__LINE__);
        fflush(stderr);
        unlink(shm_name);
        bmp_destroy(bmp);
        sem_unlink(SEM_PATH);
        exit(EXIT_FAILURE);
    }


    //opening pipe, writing and closing

    controller(fd_mb=open(master_b,O_WRONLY),__LINE__);

    controller(write(fd_mb,&b,sizeof(b)),__LINE__);

    close(fd_mb);

    //ending

    time(&curtime);
    delete(bmp);  //clean the bitmap
    
    fprintf(logfile,"Cleaning of the bitmap\n");
    fflush(logfile);

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    // Infinite loop
    while (TRUE) {

        mvprintw(LINES - 2, 1, "Press 'r' to cancel zeros\n");
        mvprintw(LINES - 1, 1, "Press 's' to make a screenshot\n");
        
        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }

        else if(cmd=='s'){
            time(&curtime);
            bmp_save(bmp,"./screenshot/imageB.bmp");  //another screenshot to check the data
            fprintf(logfile,"\nTIME: %s Screenshot of the bitmap\n",ctime(&curtime));
            fflush(logfile);
        }

        else if(cmd=='r'){           //reset the konsole
            reset_console_ui(); 
            endwin();
        }

        else {

            controller(sem_wait(sem1),__LINE__); //wait sem

            conversion(bmp,ptr);

            time(&curtime);
            fprintf(logfile,"\nTIME: %s Conversion in bitmap\n",ctime(&curtime));
            fflush(logfile);
            
            controller(sem_post(sem1),__LINE__); //signalsem

            p=get_center(bmp);   //obtaining the coordinates center

            ctime(&curtime);
            fprintf(logfile,"\nTIME: %s x:%d, y:%d\n",ctime(&curtime),p.x,p.y);
            fflush(logfile);

            mvaddch(p.x, p.y, '0');
            
            refresh();
        }
    }

    endwin();

    // closing, I will never arrive here, I wrote them for security

    controller(unlink(shm_name),__LINE__);

    //controller(munmap(ptr,size),__LINE__);
   
    bmp_destroy(bmp);

    controller(sem_close(sem1),__LINE__);
    controller(sem_unlink(SEM_PATH),__LINE__);

    fclose(logfile);

    
    return 0;
}
