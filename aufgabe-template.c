/*taskENTER_CRITICAL();
mvprintw(35, 35, "number: %d    value:%d", current_component_number, current_component_value);
refresh();
taskEXIT_CRITICAL();
*/

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
#define WAAGE_BOTTOM 10
#define WASSERKONTAINER_Y 37

#define MAX_X 19 // how deep in comparison to START_X the bottom lies
// #define CURRENT_X_COMPENSATION 12 // used for converting x to feed it into mischen()

typedef struct{
	int x;
	int y;
	int color;
} drop;


static void vKeyHitTask( void *pvParameters);
static void Waage(void *pvParameters);
static void WasserVentil(void *pvParameters);
static void Mischer(void *pvParameters);


static SemaphoreHandle_t xWaagenSemaphore = NULL;
static SemaphoreHandle_t xWaagenLeerSemaphore = NULL;
static SemaphoreHandle_t xWasserVentilSemaphore = NULL;
static SemaphoreHandle_t xZweitesMischenSemaphore = NULL;

static QueueHandle_t xMischerQueue = NULL;

static TimerHandle_t xMischenTimer = NULL;

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
	mvprintw(START_X+15, START_Y,"|                                           |");
	mvprintw(START_X+16, START_Y,"|                                           |");
	mvprintw(START_X+17, START_Y,"|                                           |");
	mvprintw(START_X+18, START_Y,"|                                           |");
	mvprintw(START_X+19, START_Y,"\\___________________________________________/");
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

	// sync primitives creating
	xWaagenSemaphore = xSemaphoreCreateMutex();
	xSemaphoreTake(xWaagenSemaphore, 0);

	xWaagenLeerSemaphore = xSemaphoreCreateCounting(2, 0);

	xWasserVentilSemaphore = xSemaphoreCreateBinary();
	xSemaphoreTake(xWasserVentilSemaphore, 0);

	xMischerQueue = xQueueCreate(1, sizeof(int));

	TimerCallbackFunction_t MischenCallback(){
		xSemaphoreGive(xWasserVentilSemaphore);
	}

	xMischenTimer = xTimerCreate("Mischen Timer", pdMS_TO_TICKS(5000), pdFALSE, (int *) 1, MischenCallback);

	xZweitesMischenSemaphore = xSemaphoreCreateBinary();
	xSemaphoreTake(xZweitesMischenSemaphore, 0);

	int waage_1_daten[4] = {WAAGE1_Y, 2, 3, 4};
	int waage_2_daten[4] = {WAAGE2_Y, 5, 6, 7};


	// task creating
	xTaskCreate( Waage,			   
				"Waage 1", 					
				configMINIMAL_STACK_SIZE, 		
				(void *) &waage_1_daten,				  	    
				mainKEYBOARD_TASK_PRIORITY,    
				NULL );
	// mvprintw(5, 0, "Task %d", 0);

	xTaskCreate( Waage,			   
				"Waage 2", 					
				configMINIMAL_STACK_SIZE, 		
				(void *) &waage_2_daten,		  	    
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
static void call_recept(){

}

static void fill(u_int8_t color, u_int8_t current_x, u_int8_t current_y, int max_x){
	attron(COLOR_PAIR(color));

	// updating scales_model
	/*
	drop this_drop;
	this_drop.x = current_x;
	this_drop.y = current_y;
	this_drop.color = color;
	scales_model[current_x][current_y] = this_drop;
	*/

	taskENTER_CRITICAL();
	if(current_x == max_x){
		mvprintw(START_X+current_x+1, START_Y+current_y, "_");
	}else{
		mvprintw(START_X+current_x+1, START_Y+current_y, " ");
	}
	refresh();
	taskEXIT_CRITICAL();

	vTaskDelay(20);
}

static void flush(u_int8_t color, u_int8_t current_x, u_int8_t current_y){
	attron(COLOR_PAIR(1));
	/*
	drop this_drop = scales_model[current_x][current_y];
	color = this_drop.color;
	*/
	taskENTER_CRITICAL();
	if(current_x == 9){
		mvprintw(START_X+current_x+1, START_Y+current_y, "_");
	}else{
		mvprintw(START_X+current_x+1, START_Y+current_y, " ");
	}
	refresh();
	taskEXIT_CRITICAL();

	vTaskDelay(20);
	// return color;
}
	

static void Waage (void * pvParameters) {
	int start_y = *(int *) pvParameters;

	int current_x = 9;
	int current_y = 0; 

	int colors[3] = {2, 2, 2};
	colors[0] = * ((int *)pvParameters + 1);
	colors[1] = * ((int *)pvParameters + 2);
	colors[2] = * ((int *)pvParameters + 3);	

	taskENTER_CRITICAL();
	mvprintw(35, 35, "colors 1: %d    2:%d      3:%d", colors[0],colors[1],colors[2]);
	refresh();
	taskEXIT_CRITICAL();
	
	int stage = 1;
	bool fill_scales = false;
	bool flush_scales = false;
	
	/*
	drop def_drop;
	def_drop.color = 1;

	
	drop scales_model[9][7] = { {def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop}
							  };
	*/

	int free_cells = 62;
	
	int components[3] = {9, 9, 9};
	int current_component_number = 0;
	int current_component_value = 0;


	for(;;){
		// what process is now running
		if (xSemaphoreTake(xWaagenSemaphore, 0) == pdTRUE){
			if(stage == 1){
				fill_scales = true;
			}			
			else if (stage == 2){
				flush_scales = true;
			}
		}

		// filling the scales
		if (fill_scales == true){			
			if(current_component_number <= 2 & free_cells >= 0){
				// changing scales_model and visual representation
				fill(colors[current_component_number], current_x, start_y + current_y, 9);

				// coordinate management
				current_y = current_y + 1;
				if(current_y >= 7){
					current_y = 0;
					current_x = current_x - 1;
				}

				// color management
				current_component_value = current_component_value + 1;
				if(current_component_value >= components[current_component_number]){
					current_component_value = 0;
					current_component_number = current_component_number + 1;
				}
				
				// total capacity control
				free_cells = free_cells - 1;

			}else{ // end of filling
				xSemaphoreGive(xWaagenSemaphore);
				fill_scales = false;
				stage = 2;		
				current_x = 9;
				current_y = 0; 
				current_component_number = 2;
				current_component_value = 0;
				vTaskDelay(100);
			}
				
		}

		// flushing the scales
		
		if (flush_scales == true){
			if(xQueueSend(xMischerQueue,(int*)& colors[current_component_value], 0) == pdTRUE){
				flush(colors[current_component_number], current_x, start_y + current_y);				
				
				// coordinate management
				current_y = current_y + 1;
				if(current_y >= 7){
					current_y = 0;
					current_x = current_x - 1;
				}

				// color management
				current_component_value = current_component_value + 1;
				if(current_component_value >= components[current_component_number]){
					current_component_value = 0;
					current_component_number = current_component_number - 1;
				}

				// total capacity control
				free_cells = free_cells + 1;				
			}

			if(free_cells == 63){ // end flushing
				flush_scales = false;
				xSemaphoreGive(xWaagenSemaphore);
				xSemaphoreGive(xWaagenLeerSemaphore);
				vTaskDelete(NULL);
			}
		}
		
	}
}


static void WasserVentil (void *pvParameters) {
	
		for(;;){

	}
}

static void Mischer (void *pvParameters) {
	int current_x = 18;
	int current_y = WAAGE1_Y;
	bool bottom = true;
	int color = 0;

	bool open_ventil = false;
	bool first_mix = false;
	bool second_mix = false;
	unsigned long time_left = 0;

	int free_cells = 43 * 8;

	
	// color, x, y, n_of_rows, time_of_filling, bottom
		for(;;){
			if(xQueueReceive(xMischerQueue,(int*) &color, 0) == pdPASS & current_x > 12){
				fill(color, current_x, current_y, 18);
				current_y = current_y + 1;

				if(current_y >= 43){
					current_y = WAAGE1_Y;
					current_x = current_x - 1;
				}

				free_cells = free_cells - 1;
				
				// 	vTaskDelay(20);
			}

			/*
			if(first_mix == false){
				if (uxSemaphoreGetCount(xWaagenLeerSemaphore) == 2){
					xTimerStart(xMischenTimer, 0);
					first_mix = true;
				}
			}
			

			if(xMischenTimer != NULL){
				if(xTimerIsTimerActive(xMischenTimer) != pdFALSE){
							
				time_left = xTimerGetExpiryTime(xMischenTimer) - xTaskGetTickCount();

					if(time_left > pdMS_TO_TICKS(3000)){
						mischen(current_x, current_y, true);
						vTaskDelay(40);
					}else if(time_left < pdMS_TO_TICKS(3000) & time_left > 0){
						mischen(current_x, current_y, false);
						vTaskDelay(20);
					}else if(time_left <= 100){
						xSemaphoreGive(xWasserVentilSemaphore);
					}
				}
			}
			
			if(xSemaphoreTake(xZweitesMischenSemaphore, 0) == pdTRUE & second_mix == false){
				xTimerReset(xMischenTimer, 0);
				second_mix = true;
			}
			*/
	}
}


static void vKeyHitTask(void *pvParameters) {
	int k = 0;
	bool process_running = false;

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
			case 101:{ //'e'
				call_recept();
			}
			case 115: { //'s'
				if(process_running == false){
					xSemaphoreGive(xWaagenSemaphore);
				}
				process_running = true;
			}
			default:
				taskENTER_CRITICAL();
				attron(COLOR_PAIR(1));
				mvprintw(40, 2, "Command: ");
				refresh();
				taskEXIT_CRITICAL();
		}
	}
} 

/*-----------------------------------------------------------*/


