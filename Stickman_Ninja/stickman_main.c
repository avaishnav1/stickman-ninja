/**
 * Akshati Vaishnav, Grace Lo, Daniela Tran
 * 
 * HARDWARE CONNECTIONS
 *
 * IMU0
 *  - 3V3 (OUT) (#36) → Vin
 *  - GND → GND
 *  - GPIO 12 (#16) → SDA (prev 8)
 *  - GPIO 13 (#17) → SCL (prev 9)
 * Button0
 *  - GPIO 2 (#4) → Left
 *  - GPIO 4 (#6) → Right
 * IMU1
 *  - 3V3 (OUT) (#36) → Vin
 *  - GND → GND
 *  - GPIO 14 (#19) → SDA
 *  - GPIO 15 (#20) → SCL
 * Button1
 *  - GPIO 9 (#12) → Left
 *  - GPIO 11 (#15) → Right
 * VGA
 *  - GND → GND
 *  - GPIO 16 (#21) → Hsync
 *  - GPIO 17 (#22) → Vsync
 *  - GPIO 18 (#24) → 330 ohm resistor → Red
 *  - GPIO 19 (#25) → 330 ohm resistor → Green
 *  - GPIO 20 (#26) → 330 ohm resistor → Blue
 */


// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
// Include PICO libraries
#include "pico/stdlib.h"
#include "pico/multicore.h"
// Include hardware libraries
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
// Include custom libraries
#include "vga_graphics.h"
#include "mpu6050.h"
#include "pt_cornell_rp2040_v1.h"

// Arrays in which raw measurements will be stored
fix15 accel0[3], gyro0[3];
fix15 accel1[3], gyro1[3];

// character array
char screentext[40];

// draw speed
int threshold = 1 ;

float filtered_ax0 = 0.0;
float filtered_ay0 = 0.0;
fix15 accel_angle0 = int2fix15(0);
fix15 gyro_angle_delta0 = int2fix15(0);
fix15 comp_angle0 = int2fix15(0);
float filtered_ax1 = 0.0;
float filtered_ay1 = 0.0;
fix15 accel_angle1 = int2fix15(0);
fix15 gyro_angle_delta1 = int2fix15(0);
fix15 comp_angle1 = int2fix15(0);

// Some macros for max/min/abs
#define min(a,b) ((a<b) ? a:b)
#define max(a,b) ((a<b) ? b:a)
#define abs(a) ((a>0) ? a:-a)

// semaphore
static struct pt_sem vga_semaphore ;

struct player_struct {
	bool player_id; //0 or 1
	short pos_x;
	short pos_y;
	bool stab;
	bool block;
	bool block_prev;
	bool stab_prev;
	int health;
	bool prev_movement; //0 if previous movement was to left, 1 if prev movement was to right
}player0, player1;

// Some paramters for PWM
#define WRAPVAL 5000
#define CLKDIV  25.0
uint slice_num ;

bool move_left0 = 0;
bool move_left1 = 0;
bool move_right0 = 0;
bool move_right1 = 0;

int stab_counter0 = 0;
int stab_counter1 = 0;

char color0 = YELLOW;
char color1 = CYAN;
short game_state = 4; 

