#include <stdio.h>
#include "LPC17xx.H"
#include <RTL.h>
#include "Serial.h"
#include "LED.h"
#include "ADC.h"
#include <stdbool.h>
#include <RTL.h>
#include "KBD.h"
#include "GLCD.h"
#include "image_bitmaps.h"

#define BG Black
#define YG Yellow
#define BL Blue
#define GR Green
#define WH White


#define __FI        1

#define ONE 0x1
#define FIFTEEN 0xF
#define DIR 0xF
#define KBD_MASK 0x79



double x1, y1,finish_x=136, finish_y=178, g1_x, g1_y, g2_x, g2_y, x_inc, y_inc, coord_increment = 3.0, LEVEL; // ghost,pacman coordinates, their movement increment, Y level = 1,2,3 or 4

double points[4], points_y[4]; //keep track of points coordinates

bool level_navigation_y_up = true, level_navigation_y_down = true, level_navigation_x_left = true, level_navigation_x_right = true, task_execute = true, GAME_OVER = false, animate = false;

uint8_t LIVES = 3, points_earned = 0, time_user = 0, num_lives = 3;

OS_MUT mut_GLCD; //this mutex makes sure two tasks arent trying to write on the screen at the same time

OS_TID taskid_ghost1, taskid_ghost2, taskid_pacman_joystick, taskid_draw_points, taskid_draw_barriers, taskid_done, taskid_check_lives, taskid_timer, taskid_button, taskid_draw_points2; //task IDs   

uint8_t mut_time_wait = 50; //mutex delay

volatile double time = 0, score = 0;

/***** JOYSTICK OPERATIONS ***/
void JOYSTICK_init(void){
	LPC_PINCON->PINSEL3 &= ~((3<< 8)|(3<<14)|(3<<16)|(3<<18)|(3<<20)); 
	LPC_GPIO1->FIODIR &= ~((1<<20)|(1<<23)|(1<<24)|(1<<25)|(1<<26));
}

uint8_t JOYSTICK_position(void){
	uint8_t kbd_val;
	kbd_val = (LPC_GPIO1->FIOPIN >> 20) & KBD_MASK;
	return kbd_val;	
}

uint8_t JOYSTICK_direction(void){
	uint8_t dir_bits;
	dir_bits = (JOYSTICK_position() >> 3) & DIR; //bit shift the number 3 bits to the right
	return dir_bits;
}

bool UP_dir(void){
	return (JOYSTICK_direction() == 0xE); //14
}

bool DOWN_dir(void){
	return (JOYSTICK_direction() == 0xB ); // 11
}

bool LEFT_dir(void){
	return (JOYSTICK_direction() == 0x7); //7
		
}

bool RIGHT_dir(void){
	return (JOYSTICK_direction() == 0xD);//13
}

/***** DRAW PACMAN ****/
void draw_pacman_task(double x, double y){
	os_mut_wait(mut_GLCD, 0xffff);
	GLCD_Bitmap(x1, y1, 36, 34, (unsigned char*)erase_bitmap);
	os_mut_release(mut_GLCD);
  os_dly_wait (mut_time_wait);
	
	x1 += x;
	y1 += y;
	
	os_mut_wait(mut_GLCD, 0xffff);
	GLCD_Bitmap(x1, y1, 36, 33, (unsigned char*)pacman_bitmap);
	os_mut_release(mut_GLCD);
	os_dly_wait (mut_time_wait);
	//os_tsk_delete_self();
}

/*****RETURN WHICH LEVEL OF Y PACMAN IS IN *****/
double level_y(){
	if ((y1 >=38 && y1 < 45) || (y1 >=19 && y1 < 29)){
		LEVEL = 1;
	}
	
	if ((y1>=85 && y1<115 )){
		LEVEL = 2;
	}
	if (y1 >= 120 && y1 < 128){
		LEVEL = 3;
	} 
	if ((y1 >= 172 && y1 < 190)){
		LEVEL = 4;
	}
	return LEVEL;
}

