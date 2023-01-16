
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
	/*
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
	*/
	xTaskCreate(vKeyHitTask, "Keyboard", configMINIMAL_STACK_SIZE, NULL, mainKEYBOARD_TASK_PRIORITY, NULL );

	vTaskStartScheduler();

	for( ;; );
}

/*-----------------------------------------------------------*/

void fill(drop scales_model[9][7], u_int8_t color, u_int8_t current_x, u_int8_t current_y){
	taskENTER_CRITICAL();
	attron(COLOR_PAIR(color));

	drop this_drop;
	this_drop.x = current_x;
	this_drop.y = current_y;
	this_drop.color = color;
	scales_model[current_x][current_y] = this_drop;

	if(current_x == 9){
		mvprintw(START_X+current_x+1, START_Y+1+current_y, "_");
	}else{
		mvprintw(START_X+current_x+1, START_Y+1+current_y, " ");
	}

	vTaskDelay(20);
	refresh();
	taskEXIT_CRITICAL();
}
	

static void Waage (void *pvParameters) {
	u_int8_t uc_y = (u_int8_t) pvParameters;
	
	int current_x = 9;
	int current_y = 0; 

	// ought to refactor
	int colors[3] = {2,3,4};
	int current_color = 0;
	int call_counter = 0;
	
	int stage = 1;
	bool fill_scales = false;
	bool flush = false;

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
								{def_drop, def_drop, def_drop, def_drop, def_drop, def_drop, def_drop},
							  };
	
	int free_cells = 62;
	
	int components[3] = {9,9,9};
	int current_component_number = 0;
	int current_component_value = 0;


	for(;;){
		// what process is now running
		if (xSemaphoreTake(xWaagenSemaphore, 0) == pdTRUE){
			if(stage == 1){
				fill_scales = true;
			}			
			else if (stage == 2){
				flush = true;
			}
		}

		// filling the scales
		if (fill_scales == true){			
			if(current_component_number <= 2 & free_cells >= 0){
				fill(scales_model , colors[current_component_number], current_x, current_y);

				// coordinate management
				current_y = current_y + 1;
				if(current_y >= 7){
					current_y = 0;
					current_x = current_x - 1;
				}

				// color management
				if(current_component_value <= components[current_component_number]){
					current_component_value = current_component_number + 1;
				}else{
					current_component_value = 0;
					current_component_number = current_component_number + 1;
				}
				
				free_cells = free_cells - 1;
			}else{ // end of filling
				xSemaphoreGive(xWaagenSemaphore);
				stage = 2;		
				vTaskDelay(100);
			}
				
		}

		// flushing the scales
		/*
		if (flush == true){
			if(xQueueSend(xMischerQueue,(int*)& colors[current_color], 0) == pdTRUE){
				
				// color, x, y, n_of_rows, time_of_filling, bottom
				fill(1, current_x, uc_y, 2, 100, bottom);
				// color, x, y, length
				pour(colors[current_color], 9, uc_y+3);
				bottom = false;
				
				current_x = current_x - 1;
				call_counter = call_counter + 1;

				if(call_counter >= 3){
					call_counter = 0;
					if(current_color <= 3){
						current_color = current_color + 1;
					}
				}

			}

			if(current_x<=2){
				flush = false;
				pour(1, 9, uc_y+3);
				xSemaphoreGive(xWaagenSemaphore);
				xSemaphoreGive(xWaagenLeerSemaphore);
				vTaskDelete(NULL);
			}
		}*/
		
	}
}

/*
static void WasserVentil (void *pvParameters) {
	// color, x, y, n_of_rows, time_of_filling, bottom
	fill(8, 10, WASSERKONTAINER_Y, 7, 0, true);
		for(;;){

	}
}

static void Mischer (void *pvParameters) {
	int current_x = 19;
	int current_y = WAAGE1_Y;
	bool bottom = true;
	int color = 0;

	bool open_ventil = false;
	bool first_mix = false;
	bool second_mix = false;
	unsigned long time_left = 0;


	// color, x, y, n_of_rows, time_of_filling, bottom
		for(;;){
			if(xQueueReceive(xMischerQueue,(int*) &color, 0) == pdPASS & current_x > 12){
				fill(color, current_x, current_y, 1, 0, bottom);
				current_y = current_y + 7;

				if(current_y >= 43){
					current_y = WAAGE1_Y;
					current_x = current_x - 1;
					bottom = false;
				}
				
				// vTaskDelay(100);
				taskENTER_CRITICAL();
				mvprintw(35, 35, "Current x: %d  , Current y: %d  ", current_x, current_y);
				refresh();
				taskEXIT_CRITICAL();
			}

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

	}
}
*/

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