/* GAME STATES
0 = player 0 wins
1 = player 1 wins
2 = regular gameplay
3 = wait to restart game and then restart game
4 = start screen
5 = instruction screen
*/
// Interrupt service routine
void sensor_irq() {

    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(5));

    // Read the IMU
	mpu6050_read_raw0(accel0, gyro0);
    mpu6050_read_raw1(accel1, gyro1);
	
	//getting the filtered values for acceleration
	filtered_ax0 += fix2float15((accel0[0]-float2fix15(filtered_ax0))>>6);
	filtered_ay0 += fix2float15((accel0[1]-float2fix15(filtered_ay0))>>6);
	filtered_ax1 += fix2float15((accel1[0]-float2fix15(filtered_ax1))>>6);
	filtered_ay1 += fix2float15((accel1[1]-float2fix15(filtered_ay1))>>6);
	
	//calculating acceleration angle, gyro angle, and complementary angle
	accel_angle0 = multfix15(float2fix15(atan2(-filtered_ax0, -filtered_ay0)), oneeightyoverpi);
	gyro_angle_delta0 = multfix15(gyro0[2], zeropt001);
	comp_angle0 = multfix15(comp_angle0 + gyro_angle_delta0, zeropt999) + multfix15(accel_angle0, zeropt001);
	
	accel_angle1 = multfix15(float2fix15(atan2(-filtered_ax1, -filtered_ay1)), oneeightyoverpi);
	gyro_angle_delta1 = multfix15(gyro1[2], zeropt001);
	comp_angle1 = multfix15(comp_angle1 + gyro_angle_delta1, zeropt999) + multfix15(accel_angle1, zeropt001);
	
	if(gpio_get(2) == 0 && gpio_get(4) == 0) { //if both l+r buttons are pressed, NO MOVEMENT
		move_left0 = 0;
		move_right0 = 0;
	} else if (gpio_get(2) == 0 && gpio_get(4) == 1) { //if l button is pressed, move left
		move_left0 = 1;
		move_right0 = 0;
	} else if (gpio_get(2) == 1 && gpio_get(4) == 0) { //if r button is pressed, move right
		move_left0 = 0;
		move_right0 = 1;
	} else if (gpio_get(2) == 1 && gpio_get(4) == 1) { //if l+r buttons are NOT pressed, NO MOVEMENT
		move_left0 = 0;
		move_right0 = 0;
	}
	
	if(gpio_get(9) == 0 && gpio_get(11) == 0) { //if both l+r buttons are pressed, NO MOVEMENT
		move_left1 = 0;
		move_right1 = 0;
	} else if (gpio_get(9) == 0 && gpio_get(11) == 1) { //if l button is pressed, move left
		move_left1 = 1;
		move_right1 = 0;
	} else if (gpio_get(9) == 1 && gpio_get(11) == 0) { //if r button is pressed, move right
		move_left1 = 0;
		move_right1 = 1;
	} else if (gpio_get(9) == 1 && gpio_get(11) == 1) { //if l+r buttons are NOT pressed, NO MOVEMENT
		move_left1 = 0;
		move_right1 = 0;
	}
	
	if(player0.stab == 1) {
		stab_counter0 += 1;
	} 
	
	if(player1.stab == 1) {
		stab_counter1 += 1;
	} 

    // Signal VGA to draw
    PT_SEM_SIGNAL(pt, &vga_semaphore);
}