/***** PUT RESTRICTIONS ON PACMAN'S MOVEMENT BASED ON BARRIERS****/
void pacman_level_check(){
	level_navigation_y_up = false;
	level_navigation_y_down = false;
	level_navigation_x_right = true;
	level_navigation_x_left = true;
	if (level_y() == 1){
		if (x1 > 15 && x1 < 140){
			level_navigation_y_up = false;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 165 && x1 < 210){
			level_navigation_y_up = true;
			if (y1>=18 && y1<25)level_navigation_y_up = false;
			level_navigation_y_down = true;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
			if (x1 >= 200){
				level_navigation_y_up = false;
				level_navigation_y_down = true;
				level_navigation_x_right = false;
				level_navigation_x_left = true;
			}
		}
		else if (x1 >= 225 && x1 < 240){
			level_navigation_y_up = false;
			level_navigation_y_down = true;
			level_navigation_x_right = false;
			level_navigation_x_left = false;
		}
		else {
			level_navigation_y_up = true;
			level_navigation_y_down = true;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}		
	}
	else if (level_y() == 2){
		if (x1 > 15 && x1 < 20){
			level_navigation_y_up = false;
			level_navigation_y_down = true;
			level_navigation_x_right = true;
			level_navigation_x_left = false;
		}
		else if (x1 >= 67 && x1 < 160){
			level_navigation_y_up = false;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 165 && x1 < 220){
			level_navigation_y_up = true;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 220 && x1 < 240){
			level_navigation_y_up = true;
			level_navigation_y_down = true;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
			if (x1>=240){
				level_navigation_x_right = false;
			}
		}
		else {
			level_navigation_y_up = true;
			level_navigation_y_down = true;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
	}
	else if (level_y() == 3){
		if (x1 > 15 && x1 < 25){
			level_navigation_y_up = true;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = false;
		}
		else if (x1 >= 25 && x1 < 50){
			level_navigation_y_up = true;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 50 && x1 < 110){
			level_navigation_y_up = false;
			level_navigation_y_down = true;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 67 && x1 < 150){
			level_navigation_y_up = false;
			level_navigation_y_down = true;
			level_navigation_x_right = false;
			level_navigation_x_left = true;
		}
		else if (x1 >= 150 && x1 < 165){
			level_navigation_y_up = false;
			level_navigation_y_down = true;
			level_navigation_x_right = false;
			level_navigation_x_left = false;
		}
		else if (x1 >= 165 && x1 < 226){
			level_navigation_y_up = false;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 224 && x1 < 240){
			level_navigation_y_up = true;
			level_navigation_y_down = true;
			level_navigation_x_right = false;
			level_navigation_x_left = true;
		}
		else {
			level_navigation_y_up = true;
			level_navigation_y_down = true;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		
	}
	else if (level_y() == 4){
		if (x1 > 15 && x1 < 67){
			level_navigation_y_up = false;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 65 && x1 < 185){
			level_navigation_y_up = true;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 190 && x1 < 220){
			level_navigation_y_up = false;
			level_navigation_y_down = false;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}
		else if (x1 >= 240 && x1 < 340){
			level_navigation_y_up = true;
			level_navigation_y_down = false;
			level_navigation_x_right = false;
			level_navigation_x_left = true;
		}
		else {
			level_navigation_y_up = true;
			level_navigation_y_down = true;
			level_navigation_x_right = true;
			level_navigation_x_left = true;
		}	
	}
}

/*** DRAW WALLS ***/
void draw_barriers_task(void){
	GLCD_SetBackColor(Black);
	GLCD_SetTextColor(White);
  GLCD_DisplayString(0, 0, __FI, "_______PACMAN_______");
  GLCD_DisplayString(1, 0, __FI, "               |    ");
	GLCD_DisplayString(2, 0, __FI, "_________    __|    "); 
	GLCD_DisplayString(3, 0, __FI, "               '    ");
	GLCD_DisplayString(4, 0, __FI, "   _________        ");
	GLCD_DisplayString(5, 0, __FI, "          |         ");
	GLCD_DisplayString(6, 0, __FI, "____      |_____    ");
	GLCD_DisplayString(8, 0, __FI, "____________________");
}


void draw_points_and_finish(void){
	int j = 0, inc=0; // (120,40),(180,80),(240,130),(60,175)
	
		for (j=0;j<4;j++){
			points[j] = 60*(j+2); //create the points array of x values of the points
			if (j == 2){
				inc+=10;
			}
			
			if (j==3){
				points[j] = 60;
				inc+=5;
			}
			
			if (j!=2){
				GLCD_Bitmap(points[j], 40+inc, 13, 13, (unsigned char*)points_bitmap);
				inc = inc + 40;
			}
		}
	
			GLCD_Bitmap(finish_x, finish_y, 18, 18, (unsigned char*)finish_bitmap); //draw the finish block
}

__task void joystick_task(void){
	while(task_execute){
		pacman_level_check();
		if (UP_dir() && level_navigation_y_up){
			draw_pacman_task(0, -coord_increment);
			
		}
		if (DOWN_dir() && level_navigation_y_down){
			draw_pacman_task(0, coord_increment);
		}
		if (LEFT_dir() && level_navigation_x_left){
			draw_pacman_task(-coord_increment, 0);
		}
		if (RIGHT_dir() && level_navigation_x_right){
			draw_pacman_task(coord_increment, 0);
		}
	}
	os_tsk_delete_self();
}

__task void draw_ghost1_task(void){ //ghost that paces up and down

	os_mut_wait(mut_GLCD, 0xffff);
	while(task_execute){
		while (g1_y <= 170){
			os_mut_wait(mut_GLCD, 0xffff);
			GLCD_Bitmap(g1_x, g1_y, 36, 34, (unsigned char*)erase_bitmap);
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
			
			os_mut_wait(mut_GLCD, 0xffff);
			g1_y += coord_increment;
			GLCD_Bitmap(g1_x, g1_y, 36, 33, (unsigned char*)ghost_bitmap);
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
		}
		
		while (g1_y >= 24){
			os_mut_wait(mut_GLCD, 0xffff);
			GLCD_Bitmap(g1_x, g1_y, 36, 34, (unsigned char*)erase_bitmap);
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
			
			g1_y -= coord_increment;
			os_mut_wait(mut_GLCD, 0xffff);
			GLCD_Bitmap(g1_x, g1_y, 36, 33, (unsigned char*)ghost_bitmap);
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
		}
		
		os_mut_release(mut_GLCD);
		os_dly_wait (mut_time_wait);
	}
	os_tsk_delete_self();
}

__task void draw_points_and_finish_task(void){ //draw finish and level 4 point continously to avoid being erased by horizental ghost
	while (task_execute){
		os_mut_wait(mut_GLCD, 0xffff);
		GLCD_Bitmap(finish_x, finish_y, 18, 18, (unsigned char*)finish_bitmap);
		os_mut_release(mut_GLCD);
		os_dly_wait (mut_time_wait);
		
		os_mut_wait(mut_GLCD, 0xffff);
		GLCD_Bitmap(points[3], 175, 13, 13, (unsigned char*)points_bitmap);
		os_mut_release(mut_GLCD);
		os_dly_wait (mut_time_wait);
	}
	os_tsk_delete_self();
}

__task void draw_ghost2_task(void){ //horizental ghost paces right and left
	os_mut_wait(mut_GLCD, 0xffff);
	while(task_execute){
		while (g2_x <= 280){
			os_mut_wait(mut_GLCD, 0xffff);
			GLCD_Bitmap(g2_x, g2_y, 36, 34, (unsigned char*)erase_bitmap);
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
			
			os_mut_wait(mut_GLCD, 0xffff);
			g2_x += coord_increment;
			GLCD_Bitmap(g2_x, g2_y, 36, 33, (unsigned char*)ghost_bitmap);
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
		}
		
		while (g2_x >= 20){
			os_mut_wait(mut_GLCD, 0xffff);
			GLCD_Bitmap(g2_x, g2_y, 36, 34, (unsigned char*)erase_bitmap);
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
			
			g2_x -= coord_increment;
			os_mut_wait(mut_GLCD, 0xffff);
			GLCD_Bitmap(g2_x, g2_y, 36, 33, (unsigned char*)ghost_bitmap);
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
		}
		os_mut_release(mut_GLCD);
		os_dly_wait (mut_time_wait);
	}
	os_tsk_delete_self();
}

__task void check_lives_task(void){
	int p = 0;// (120,40),(180,80),(240,130),(60,175)
	char str[30];
	LED_Out((1 << num_lives ) - 1);
	
	points_y[0] = 40;
	points_y[1] = 80;
	points_y[2] = 130;
	points_y[3] = 175;
	
	while (task_execute){
		for (p=0; p<4; p++){ //check to see if pacman has reached points
			if ((x1>= points[p]-7 && x1<=points[p]+7) && (y1>=points_y[p]-7 && y1<=points_y[p]+7 )){
				points_earned++;
		}
		
		if (points_earned >= 4){
			num_lives = 4;
			LED_Out((1 << num_lives ) - 1);
			
			}	
		}
	
		if (x1>finish_x-30 && x1<finish_x+10 && y1>finish_y-25 && y1<finish_y+25){
			task_execute = false; // finish block hit
		}
		
		if (INT0_Get()==0){ //check for INT0 button
			for (p=0;p<3; p++){
				os_mut_wait(mut_GLCD, 0xffff);
				GLCD_Bitmap(x1, y1, 36, 33, (unsigned char*)pacman_bitmap);
				os_mut_release(mut_GLCD);
				os_dly_wait (mut_time_wait + 500000);
				
				os_mut_wait(mut_GLCD, 0xffff);
				GLCD_Bitmap(x1, y1, 36, 33, (unsigned char*)pacman_bitmap1);
				os_mut_release(mut_GLCD);
				os_dly_wait (mut_time_wait + 500000);
				
				os_mut_wait(mut_GLCD, 0xffff);
				GLCD_Bitmap(x1, y1, 36, 33, (unsigned char*)pacman_bitmap1);
				os_mut_release(mut_GLCD);
				os_dly_wait (mut_time_wait);
			}
		}
		
		if ((x1 >= g1_x - 35 && x1 <= g1_x+35 && y1 >= g1_y-35 && y1<=g1_y+35)
				|| (x1 >= g2_x - 35 && x1 <= g2_x+35 && y1 >= g2_y-35 && y1<=g2_y+35)){
			GAME_OVER = true;
			num_lives--;
			task_execute = false;
		}
	}
	
	os_mut_release(mut_GLCD);
	os_dly_wait (mut_time_wait);
	
	//score
	score = (1000000000*points_earned*2/5)-(os_time_get()*3/5);
	sprintf(str, "Score: %d",((int)(score))%10000);
	
	os_mut_wait(mut_GLCD, 0xffff);
	
	GLCD_Clear(BG);
	GLCD_SetBackColor(Black);
	GLCD_SetTextColor(Red);
	
	if (GAME_OVER){
		GLCD_DisplayString(4, 0, __FI, "   GAME OVER  ");
	} else {
		GLCD_DisplayString(4, 0, __FI, (unsigned char*)str);
	}
	os_mut_release(mut_GLCD);
	os_dly_wait (mut_time_wait);
	
	os_tsk_delete_self();
}

__task void button_task(void){
	while(task_execute){
		LED_Out((1 << 1 ) - 1);
		if (INT0_Get()==0){
			LED_Out((1 << 5));
			os_mut_wait(mut_GLCD, 0xffff);
			GLCD_Clear(BG);
			GLCD_SetBackColor(Black);
			GLCD_SetTextColor(Red);
			task_execute = false;
		
			os_mut_release(mut_GLCD);
			os_dly_wait (mut_time_wait);
		}
	}
	os_tsk_delete_self();
}

__task void base_task(void){
	os_mut_init(mut_GLCD);
	os_tsk_prio_self( 5 );
	
	taskid_ghost1 = os_tsk_create(draw_ghost1_task, 7);
	taskid_ghost2 = os_tsk_create(draw_ghost2_task, 7);
	taskid_draw_points = os_tsk_create(draw_points_and_finish_task, 7);
	taskid_pacman_joystick =  os_tsk_create(joystick_task, 7);
	taskid_check_lives = os_tsk_create(check_lives_task, 7);
	os_tsk_delete_self();
}

int main (void) {
	x1 = 20;
	y1 = 20;
	g1_x = 280;
	g1_y = 25;
	g2_x = 20;
	g2_y = 170;
	
	SystemInit();
  LED_Init();                                /* LED Initialization            */
  SER_Init();                                /* UART Initialization           */
  ADC_Init();                                /* ADC Initialization            */
	GLCD_Init();                               /* Initialize graphical LCD      */
	JOYSTICK_init();
	GLCD_Clear(BG);
	draw_barriers_task();
	draw_points_and_finish();
	os_sys_init(base_task);
	
	
	while (1) {};
}

  
  

