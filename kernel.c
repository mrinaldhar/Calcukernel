#include "keyboard_map.h"

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);

/* current cursor location */
unsigned int current_loc = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char*)0xb8000;


void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	struct IDT_entry{
		unsigned short int offset_lowerbits;
		unsigned short int selector;
		unsigned char zero;
		unsigned char type_attr;
		unsigned short int offset_higherbits;
	};

	struct IDT_entry IDT[IDT_SIZE];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler; 
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;


	/*     Ports
	 *	 PIC1	PIC2
	 *Command 0x20	0xA0
	 *Data	 0x21	0xA1
	 */

	/* ICW1 - begin initialization */
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	 * In x86 protected mode, we have to remap the PICs beyond 0x20 because
	 * Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	 */
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	write_port(0x21 , 0x00);  
	write_port(0xA1 , 0x00);  

	/* ICW4 - environment info */
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(0x21 , 0xFD);
}

void kprint(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		vidptr[current_loc++] = 0xf0;
	}
}

void kprint_newline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
}

void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0xf0; 
	}
}



//Calculator Code Begins


//Variables we need

char bufferkb[100];
int nums[100];
int i=0;
int ind=0;
char bufferout[100];
char bufferk[100];
char stack[100];
int stackind=0;
int priority(char op) {
	if (op=='/')
	{
		return 4;
	}
	else if (op=='*') 
	{
		return 3;
	}
	else if (op=='+')
	{
		return 2;
	}
	else if (op == '-') 
	{
		return 1;
	}
}
char top(void) {
return stack[stackind-1];
}
char pop(void) {
	stackind = stackind - 1;
	return stack[stackind];
}
void push(char c) {
	stack[stackind] = c;
	stackind++;
}
void postfix(char * bufferkb, int i) {
int a, b;
b=0;
char c;
	for(a=0; a<i; a++) {
		if (bufferkb[a] <=57 && bufferkb[a] >= 48) {
		bufferk[b]=bufferkb[a];
		b++;
		}
		else if (bufferkb[a] == '-' || bufferkb[a] == '+' || bufferkb[a] == '*' || bufferkb[a] == '/') {
			
			if (stackind != 0) {
				c=top();
				while (priority(c) > priority(bufferkb[a])) {
					bufferk[b++] = pop();
					c=top();
				}
				push(bufferkb[a]);
			}
			else {
				push(bufferkb[a]);
			}
		}
	}
	while (stackind!=0) {
		bufferk[b++] = pop();
	}
	bufferk[b]='\0';

}

//function that processes the input and output

void process_dmas(char * bufferkb, int i) {
int j=0;
int k=0;
int temp;
int flag = 0;
char c;
postfix(bufferkb, i);
for (j=0; j<i; j++) {
if (bufferk[j] <=57 && bufferk[j] >= 48) {
nums[k] = bufferk[j] - 48;
k++;
}
else if (bufferk[j] == '+') {
nums[k-2] = nums[k-1] + nums[k-2];
k--;
}
else if (bufferk[j] == '*') {
nums[k-2] = nums[k-1] * nums[k-2];
k--;
}
else if (bufferk[j] == '-') {
nums[k-2] = nums[k-2] - nums[k-1];
k--;
}
else if (bufferk[j] == '/') {
nums[k-2] = nums[k-2] / nums[k-1];
k--;
}
}
ind=k;
temp = nums[0];
j=0;
if (temp < 0) {
	temp = -1 * temp;
	flag = 1;
}
while (temp) {

bufferout[j] = (temp%10) + 48;
j++;
temp = temp/10;
}
k=0;
if (flag == 1) {
	bufferout[j] = '-';
	j++;
}
bufferout[j]='\0';
j--;
while (j>k) {
	c= bufferout[j];
	bufferout[j] = bufferout[k];
	bufferout[k]=c;
	k++;
	j--;
}

}



//Function to clear the input and output buffers after each calculation

void clearbuffer(char * buffer) {
int var=0;
for (var=0; var<100; var++) {
buffer[var]='\0';
}
}


// Calculator Code Ends




void keyboard_handler_main(void) {
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;
		vidptr[current_loc++] = keyboard_map[keycode];
		vidptr[current_loc++] = 0xf0;	
		bufferkb[i]=keyboard_map[keycode];
		i++;	
		if (keycode == 28) {
		bufferkb[i+1]='\0';
		i++;
		process_dmas(bufferkb, i);
		kprint(bufferout);
		kprint_newline();	
		//kprint(bufferk); THIS IS THE POSTFIX THAT WE EVALUATED
		//kprint_newline();
		i=0;
		clearbuffer(bufferkb);
		clearbuffer(bufferout);
		}
	}
	
}

void kmain(void)
{
	char *str = "CalcuKernel v0.2";
	clear_screen();
	kprint(str);
	kprint_newline();
	str = "Developed by Mrinal Dhar";
	kprint(str);
	kprint_newline();
	kprint_newline();

	str = "*************************";
	kprint(str);
	kprint_newline();
	kprint_newline();

	str = "Now supports DMAS Calculations with negative value results too!";
	kprint(str);
	kprint_newline();
	kprint_newline();

	str = "Keymap for * and + operators: ";
	kprint(str);
	kprint_newline();
	str = "Left Shift : *";
	kprint(str);
	kprint_newline();
	str = "Right Shift : +";
	kprint(str);
	kprint_newline();
	kprint_newline();

	str = "*************************";
	kprint(str);
	kprint_newline();
	kprint_newline();
	str = "Enter calculation:";
	kprint(str);
	kprint_newline();
	
	kprint_newline();
	idt_init();
	kb_init();

	while(1);
}