//animation thread
static PT_THREAD (protothread_anim1(struct pt *pt)) {
    // Mark beginning of thread
    PT_BEGIN(pt);
    // Variables for maintaining frame rate
	
	player0.player_id = 0;
	player0.pos_x = (140);
	player0.pos_y = (345);
	player0.stab = 0;
	player0.block = 0;
	player0.block_prev = 0;
	player0.stab_prev = 0;
	player0.health = 100;
	player0.prev_movement = 1; //player0 initially faces player1 -- so player0 faces right
	
	player1.player_id = 1;
	player1.pos_x = (500);
	player1.pos_y = (345);
	player1.stab = 0;
	player1.block = 0;
	player1.block_prev = 0;
	player1.stab_prev = 0;
	player1.health = 100;
	player1.prev_movement = 0; //player1 initially faces player0 -- so player1 faces left
	
	int prev_health0 = player0.health;
	int prev_health1 = player1.health;
	static char health0_print[40];
	static char health1_print[40];
	static char winner_print[40];
	static char restart_print[40];
	static char start_print0[40];
	static char start_print1[40];
	static char start_print2[40];
	static char instruction_print[100];
	
	PT_YIELD_usec(100000);
	
    while(1) {
		if(game_state == 0) {
			setTextColor2(WHITE, BLACK); 
			setCursor(200, 200); 
			setTextSize(3);
			sprintf(winner_print, "Player 0 Wins!");
			writeString(winner_print);
			PT_YIELD_usec(1000000);
			game_state = 3;
		} else if(game_state == 1) {
			setTextColor2(WHITE, BLACK); 
			setCursor(200, 200); 
			setTextSize(3);
			sprintf(winner_print, "Player 1 Wins!");
			writeString(winner_print);
			PT_YIELD_usec(1000000);
			game_state = 3;
		} else if(game_state == 2) {
			
			//horizontal line (ground)
			drawHLine(0, 420, 640, WHITE);
			
			//health bar text
			setTextSize(1);
			setTextColor2(color0, BLACK); 
			setCursor(247, 43); 
			sprintf(health0_print, "%d", player0.health);
			writeString(health0_print);
			
			setTextColor2(color1, BLACK); 
			setCursor(557, 43); 
			sprintf(health1_print, "%d", player1.health);
			writeString(health1_print);
			
			if((prev_health0 == 100 && player0.health < 100)){
				fillRect(259, 43, 6, 8, BLACK);
			} 
			if (prev_health0 == 10 && player0.health < 10) {
				fillRect(253, 43, 6, 8, BLACK);
			}
			if(prev_health1 == 100 && player1.health < 100){
				fillRect(569, 43, 6, 8, BLACK);
			} 
			if (prev_health1 == 10 && player1.health < 10) {
				fillRect(563, 43, 6, 8, BLACK);
			}
			
			
			/* PLAYER 0 */		
			//stabbing logic -- player0
			if((fix2int15(comp_angle0) >= 75) && (fix2int15(comp_angle0) <= 105) && (fix2float15(accel0[1]) > 0.01)) {
				if (stab_counter0 < 500) {
					player0.stab = 1;
				} else {
					player0.stab = 0;
					stab_counter0 = 0;
				}
				player0.block = 0;
			} else if ((fix2int15(comp_angle0) >= -15) && (fix2int15(comp_angle0) <= 15)) {
				player0.block = 1;
				player0.stab = 0;
			} else {
				player0.block = 0;
				player0.stab = 0;
			}
			
			//moving logic -- player0
			if(move_left0 == 1 && move_right0 == 0 && player0.pos_x-84-5 >= 0) { //last check is for VGA walls
				fillRect(player0.pos_x-84, player0.pos_y-60, 168, 135, BLACK);
				player0.pos_x -= 5;
				if(player0.prev_movement == 1) { //player0 was moving right, but now its moving left...
					player0.prev_movement = 0;
				}
			} else if(move_left0 == 0 && move_right0 == 1 && player0.pos_x+84+5 <= 639) { //last check is for VGA walls
				fillRect(player0.pos_x-84, player0.pos_y-60, 168, 135, BLACK);
				player0.pos_x += 5;
				if(player0.prev_movement == 0) { //player0 was moving right, but now its moving left...
					player0.prev_movement = 1;
				}
			}
			
			//constant body features -- player0
			drawCircle(player0.pos_x, player0.pos_y-30, 15, color0); //head circle
			drawVLine(player0.pos_x, player0.pos_y-15, 45, color0); //body line
			
			if(player0.prev_movement == 0) { //move left
				//legs
				drawLine(player0.pos_x, player0.pos_y+30, player0.pos_x-15, player0.pos_y+53, color0); //left leg1
				drawVLine(player0.pos_x-15, player0.pos_y+53, 22, color0); //left leg2
				drawLine(player0.pos_x, player0.pos_y+30, player0.pos_x+6, player0.pos_y+53, color0);//right leg1
				drawLine(player0.pos_x+6, player0.pos_y+53, player0.pos_x+15, player0.pos_y+75, color0);//right leg2
				
				//arms
				drawLine(player0.pos_x, player0.pos_y, player0.pos_x+15, player0.pos_y+30, color0); //right arm
				if(player0.block == 0 && player0.stab == 1){ //player0 stab!
					drawLine(player0.pos_x, player0.pos_y, player0.pos_x-30, player0.pos_y+3, color0); //left arm
					drawHLine(player0.pos_x-84, player0.pos_y+3, 60, color0); //sword1 --long part
					drawVLine(player0.pos_x-30, player0.pos_y, 6, color0); //sword2 --short part
				} else if(player0.block == 1 && player0.stab == 0){ //player0 block!
					drawLine(player0.pos_x, player0.pos_y, player0.pos_x-11, player0.pos_y+23, color0); //left arm1
					drawLine(player0.pos_x-11, player0.pos_y+23, player0.pos_x-24, player0.pos_y+15, color0); //left arm2
					drawVLine(player0.pos_x-24, player0.pos_y-37, 60, color0); //sword1
					drawHLine(player0.pos_x-29, player0.pos_y+12, 10, color0); //sword2
				} else { //player0 wait state!
					drawLine(player0.pos_x, player0.pos_y, player0.pos_x-9, player0.pos_y+12, color0); //left arm1
					drawLine(player0.pos_x-9, player0.pos_y+12, player0.pos_x-30, player0.pos_y, color0); //left arm2
					drawLine(player0.pos_x-24, player0.pos_y+6, player0.pos_x-69, player0.pos_y-39, color0); //sword1
					drawLine(player0.pos_x-29, player0.pos_y-5, player0.pos_x-35, player0.pos_y+2, color0); //sword2
				}
				
			} else if(player0.prev_movement == 1) { //move right
				
				//legs
				drawLine(player0.pos_x, player0.pos_y+30, player0.pos_x-6, player0.pos_y+53, color0);//left leg1
				drawLine(player0.pos_x-6, player0.pos_y+53, player0.pos_x-15, player0.pos_y+75, color0);//left leg2
				drawLine(player0.pos_x, player0.pos_y+30, player0.pos_x+15, player0.pos_y+53, color0); //right leg1
				drawVLine(player0.pos_x+15, player0.pos_y+53, 22, color0); //right leg2
				
				//arms
				drawLine(player0.pos_x, player0.pos_y, player0.pos_x-15, player0.pos_y+30, color0); //left arm
				if(player0.block == 0 && player0.stab == 1){ //player0 stab!
					drawLine(player0.pos_x, player0.pos_y, player0.pos_x+30, player0.pos_y+3, color0); //right arm
					drawHLine(player0.pos_x+24, player0.pos_y+3, 60, color0); //sword1 --long part
					drawVLine(player0.pos_x+30, player0.pos_y, 6, color0); //sword2 --short part
				} else if(player0.block == 1 && player0.stab == 0){ //player0 block!
					drawLine(player0.pos_x, player0.pos_y, player0.pos_x+11, player0.pos_y+23, color0); //right arm1
					drawLine(player0.pos_x+11, player0.pos_y+23, player0.pos_x+24, player0.pos_y+15, color0); //right arm2
					drawVLine(player0.pos_x+24, player0.pos_y-37, 60, color0); //sword1
					drawHLine(player0.pos_x+19, player0.pos_y+12, 10, color0); //sword2
				} else { //player0 wait state!
					drawLine(player0.pos_x, player0.pos_y, player0.pos_x+9, player0.pos_y+12, color0); //right arm1
					drawLine(player0.pos_x+9, player0.pos_y+12, player0.pos_x+30, player0.pos_y, color0); //right arm2
					drawLine(player0.pos_x+24, player0.pos_y+6, player0.pos_x+69, player0.pos_y-39, color0); //sword1
					drawLine(player0.pos_x+29, player0.pos_y-5, player0.pos_x+35, player0.pos_y+2, color0); //sword2
				}
			}
			
			/* PLAYER 1 */
			//stabbing logic -- player1
			if((fix2int15(comp_angle1) >= 75) && (fix2int15(comp_angle1) <= 105) && (fix2float15(accel1[1]) > 0.01)) {
				if (stab_counter1 < 500) {
					player1.stab = 1;
				} else {
					player1.stab = 0;
					stab_counter1 = 0;
				}
				player1.block = 0;
			} else if ((fix2int15(comp_angle1) >= -15) && (fix2int15(comp_angle1) <= 15)) {
				player1.block = 1;
				player1.stab = 0;
			} else {
				player1.block = 0;
				player1.stab = 0;
			}
			
			//moving logic -- player1
			if(move_left1 == 1 && move_right1 == 0 && player1.pos_x-84-5 >= 0) { //VGA box
				fillRect(player1.pos_x-84, player1.pos_y-60, 168, 135, BLACK);
				player1.pos_x -= 5;
				if(player1.prev_movement == 1) { //player1 was moving right, but now its moving left...
					player1.prev_movement = 0;
				}
			} else if(move_left1 == 0 && move_right1 == 1 && player1.pos_x+84+5 <= 639) { //VGA box
				fillRect(player1.pos_x-84, player1.pos_y-60, 168, 135, BLACK);
				player1.pos_x += 5;
				if(player1.prev_movement == 0) { //player1 was moving left, but now its moving right...
					player1.prev_movement = 1;
				}
			}
			
			//constant body features -- player1
			drawCircle(player1.pos_x, player1.pos_y-30, 15, color1); //head circle
			drawVLine(player1.pos_x, player1.pos_y-15, 45, color1); //body line
			
			if(player1.prev_movement == 0) {//move left
			
				//legs
				drawLine(player1.pos_x, player1.pos_y+30, player1.pos_x-15, player1.pos_y+53, color1); //left leg1
				drawVLine(player1.pos_x-15, player1.pos_y+53, 22, color1); //left leg2
				drawLine(player1.pos_x, player1.pos_y+30, player1.pos_x+6, player1.pos_y+53, color1);//right leg1
				drawLine(player1.pos_x+6, player1.pos_y+53, player1.pos_x+15, player1.pos_y+75, color1);//right leg2
				
				//arms
				drawLine(player1.pos_x, player1.pos_y, player1.pos_x+15, player1.pos_y+30, color1); //right arm
				if(player1.block == 0 && player1.stab == 1){ //player1 stab!
					drawLine(player1.pos_x, player1.pos_y, player1.pos_x-30, player1.pos_y+3, color1); //left arm
					drawHLine(player1.pos_x-84, player1.pos_y+3, 60, color1); //sword1 --long part
					drawVLine(player1.pos_x-30, player1.pos_y, 6, color1); //sword2 --short part
				} else if(player1.block == 1 && player1.stab == 0){ //player1 block!
					drawLine(player1.pos_x, player1.pos_y, player1.pos_x-11, player1.pos_y+23, color1); //left arm1
					drawLine(player1.pos_x-11, player1.pos_y+23, player1.pos_x-24, player1.pos_y+15, color1); //left arm2
					drawVLine(player1.pos_x-24, player1.pos_y-37, 60, color1); //sword1
					drawHLine(player1.pos_x-29, player1.pos_y+12, 10, color1); //sword2
				} else { //player1 wait state!
					drawLine(player1.pos_x, player1.pos_y, player1.pos_x-9, player1.pos_y+12, color1); //left arm1
					drawLine(player1.pos_x-9, player1.pos_y+12, player1.pos_x-30, player1.pos_y, color1); //left arm2
					drawLine(player1.pos_x-24, player1.pos_y+6, player1.pos_x-69, player1.pos_y-39, color1); //sword1
					drawLine(player1.pos_x-29, player1.pos_y-5, player1.pos_x-35, player1.pos_y+2, color1); //sword2
				}
				
			} else if(player1.prev_movement == 1) {//move right
			
				//legs
				drawLine(player1.pos_x, player1.pos_y+30, player1.pos_x-6, player1.pos_y+53, color1);//left leg1
				drawLine(player1.pos_x-6, player1.pos_y+53, player1.pos_x-15, player1.pos_y+75, color1);//left leg2
				drawLine(player1.pos_x, player1.pos_y+30, player1.pos_x+15, player1.pos_y+53, color1); //right leg1
				drawVLine(player1.pos_x+15, player1.pos_y+53, 22, color1); //right leg2
				
				//arms
				drawLine(player1.pos_x, player1.pos_y, player1.pos_x-15, player1.pos_y+30, color1); //left arm
				if(player1.block == 0 && player1.stab == 1){ //player1 stab!
					drawLine(player1.pos_x, player1.pos_y, player1.pos_x+30, player1.pos_y+3, color1); //right arm
					drawHLine(player1.pos_x+24, player1.pos_y+3, 60, color1); //sword1 --long part
					drawVLine(player1.pos_x+30, player1.pos_y, 6, color1); //sword2 --short part
				} else if(player1.block == 1 && player1.stab == 0){ //player1 block!
					drawLine(player1.pos_x, player1.pos_y, player1.pos_x+11, player1.pos_y+23, color1); //right arm1
					drawLine(player1.pos_x+11, player1.pos_y+23, player1.pos_x+24, player1.pos_y+15, color1); //right arm2
					drawVLine(player1.pos_x+24, player1.pos_y-37, 60, color1); //sword1
					drawHLine(player1.pos_x+19, player1.pos_y+12, 10, color1); //sword2
				} else { //player1 wait state!
					drawLine(player1.pos_x, player1.pos_y, player1.pos_x+9, player1.pos_y+12, color1); //right arm1
					drawLine(player1.pos_x+9, player1.pos_y+12, player1.pos_x+30, player1.pos_y, color1); //right arm2
					drawLine(player1.pos_x+24, player1.pos_y+6, player1.pos_x+69, player1.pos_y-39, color1); //sword1
					drawLine(player1.pos_x+29, player1.pos_y-5, player1.pos_x+35, player1.pos_y+2, color1); //sword2
				}
				
			}
			
			//prev movement is: 0 if move left, 1 if move right
			if(player0.block != player0.block_prev || player0.stab != player0.stab_prev || player1.block != player1.block_prev || player1.stab != player1.stab_prev) {
				fillRect(player0.pos_x-84, player0.pos_y-60, 168, 135, BLACK); //black box for stabbing changes
				fillRect(player1.pos_x-84, player1.pos_y-60, 168, 135, BLACK); //black box for stabbing changes
				if(player0.pos_x < player1.pos_x) { //player0 is to the left of player1
					if(player0.prev_movement == 1 && player1.prev_movement == 0) { //face each other
						//player0 faces right, player1 faces left
						if(player0.stab == 1 && player1.block == 0 && ((player1.pos_x - player0.pos_x)>=30) && ((player1.pos_x - player0.pos_x)<=114)) { // player0 stabbing player1
							prev_health1 = player1.health;
							player1.health -= 1;
							
							//health bar logic
							fillRect(351+(player1.health*2), 43, 2, 8, BLACK);
						}
						if(player1.stab == 1 && player0.block == 0 && ((player1.pos_x - player0.pos_x)>=30) && ((player1.pos_x - player0.pos_x)<=114)) { // player1 stabbing player0
							prev_health0 = player0.health;
							player0.health -= 1;
							
							//health bar logic
							fillRect(41+(player0.health*2), 43, 2, 8, BLACK);
						}
						
					} else if(player0.prev_movement == 0 && player1.prev_movement == 1) { //face away from each other
						//player0 faces left, player1 faces right
						//do nothing because they aren't facing each other and player0 is to the left of player1
						continue;
					} else if(player0.prev_movement == 1 && player1.prev_movement == 1) { //p0 faces p1, p1 looks away from p0
						//player0 faces right, player1 faces right => player1's blocks do not matter! also player1 cannot stab player0
						if(player0.stab == 1 && ((player1.pos_x - player0.pos_x)>=30) && ((player1.pos_x - player0.pos_x)<=84)) { // player0 stabbing player1
							prev_health1 = player1.health;
							player1.health -= 1;
							
							//health bar logic
							fillRect(351+(player1.health*2), 43, 2, 8, BLACK);
						}
					} else if(player0.prev_movement == 0 && player1.prev_movement == 0) { //p1 faces p0, p0 looks away from p1
						//player0 faces left, player1 faces left => player0's blocks do not matter! also player0 cannot stab player1
						if(player1.stab == 1 && ((player1.pos_x - player0.pos_x)>=30) && ((player1.pos_x - player0.pos_x)<=84)) { // player1 stabbing player0
							prev_health0 = player0.health;
							player0.health -= 1;
							
							//health bar logic
							fillRect(41+(player0.health*2), 43, 2, 8, BLACK);
						}
					}
				} else if(player0.pos_x > player1.pos_x) { //player0 is to the right of player1
					if(player0.prev_movement == 1 && player1.prev_movement == 0) { //face away from each other
						//player0 faces right, player1 faces left
						//do nothing because they aren't facing each other and player1 is to the left of player0
					} else if(player0.prev_movement == 0 && player1.prev_movement == 1) { //face each other
						//player0 faces left, player1 faces right
						if(player0.stab == 1 && player1.block == 0 && ((player0.pos_x - player1.pos_x)>=30) && ((player0.pos_x - player1.pos_x)<=114)) { // player0 stabbing player1
							prev_health1 = player1.health;
							player1.health -= 1;
							
							//health bar logic
							fillRect(351+(player1.health*2), 43, 2, 8, BLACK);
						}
						if(player1.stab == 1 && player0.block == 0 && ((player0.pos_x - player1.pos_x)>=30) && ((player0.pos_x - player1.pos_x)<=114)) { // player1 stabbing player0
							prev_health0 = player0.health;
							player0.health -= 1;
							
							//health bar logic
							fillRect(41+(player0.health*2), 43, 2, 8, BLACK);
						}
					} else if(player0.prev_movement == 1 && player1.prev_movement == 1) { //p1 faces p0, p0 looks away from p1
						//player0 faces right, player1 faces right => player0's blocks do not matter! also player0 cannot stab player1
						if(player1.stab == 1 && player1.block == 0 && ((player0.pos_x - player1.pos_x)>=30) && ((player0.pos_x - player1.pos_x)<=84)) { // player1 stabbing player0
							prev_health0 = player0.health;
							player0.health -= 1;
							
							//health bar logic
							fillRect(41+(player0.health*2), 43, 2, 8, BLACK);
						}
					} else if(player0.prev_movement == 0 && player1.prev_movement == 0) { //p0 faces p1, p1 looks away from p0
						//player0 faces left, player1 faces left => player1's blocks do not matter! also player1 cannot stab player0
						if(player0.stab == 1 && player0.block == 0 && ((player0.pos_x - player1.pos_x)>=30) && ((player0.pos_x - player1.pos_x)<=84)) { // player0 stabbing player1
							prev_health1 = player1.health;
							player1.health -= 1;
							
							//health bar logic
							fillRect(351+(player1.health*2), 43, 2, 8, BLACK);
						}
					}
				}
			}
			player0.block_prev = player0.block;
			player0.stab_prev = player0.stab;
			player1.block_prev = player1.block;
			player1.stab_prev = player1.stab;
			
			
			//game state changes
			if(player0.health < 0) {
				game_state = 1; //player1 wins
				drawRect(40, 42, 202, 10, color0);
			} else if (player1.health < 0) {
				game_state = 0; //player0 wins
				drawRect(350, 42, 202, 10, color1);
			}
			
		} else if (game_state == 3) {
			setTextSize(2);
			setTextColor2(WHITE, BLACK); 
			setCursor(180, 350); 
			sprintf(restart_print, "Press button to restart!");
			writeString(restart_print);
			if(gpio_get(8) == 0) {
				PT_YIELD_usec(100000);
				player0.player_id = 0;
				player0.pos_x = (140);
				player0.pos_y = (345);
				player0.stab = 0;
				player0.block = 0;
				player0.block_prev = 0;
				player0.stab_prev = 0;
				player0.health = 100;
				
				player1.player_id = 1;
				player1.pos_x = (500);
				player1.pos_y = (345);
				player1.stab = 0;
				player1.block = 0;
				player1.block_prev = 0;
				player1.stab_prev = 0;
				player1.health = 100;
				
				int prev_health0 = player0.health;
				int prev_health1 = player1.health;
				
				fillRect(0, 0, 640, 480, BLACK);
				
				//health bar
				fillRect(40, 42, 202, 10, color0);
				drawRect(38, 40, 206, 14, color0);
				fillRect(350, 42, 202, 10, color1);
				drawRect(348, 40, 206, 14, color1);
				
				game_state = 2;
			}
		} else if (game_state == 4) { //start screen			
			//title print
			setTextSize(5);
			setTextColor2(RED, BLACK); 
			setCursor(190, 150); 
			sprintf(start_print0, "STICKMAN");
			writeString(start_print0);
			setCursor(235, 200); 
			sprintf(start_print1, "NINJA");
			writeString(start_print1);
			setTextSize(2);
			setCursor(150, 350); 
			sprintf(start_print2, "Press button to continue...");
			writeString(start_print2);
			
			if(gpio_get(8) == 0) {
				PT_YIELD_usec(1000000);
				game_state = 5;
				fillRect(0, 0, 640, 480, BLACK);
			}
		} else if (game_state == 5) { //instruction screen
			//print instructions!!!
			setTextSize(4);
			setTextColor2(GREEN, BLACK); 
			setCursor(175, 50); 
			sprintf(instruction_print, "INSTRUCTIONS");
			writeString(instruction_print);
			/* Instruction Text:
			To move:
				Move left: press left button
				Move right: press right button 
				
			To use the sword:
				Make sure the IMU faces right!
				To stab: hold the board horizontally and move in a stabbing motion 
				To block: hold the board vertically
			*/
			setTextColor2(WHITE, BLACK); 
			setTextSize(3);
			setCursor(50, 110); 
			sprintf(instruction_print, "To move...");
			writeString(instruction_print);
			setTextSize(2);
			setCursor(70, 140); 
			sprintf(instruction_print, "Move left: press left button");
			writeString(instruction_print);
			setCursor(70, 160); 
			sprintf(instruction_print, "Move right: press right button");
			writeString(instruction_print);
			
			setTextSize(3);
			setCursor(50, 210); 
			sprintf(instruction_print, "To use the sword...");
			writeString(instruction_print);
			setTextSize(2);
			setCursor(70, 240); 
			sprintf(instruction_print, "Make sure the IMU faces right!");
			writeString(instruction_print);
			setCursor(70, 260); 
			sprintf(instruction_print, "To stab: hold the board horizontally and ");
			writeString(instruction_print);
			setCursor(70, 280); 
			sprintf(instruction_print, "move in a stabbing motion");
			writeString(instruction_print);
			setCursor(70, 300); 
			sprintf(instruction_print, "To block: hold the board vertically");
			writeString(instruction_print);
			
			if(gpio_get(8) == 0) {
				PT_YIELD_usec(100000);
				game_state = 2;
				fillRect(0, 0, 640, 480, BLACK);
				//health bar
				fillRect(40, 42, 202, 10, color0);
				drawRect(38, 40, 206, 14, color0);
				fillRect(350, 42, 202, 10, color1);
				drawRect(348, 40, 206, 14, color1);
			}
		}
		// NEVER exit while
    } // END WHILE(1)
  PT_END(pt);
} // animation thread


