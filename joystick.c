
#define ONE 0x1
#define DIR 0xF
#define KBD_MASK 0x79


void JT_init(void){
	LPC_PINCON->PINSEL3 &= ~((3<< 8)|(3<<14)|(3<<16)|(3<<18)|(3<<20)); 
	LPC_GPIO1->FIODIR &= ~((1<<20)|(1<<23)|(1<<24)|(1<<25)|(1<<26));
}

uint8_t JT_position(void){
	uint8_t kbd_val;
	kbd_val = (LPC_GPIO1->FIOPIN >> 20) & KBD_MASK;
	return kbd_val;	
}

uint8_t JT_direction(void){
	uint8_t dir_bits;
	dir_bits = (JT_position() >> 3) & DIR;
	return dir_bits;
}

int UP_dir(void){
	//return (JT_direction() & 
	if ((JT_direction() & ONE) == 0)
		return 1;
	else
		return 0;
}

int DOWN_dir(void){
	if (((JT_direction() >> 2) & ONE) == 0)
		return 1;
	else
		return 0;
}

int LEFT_dir(void){
	if (((JT_direction() >> 3) & ONE) == 0)
		return 1;
	else
		return 0;
}

int RIGHT_dir(void){
	if (((JT_direction() >> 1) & ONE) == 0)
		return 1;
	else
		return 0;
}

int Start(int a){
	return a;
}

int snake_dir(int c){
	
	if (RIGHT_dir() == 1 && c != 2)
		c = 0;
	if (DOWN_dir() == 1 && c != 3)
		c = 1;
	if (LEFT_dir() == 1 && c != 0)
		c = 2;
	if (UP_dir() == 1 && c!= 1)
		c = 3;
	
	return c;
}

void print_dir(int a){
	if (Start(a) == 1){
		if (UP_dir() == 1){
			GLCD_Clear(White);
			GLCD_DisplayString(0, 0, __FI, "    UP    ");
		}
		if (DOWN_dir() == 1){
			GLCD_Clear(White);
			GLCD_DisplayString(0, 0, __FI, "    DOWN    ");
		}
		if (LEFT_dir() == 1){
			GLCD_Clear(White);
			GLCD_DisplayString(0, 0, __FI, "    LEFT    ");
		}
		if (RIGHT_dir() == 1){
			GLCD_Clear(White);
			GLCD_DisplayString(0, 0, __FI, "    RIGHT    ");	
		}
	}
}
