
/* Standard includes. */
#include <time.h>
/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */

#include <ncurses.h>
#include <stdlib.h>
// #include <libconfig.h>

#define INCLUDE_vTaskDelete 1
#define N_TASKS 1
#define mainKEYBOARD_TASK_PRIORITY          ( tskIDLE_PRIORITY + 0 )
#define START_X 10
#define START_Y 10
#define WAAGE1_Y 1
#define WAAGE2_Y 19
#define WASSERKONTAINER_Y 37



static void vKeyHitTask( void *pvParameters);
static void Waage(void *pvParameters);
static void WasserVentil(void *pvParameters);
static void Mischer(void *pvParameters);


static SemaphoreHandle_t xWaagenSemaphore = NULL;
static TimerHandle_t xTimer = NULL;

bool quit = false;
int TaskData[N_TASKS];
/*-----------------------------------------------------------*/

static void drawEquipment(){
	taskENTER_CRITICAL();
	mvprintw(START_X+1, START_Y, "/-------\\         /-------\\         /-------\\");
	mvprintw(START_X+2, START_Y, "|       |         |       |         |       |");
	mvprintw(START_X+3, START_Y, "|       |         |       |         |       |");
	mvprintw(START_X+4, START_Y, "|       |         |       |         |       |");
	mvprintw(START_X+5, START_Y, "|       |         |       |         |       |");
	mvprintw(START_X+6, START_Y, "|       |         |       |         |       |");
	mvprintw(START_X+7, START_Y, "|       |         |       |         |       |");
	mvprintw(START_X+8, START_Y, "|       |         |       |         |       |");
	mvprintw(START_X+9, START_Y, "|       |         |       |         |       |");
	mvprintw(START_X+10, START_Y,"\\_______/         \\_______/         \\_______/");
	mvprintw(START_X+11, START_Y, "   | |===============| |===============| |");
	mvprintw(START_X+12, START_Y,"|                                           |");
	mvprintw(START_X+13, START_Y,"|                                           |");
	mvprintw(START_X+14, START_Y,"|                                           |");
	mvprintw(START_X+15, START_Y,"\\___________________________________________/");
	refresh();
	taskEXIT_CRITICAL();
}

void main_rtos( void )
{
	drawEquipment();

	//initialise recipy
	// LPCSTR Rezept = "./mischer.ini"

	// settings of color usind ncurses.h library
	initscr();
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_RED);
	init_pair(3, COLOR_WHITE, COLOR_YELLOW);
	init_pair(4, COLOR_WHITE, COLOR_GREEN);
	init_pair(5, COLOR_WHITE, COLOR_CYAN);
	init_pair(6, COLOR_WHITE, COLOR_WHITE);
	init_pair(7, COLOR_WHITE, COLOR_RED);
	init_pair(8, COLOR_WHITE, COLOR_BLUE);
	init_pair(9, COLOR_WHITE, COLOR_MAGENTA);

	// semaphore creating
	xWaagenSemaphore = xSemaphoreCreateMutex();
	xSemaphoreGive(xWaagenSemaphore);


	// task creating
	xTaskCreate( Waage,			   
				"Waage 1", 					
				configMINIMAL_STACK_SIZE, 		
				(void*) WAAGE1_Y, 					  	    
				mainKEYBOARD_TASK_PRIORITY,    
				NULL );
	mvprintw(5, 0, "Task %d", 0);

	xTaskCreate( Waage,			   
				"Waage 2", 					
				configMINIMAL_STACK_SIZE, 		
				(void*) WAAGE2_Y, 					  	    
				mainKEYBOARD_TASK_PRIORITY,    
				NULL );
	
	xTaskCreate( WasserVentil,			   
				"Wasser Ventil", 					
				configMINIMAL_STACK_SIZE, 		
				(void*) 1, 					  	    
				mainKEYBOARD_TASK_PRIORITY,    
				NULL );

	xTaskCreate( Mischer,			   
				"Mischer", 					
				configMINIMAL_STACK_SIZE, 		
				(void*) 1, 					  	    
				mainKEYBOARD_TASK_PRIORITY,    
				NULL );

	xTaskCreate(vKeyHitTask, "Keyboard", configMINIMAL_STACK_SIZE, NULL, mainKEYBOARD_TASK_PRIORITY, NULL );

	vTaskStartScheduler();

	for( ;; );
}

/*-----------------------------------------------------------*/

void fill(u_int8_t color, u_int8_t x, u_int8_t y, u_int8_t n_of_rows, u_int32_t time_of_filling, bool bottom){
	taskENTER_CRITICAL();
	attron(COLOR_PAIR(color));

	float delay = pdMS_TO_TICKS(time_of_filling / n_of_rows); 

	if(bottom == pdTRUE){
		mvprintw(START_X+x, START_Y+y, "_______");
	}else{
		mvprintw(START_X+x, START_Y+y, "       ");
	}

	vTaskDelay(delay);
	for(int i=1; i<=n_of_rows-1; i++){
		attron(COLOR_PAIR(color));
		mvprintw(START_X+x-i, START_Y+y, "       ");
		vTaskDelay(delay);
	}

	refresh();
	taskEXIT_CRITICAL();
}
	

static void Waage (void *pvParameters) {
	u_int8_t uc_y = (u_int8_t) pvParameters;
	for(;;){
		if (xSemaphoreTake(xWaagenSemaphore, 0) == pdTRUE){
			
			// char[100] returnValue;
			// GetPrivateProfileString("Waage %d", 2, "Komponent 1", 0, returnValue, 100, ini);

			// color, x, y, n_of_rows, time_of_filling, bottom
			fill(2, 10, uc_y, 2, 1000, true);
			fill(3, 8, uc_y, 2, 1000, false);
			fill(4, 6, uc_y, 2, 1000, false);
			xSemaphoreGive(xWaagenSemaphore);	
			vTaskDelete(NULL);
		}
	}
}

static void WasserVentil (void *pvParameters) {
	// color, x, y, n_of_rows, time_of_filling, bottom
	fill(8, 10, WASSERKONTAINER_Y, 7, 0, true);
		for(;;){

	}
}

static void Mischer (void *pvParameters) {

for(;;){}
}


static void vKeyHitTask(void *pvParameters) {
	int k = 0;

    attron(A_BOLD);
    attron(COLOR_PAIR(1));
    mvprintw(2, 20, "Press 'q' to quit application !!!\n");
    attroff(A_BOLD);
    attroff(COLOR_PAIR(1));
	refresh();

	/* Ideally an application would be event driven, and use an interrupt to process key
	 presses. It is not practical to use keyboard interrupts when using the FreeRTOS 
	 port, so this task is used to poll for a key press. */
	for (;;) {
		k = getch();
		switch(k) {
			case 113: { //'q'
				quit = true;
				endwin();
				exit(2);
			}
			default:
				break;
		}
	}
}

/*-----------------------------------------------------------*/