// Entry point for core 1
void core1_entry() {
	pt_add_thread(protothread_anim1);
    pt_schedule_start ;
}

int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize VGA
    initVGA() ;
	
	//button config
	//left
	gpio_init(2) ;
	gpio_set_dir(2, GPIO_IN);
	gpio_pull_up(2);

	//right
	gpio_init(4) ;
	gpio_set_dir(4, GPIO_IN);
	gpio_pull_up(4);
	
	//left
	gpio_init(9) ;
	gpio_set_dir(9, GPIO_IN);
	gpio_pull_up(9);
	
	//right
	gpio_init(11) ;
	gpio_set_dir(11, GPIO_IN);
	gpio_pull_up(11);
	
	//restart button
	gpio_init(8) ;
	gpio_set_dir(8, GPIO_IN);
	gpio_pull_up(8);

    ////////////////////////////////////////////////////////////////////////
    ///////////////////////// I2C CONFIGURATION ////////////////////////////
    i2c_init(I2C_CHAN0, 200000) ;
    gpio_set_function(SDA_PIN0, GPIO_FUNC_I2C) ;
    gpio_set_function(SCL_PIN0, GPIO_FUNC_I2C) ;
    gpio_pull_up(SDA_PIN0) ;
    gpio_pull_up(SCL_PIN0) ;
	
	i2c_init(I2C_CHAN1, 200000) ;
	gpio_set_function(SDA_PIN1, GPIO_FUNC_I2C) ;
    gpio_set_function(SCL_PIN1, GPIO_FUNC_I2C) ;
    gpio_pull_up(SDA_PIN1) ;
    gpio_pull_up(SCL_PIN1) ;

    // MPU6050 initialization
    mpu6050_reset();
	mpu6050_read_raw0(accel0, gyro0);
    mpu6050_read_raw1(accel1, gyro1);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, sensor_irq);
    irq_set_enabled(PWM_IRQ_WRAP, true);
	
	// This section configures the period of the PWM signals
    pwm_set_wrap(slice_num, WRAPVAL) ;
    pwm_set_clkdiv(slice_num, CLKDIV) ;

    // This sets duty cycle
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 0);

    // Start the channel
    pwm_set_mask_enabled((1u << slice_num));


    ////////////////////////////////////////////////////////////////////////
    ///////////////////////////// ROCK AND ROLL ////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    // start core 1 
    multicore_reset_core1();
    multicore_launch_core1(core1_entry);

    // start core 0
    pt_schedule_start ;

}
