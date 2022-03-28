#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <mem.h>
#include <conio.h>
#undef outp

#define ADLIB_PORT			0x388
#define SC_INDEX            0x03c4    // VGA sequence controller
#define SC_DATA             0x03c5

#define GC_INDEX            0x03ce    // VGA graphics controller
#define GC_DATA             0x03cf

#define MAP_MASK            0x02
#define ALL_PLANES          0xff02
#define MEMORY_MODE         0x04
#define CRTC_DATA           0x03d5

#define VIDEO_INT           0x10      // the BIOS video interrupt.
#define SET_MODE            0x00      // BIOS func to set the video mode.
#define VGA_256_COLOR_MODE  0x13      // use to set 256-color mode. 
#define TEXT_MODE           0x03      // use to set 80x25 text mode. 

#define SCREEN_WIDTH        320       // width in pixels of mode 0x13
#define SCREEN_HEIGHT       240       // height in pixels of mode 0x13
#define NUM_COLORS          256       // number of colors in mode 0x13

#define AC_HPP              0X20 | 0X13    // Horizontal Pel Panning Register

#define DE_MASK				0x01     //display enable bit in status register 1

#define MISC_OUTPUT         0x03c2    // VGA misc. output register
#define S_MEMORY_MODE		0x03C4	
#define INPUT_STATUS_0		0x03da
#define AC_MODE_CONTROL		0x10	  //Index of Mode COntrol register in AC
#define AC_INDEX			0x03c0	  //Attribute controller index register
#define CRTC_INDEX			0x03d4	  // CRT Controller Index
#define H_TOTAL             0x00      // CRT controller registers
#define H_DISPLAY_END       0x01
#define H_BLANK_START       0x02
#define H_BLANK_END         0x03
#define H_RETRACE_START     0x04
#define H_RETRACE_END       0x05
#define V_TOTAL             0x06
#define OVERFLOW            0x07
#define PRESET_ROW_SCAN     0x08 
#define MAX_SCAN_LINE       0x09
#define V_RETRACE_START     0x10
#define V_RETRACE_END       0x11
#define V_DISPLAY_END       0x12
#define OFFSET              0x13
#define UNDERLINE_LOCATION  0x14
#define V_BLANK_START       0x15
#define V_BLANK_END         0x16
#define MODE_CONTROL        0x17
#define LINE_COMPARE		0x18 

/* macro to write a word to a port */
#define word_out(port,register,value) \
  outport(port,(((word)value<<8) + register))

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

byte EGA = 0;
byte *EGA_MODE = (byte *) 0x00000488; //Initial EGA mode switches address
byte *VGA=(byte *)0xA0000000L;  // Tile data address
byte *VRAM_MAP =(byte *)0xB8000000L; //Map address
byte *TileData;

extern unsigned char sdata1[];
extern unsigned char font[];
extern unsigned char sprite_question[];
extern unsigned char sprite_moon[];
extern unsigned char sprite_ghost[];
extern unsigned char sine[];
extern unsigned char intro_palette[];
extern unsigned char twister_palette[];
extern unsigned char parallax_palette[];
extern unsigned char cal_map[];
extern unsigned char bios_map[];
extern unsigned char intro_anim[];
extern unsigned char intro_map[];
extern int logo_wave[];
extern unsigned char scroll_intro_map[];
extern char greetings_wave_y[];
extern unsigned char small_scroll_bar[];
extern unsigned char copper_bar[];
extern unsigned short copper_fix_pos[];
extern unsigned char copper_wave[];
extern unsigned char heart_animation[];
extern unsigned char Parallax_Animation[];
extern unsigned char twister_map[];
extern unsigned char twister_text[];
extern unsigned char twister_animation_map[];
extern unsigned short twister_twist[];
extern unsigned char text_3d[];
extern unsigned char PC_3d[];
extern unsigned char parallax_map[];
extern unsigned char parallax_tiles[];
extern unsigned char Random_Fade[];

byte intro_palette_EGA[16] = {0,8,2,10,5,9,9,9,9,9,9,13,13,2,12,15};
byte twister_palette_EGA[16] = {0,1,1,8,9,7,14,1,8,1,9,7,14,15,14,9};
byte parallax_palette_EGA[16] = {0,1,9,3,11,11,11,2,7,7,7,14,12,5,13,15};


byte SpBKG[32*8];
byte spr_init[8] = {0,0,0,0,0,0,0,0};
int last_sx[8];
int last_sy[8];

char key = 0;

int Split_Value = 128;
int Scene = 0;
int line = 0;
unsigned char pixel = 0;
unsigned char text_mode_panning[] = {8, 0, 1, 2, 3, 4, 5, 6, 7};//Avoid 8 when scrolling on real VGA
byte panning = 0;
byte static_screen = 1;
word imfwait = 0;
word music_offset = 0;
word loop = 0;
word greetings_offset = 0;
word pmap_offset = 40*8;
byte copper_shade = 0;
byte r = 63;
byte g = 0;
byte b = 0;
int cycle_color_state = 0;
byte bar_shade[] = {0,16};
byte copper_cycle_color = 0;
void interrupt (*old_time_handler)(void);

void Clearkb(){
	asm mov ah,00ch
	asm mov al,0
	asm int 21h
}

void opl2_out(unsigned char reg, unsigned char data){
	asm mov dx, 00388h
	asm mov al, reg
	asm out dx, al
	
	//Wait at least 3.3 microseconds
	asm mov cx,6
	wait:
		asm in ax,dx 
		asm loop wait
	
	asm inc dx
	asm mov al, data
	asm out dx, al
	
	//Wait at least 23 microseconds??
	/*asm mov cx,40
	wait2:
		asm in ax,dx
		asm loop wait2*/
}

void opl2_clear(void){
	int i;
    for (i=0; i< 256; opl2_out(i++, 0));    //clear all registers
}

int Adlib_Check(){ 
    int status1, status2, i, Adlib_Detect;
    opl2_out(4, 0x60);
    opl2_out(4, 0x80);
    status1 = inp(ADLIB_PORT);
   
    opl2_out(2, 0xFF);
    opl2_out(4, 0x21);
    for (i=100; i>0; i--) inp(ADLIB_PORT);
    status2 = inp(ADLIB_PORT);
    
    opl2_out(4, 0x60);
    opl2_out(4, 0x80);
    if ( ( ( status1 & 0xe0 ) == 0x00 ) && ( ( status2 & 0xe0 ) == 0xc0 ) ){
		opl2_clear();    //clear all registers
		opl2_out(1, 0x20);  // Set WSE=1
		Adlib_Detect = 1;
    } else Adlib_Detect = 0;
	
	return Adlib_Detect;
}

void Play_Music(){
	while (!imfwait){
        imfwait = sdata1[music_offset+2];
        opl2_out(sdata1[music_offset], sdata1[music_offset+1]);
		music_offset+=3;
		if (music_offset == 0xBB17) {music_offset = 0; imfwait = 1;}
	}
	imfwait--;
}

int Check_Graphics(){
	int VGAEGA = 0;
	union REGS regs;
	regs.h.ah=0x1A;
	regs.h.al=0x00;
	regs.h.bl=0x32;
	int86(0x10, &regs, &regs);
	if (regs.h.al == 0x1A) {VGAEGA = 1; EGA = 0;printf("VGA detected\n");sleep(2);}	//VGA
	else {
		regs.h.ah=0x12;
		regs.h.bh=0x10;
		regs.h.bl=0x10;
		int86(0x10, &regs, &regs);
		if (regs.h.bh == 0) {VGAEGA = 2; EGA = 1;printf("EGA detected\n");sleep(2);}	//EGA
	}
	return VGAEGA;
}

void Exit_Demo(){
	union REGS regs;
	opl2_clear();
	regs.h.ah = 0x00;
	regs.h.al = 0x03;
	int86(0x10, &regs, &regs);
	exit(1);
}

void Set_40x30_TileMap_Mode(){//320x240@60Hz + square cursor
	union REGS regs;
	byte r_reg;
	int i = 0;
	word g_40x30_text[] = {
		0x0011,/*0x2d00,0x2701,0x2802,0x9003,0x2b04,0xa005,*/ //Horizontal 
		0x0d06,0x2e07,0x0008,0x8709,0x000A,0xea10,0x8e11,
		0xdf12,0x1513,0x1f14,0xe715,0x0616,0xa317,
	};
	system("cls");
	if (EGA == 0){
		//set mode 1 40x25
		regs.h.ah = 0x00;
		regs.h.al = 0x01; 
		int86(0x10, &regs, &regs);
		//tweak mode to be 40x30
		for (i = 0; i < 13; i++)outport(0x03d4,g_40x30_text[i]); 
		r_reg = inportb(0x3CC) & 0xF3; //Disable bits 2 and 3 (25 Mhz mode, 320 width)
		outportb(0x03C2, r_reg);
		//write SEQUENCER regs
		outport(0x03C4,0x0901);	//Clock rate/2 ; cell = 8 pixels wide
		outport(0x03C4,0x0003); //select font bank
		outport(0x03C4,0x0204); //Select VRAM size (0 = 64; 2 = 256;)
		//write GRAPHICS CONTROLLER regs
		outport(0x03CE,0x1005);	// Shift Reg. -- Shift Register Interleave Mode
		outport(0x03CE,0x0E06);	// Select VRAM B8000h-BFFFFh; Chain O/E ON; Text mode ON 
		//write ATTRIBUTE CONTROLLER regs
		inportb(0x03DA);//Set flip flop to index mode
		outportb(0x03C0,0x20 | 0x13);outportb(0x03C0,0x08);//Set no panning 
		outportb(0x03C0,0x20 | 0x10);outportb(0x03C0,0x24);//2c disable win pan, disable blink 

		//Make cursor a square
		outport(0x03D4,0x000A);
		outport(0x03D4,0x080B);
	}
	if (EGA == 1){
		//set mode 1 320x200 text 40x25
		regs.h.ah = 0x00;
		regs.h.al = 0x01; 
		int86(0x10, &regs, &regs);
		//Set font 8x8
		regs.h.ah = 0x11;
		regs.h.al = 0x12;
		regs.h.bh = 0x00;
		regs.h.bl = 0x00; 
		int86(0x10, &regs, &regs);
		//Disable blinking
		regs.h.ah = 0x10;
		regs.h.al = 0x03;
		regs.h.bl = 0x00;
		regs.h.bh = 0x00; 
		int86(0x10, &regs, &regs);
		//outport(0x03D4,0x0709);
		outport(0x03D4,0x1513);
		/*
		//write SEQUENCER regs
		//outport(0x03C4,0x0300);
		//outport(0x03C4,0x0001);//0x0101
		//outport(0x03C4,0x0302);
		//outport(0x03C4,0x0003);
		//outport(0x03C4,0x0304);
		//write GRAPHICS CONTROLLER regs
		outport(0x03CE,0x1005);	// Shift Reg. -- Shift Register Interleave Mode
		//outport(0x03CE,0x0E06);	// Select VRAM B8000h-BFFFFh; Chain O/E ON; Text mode ON 
		*/
		//write ATTRIBUTE CONTROLLER regs
		//inportb(0x03DA);//Set flip flip to index mode
		//outportb(0x03C0,0x20 | 0x13);outportb(0x03C0,0x08);//Set no panning 
		//outportb(0x03C0,0x20 | 0x10);outportb(0x03C0,0x24);// disable blinking (16 col B/F)
		//Make cursor a square
		outport(0x03D4,0x000A);
		outport(0x03D4,0x080B);
	}
}

void Enable_TileData_Write(){
	asm mov dx,0x03C4
	asm mov ax,0x0402	//Enable plane 0100 (2 - glyphs)
	asm out dx,ax
	asm mov ax,0x0404	//Sequential memory access
	asm out dx,ax
	asm mov dx,0x03CE
	asm mov ax,0x0204	//Read plane (2 - glyphs)
	asm out dx,ax
	asm mov ax,0x0005
	asm out dx,ax
	asm mov ax,0x0406	//Select VRAM A0000h-AFFFFh, Chain O/E OFF; keep text mode
	asm out dx,ax
};
	
void Disable_TileData_Write(){
	asm mov dx,0x03C4
	asm mov ax,0x0302	//Enable planes 0011 (1 and 0 - attribute and ascii)
	asm out dx,ax
	asm mov ax,0x0004	//Odd-Even memory access (Odd = plane 0 = ascii; even = plane 1 = attribute)
	asm out dx,ax
	asm mov dx,0x03CE
	asm mov ax,0x0004
	asm out dx,ax
	asm mov ax,0x1005
	asm out dx,ax
	asm mov ax,0x0E06	//Select VRAM B8000h-BFFFFh; Chain O/E ON; Text mode ON
	asm out dx,ax
};

void Set_Tiles(){
	word y = 0;
	word offset1 = 0;
	//It looks like VGA has 8x32 character glyphs 
	Enable_TileData_Write();
	for (y = 0; y < 32*256; y+=32){
		memcpy(&VGA[y],&font[offset1],8);//we load the first 8x8 part of each gliph
		offset1+=8;
	}
	Disable_TileData_Write();
}

void Animate_TileData(word tilepos,unsigned char *data, byte ntiles){
	/*int i = 0;
	int offset = 0;
	//VGA has 8x32 character glyphs */
	tilepos = tilepos << 5;
	/*for (i = 0; i < ntiles; i++){
		memcpy(&VGA[tilepos],&data[offset],8);//Update tile
		offset+=8;
		tilepos+=32;
	}*/

	//copy
	asm push ds
	asm push es
	asm push si
	asm push di
	
	asm mov ax,0xA000
	asm mov es,ax
	asm mov di,tilepos		// Destination VRAM  ES:DI
	asm lds	si,data			// Source RAM  DS:SI
	
	asm	or ch,ch
	asm mov cl,ntiles
	_loop: //Update tiles
		asm movsw			 
		asm movsw
		asm movsw
		asm movsw
		asm add di,24
		asm loop _loop
	
	asm pop di
	asm pop si
	asm pop es
	asm pop ds
}

void (*Text_Palette)(unsigned char *);

void Text_Palette_VGA(unsigned char *palette){
	int i;
	outportb(0x03C8,0);//Text Colors 0-5 are located at VGA 0-5
		for (i = 0; i < 6*3; i++)outportb(0x03C9,palette[i]>>2);
	outportb(0x03C8,20);//Text Color 6 is located at VGA color 20
		outportb(0x03C9,palette[6*3]>>2);
		outportb(0x03C9,palette[(6*3)+1]>>2);
		outportb(0x03C9,palette[(6*3)+2]>>2);
	outportb(0x03C8,7);//Text Color 7 is located at VGA 7
		outportb(0x03C9,palette[7*3]>>2);
		outportb(0x03C9,palette[(7*3)+1]>>2);
		outportb(0x03C9,palette[(7*3)+2]>>2);
	outportb(0x03C8,56);//Text Colors 8-15 are located at VGA 56-63
		for (i = 8*3; i < 16*3; i++) outportb(0x03C9,palette[i]>>2);
}

void Text_Palette_EGA(unsigned char *palette){
	int j;
	byte col = 0;
	for (j= 0; j<16;j++){
		col = palette[j];
		if (col > 7) col+=8;
		inportb(0x03DA);
		outportb(0x03C0,j);	//Index (0-15)
		outportb(0x03C0,col);//Color (0-15)
		outportb(0x03C0,0x20);
	}
}

void Wait_Vsync(){
	//Wait Vsync
	asm mov		dx,INPUT_STATUS_0
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
};

void Set_Map(unsigned char *map, int width,int height,int music){
	int i;
	word offset = 0;
	word offset1 = 0;
	for (i = 0; i < height;i++){
		if (music) Wait_Vsync();
		memcpy(&VRAM_MAP[offset],&map[offset1],width);
		memcpy(&VRAM_MAP[offset+(84*30*5)],&map[offset1],width);
		offset+=84;
		offset1+=(width);
		if (music) Play_Music();
	}
}

void Fade_In_Map(unsigned char *map, int width, byte col0){
	int i,j,k;
	int offset = 0;
	int offset1 = 0;
	
	//Reset panning
	asm mov dx,003c0h
	asm mov ax,033h
	asm out dx,ax
	asm mov al,8
	asm out dx,al
	
	for (i = -10; i < 160;i+=2){
		offset = i;
		offset1 = i;
		k = i;
		for (j = 0; j < 30;j++){
			if ((k> -2)&&(k<84)){VRAM_MAP[offset  ] = map[offset1  ]; VRAM_MAP[offset+1] = map[offset1+1];}
			if ((k> -4)&&(k<82)){VRAM_MAP[offset+2] = map[offset1+2]; VRAM_MAP[offset+3] = col0+3;}
			if ((k> -6)&&(k<80)){VRAM_MAP[offset+4] = map[offset1+4]; VRAM_MAP[offset+5] = col0+2;}
			if ((k> -8)&&(k<78)){VRAM_MAP[offset+6] = map[offset1+6]; VRAM_MAP[offset+7] = col0+1;}
			if ((k>-10)&&(k<76)){VRAM_MAP[offset+8] = map[offset1+8]; VRAM_MAP[offset+9] = col0  ;}
			offset1+=(width-2);
			offset+=82;
			k-=2;
		}
		Play_Music();
		Wait_Vsync();
	}
	Clearkb();
}

void Fade_Out_Map(int width, byte col0){
	int i,j,k;
	int offset = 0;
	int offset1 = 0;
	for (i = -10; i < 160;i+=2){
		offset = i;
		offset1 = i;
		k = i;
		for (j = 0; j < 30;j++){
			if ((k> -2)&&(k<84)) VRAM_MAP[offset  ] = 0;
			if ((k> -4)&&(k<82)){VRAM_MAP[offset+3] = col0;}
			if ((k> -6)&&(k<80)){VRAM_MAP[offset+5] = col0+1;}
			if ((k> -8)&&(k<78)){VRAM_MAP[offset+7] = col0+2;}
			if ((k>-10)&&(k<76)){VRAM_MAP[offset+9] = col0+3;}
			offset1+=(width-2);
			offset+=82;
			k-=2;
		}
		Play_Music();
		Wait_Vsync();
	}
	Clearkb();
}

void Fade_In_Random(unsigned char *map, int width){
	int i,j,k,offset,offset1;
	//Reset panning
	asm mov dx,003c0h
	asm mov ax,033h
	asm out dx,ax
	asm mov al,8
	asm out dx,al
	
	for (i = 0; i < 82*30; i+=30){
		for (k = 0; k < 30; k+=2){
			offset = (Random_Fade[i+k+1]*84)+(Random_Fade[i+k]<<1);
			offset1 = (Random_Fade[i+k+1]*width)+(Random_Fade[i+k]<<1);
			VRAM_MAP[offset] = map[offset1];
			VRAM_MAP[offset+1] = map[offset1+1];
		}
		Play_Music();
		Wait_Vsync();
	}
	Clearkb();
}

void (*ScrollTextMap)(int, int);

void ScrollTextMap_EGA(int x, int y){
	word _offset;
	pixel = y & 7;
	line = y >> 3;
	
	_offset = (line*42) + (x >> 3); //line*80 + x/8
	asm CLI
	
	//Wait Vsync
	asm mov		dx,INPUT_STATUS_0
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync

	//Set address
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,_offset
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,_offset
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port
	
	// set the preset row scan while IN the vretrace, else flicker
	// Also this seems to fix the animate tiles function, 
	// some register needs to be reset in vsync or something
	asm mov dx,0x03d4
	asm mov ax,0x08
	asm out dx,ax
	asm inc dx
	asm mov ax,word ptr pixel
	asm out dx,ax
	
	//The smooth panning magic happens here
	panning = x&7;
	asm mov dx,003c0h
	asm mov ax,033h
	asm out dx,ax
	asm mov al,panning
	asm out dx,al

	asm STI
}

void ScrollTextMap_VGA(int x, int y){
	word _offset;
	pixel = y & 7;
	line = y >> 3;
	
	_offset = (line*42) + (x >> 3); //line*80 + x/8
	asm CLI
	
	//Set address
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,_offset
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,_offset
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port
	
	//Wait Vsync
	asm mov		dx,INPUT_STATUS_0
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
	
	//The smooth panning magic happens here
	if (static_screen) panning = 8;
	else panning = text_mode_panning[(x&7)+1]; //avoid value "8" when scrolling, it causes choppy scroll on real VGA
	asm mov dx,003c0h
	asm mov ax,033h
	asm out dx,ax
	asm mov al,panning
	asm out dx,al
	
	// set the preset row scan while IN the vretrace, else flicker
	// Also this seems to fix the animate tiles function, 
	// some register needs to be reset in vsync or something
	asm mov dx,0x03d4
	asm mov ax,0x08
	asm out dx,ax
	asm inc dx
	asm mov ax,word ptr pixel
	asm out dx,ax
	
	asm STI
}

void ScrollTextMap_SVGA(int x, int y){
	word _offset;
	byte ac;
	pixel = y & 7;
	line = y >> 3;
	
	_offset = (line*42) + (x >> 3); //line*80 + x/8
	
	//Set address
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,_offset
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,_offset
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port
	
	asm CLI
	asm mov dx,0x03d4
	asm mov ax,0x08
	asm out dx,ax
	asm inc dx
	asm mov ax,word ptr pixel
	asm out dx,ax
	
	//The smooth panning magic happens here
	panning = text_mode_panning[x&7];
	asm mov dx,003c0h
	asm mov ax,033h
	asm out dx,ax
	asm mov al,panning
	asm out dx,al
	asm STI
	
	//Wait Vsync
	asm mov		dx,INPUT_STATUS_0
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
}

void (*SplitScreen)(int);

void SplitScreen_VGA(int line){
	asm cli 
	// set bits 7-0 of the split screen scan line
	outportb(0x03d4,0x18); outportb(0x03d5,line&0x00FF);
	// Set bit 8
	if (line == 480) outport(0x03d4,0x3e07);
	else outport(0x03d4,0x2e07);
    asm sti
	//We don't need bit 9 at Maximum Scan Line Register
}

void SplitScreen_EGA(int line){
	asm cli 
	// set bits 7-0 of the split screen scan line
	outportb(0x03d4,0x18); outportb(0x03d5,line&0x00FF);
	// Set bit 8
	if (line == 480) outport(0x03d4,0x1107);
	else outport(0x03d4,0x0107);
    asm sti
}

void Clear_screen(){
	int offset = 0;
	for (offset = 0; offset < 84*30; offset+=84){
		Wait_Vsync();
		memset(&VRAM_MAP[offset],0,84);
		Play_Music();
	}
	ScrollTextMap(0,0);
}

void Move_Cursor(int x, int y){//custom gotoxy
	word pos = (42*y)+x;
	word pos_low  = (pos <<   8  ) | 0x0F;
	word pos_high = (pos & 0xFF00) | 0x0E;
	asm mov ax,0x03D4
	asm mov dx,ax
	asm mov ax,pos_low
	asm out dx,ax
	asm mov ax,pos_high
	asm out dx,ax
};

void (*Update_Sprite)(word, int,unsigned char*);
void Update_Sprite_Hardware(word tilepos,int y, unsigned char *data){
	asm mov dx,0x03CE
	asm mov ax,0x1003
	asm out dx,ax		//VRAM writes will be or-ed with latch
	//VGA has 8x32 character glyphs
	tilepos = tilepos << 5;
	//copy

	asm push ds
	asm push es
	asm push si
	asm push di
	
	asm mov ax,0xA000
	asm mov es,ax
	asm mov di,tilepos		// Destination VRAM  ES:DI
	asm lds	si,data			// Source RAM  DS:SI
	
	asm mov bx,y
	asm and bx,7
	asm mov cx,3
	_loop:
		//Update column
		asm push cx
		asm add di,bx	//skip
		asm mov cl,8	
		asm sub cx,bx
		_loop0://1
			asm mov al,es:[di]	//Load latch from VRAM
			asm movsb
			asm loop _loop0
		asm add di,24
		asm mov cx,8
		_loop1://2
			asm mov al,es:[di]	//Load latch from VRAM
			asm movsb
			asm loop _loop1
		asm add di,24
		
		asm mov cx,bx		//3
		asm cmp	cx,0
		asm je	_end
		_loop2:
			asm mov al,es:[di]
			asm movsb
			asm loop _loop2
		
		_end:
		asm add di,24+8
		asm sub di,bx
		asm pop cx
		asm loop _loop
	
	asm pop di
	asm pop si
	asm pop es
	asm pop ds
	
	asm mov ax,0x0003
	asm out dx,ax		//Reset 03CE
}

void Update_Sprite_Emulator(word tilepos,int y, unsigned char *data){
	//VGA has 8x32 character glyphs
	tilepos = tilepos << 5;
	//copy

	asm push ds
	asm push es
	asm push si
	asm push di
	
	asm mov ax,0xA000
	asm mov es,ax
	asm mov di,tilepos		// Destination VRAM  ES:DI
	asm lds	si,data			// Source RAM  DS:SI
	
	asm mov bx,y
	asm and bx,7
	asm mov cx,3
	_loop:
		//Update column
		asm push cx
		asm add di,bx	//skip
		asm mov cl,8	
		asm sub cx,bx
		_loop0://1
			asm mov ah,ds:[si]
			asm mov al,es:[di]
			asm or	al,ah
			asm mov es:[di],al
			asm inc si
			asm inc di
			asm loop _loop0
		asm add di,24
		asm mov cx,8
		_loop1://2
			asm mov al,es:[di]
			asm mov ah,ds:[si]
			asm or	al,ah
			asm mov es:[di],al
			asm inc si
			asm inc di
			asm loop _loop1
		asm add di,24
		
		asm mov cx,bx		//3
		asm cmp	cx,0
		asm je	_end
		_loop2:
			asm mov al,es:[di]
			asm mov ah,ds:[si]
			asm or	al,ah
			asm mov es:[di],al
			asm inc si
			asm inc di
			asm loop _loop2
		
		_end:
		asm add di,24+8
		asm sub di,bx
		asm pop cx
		asm loop _loop
	
	asm pop di
	asm pop si
	asm pop es
	asm pop ds
}

void Draw_Sprite(byte spr,int tile, unsigned char *data, int frame, int x, int y){
	int pos0 = ((y>>3)*84) + ((x>>3)<<1);
	int pos1 = ((last_sy[spr]>>3)*84) + ((last_sx[spr]>>3)<<1);
	int pos2;
	int sprpos = spr<<5;
	int tilepos = tile<<5;
	word px = x&7;
	word offset = (px<<5)+(px<<4)+(frame<<8)+(frame<<7);
	int i = 0;
	unsigned char *data3 = SpBKG;

	//Restore BKG
	asm push ds
	asm push es
	asm push si
	asm push di
	if (spr_init[spr]){
		//for (i=0;i<18;i+=6){memcpy(&VRAM_MAP[pos1],&SpBKG[i+sprpos],6);pos1+=84;}
		asm mov ax,0xB800
		asm mov es,ax	
		asm mov di,pos1		// Destination ES:DI
		asm lds si,data3	// Source
		asm add si,sprpos
		asm mov cx,3
		_loopA:
			asm movsw
			asm movsw
			asm movsw
			asm add di,78
			asm loop _loopA
	}
	spr_init[spr] = 1;
	//Get BKG
	pos2 = pos0;
	//for (i = 0; i < 18;i+=6){memcpy(&SpBKG[i+sprpos],&VRAM_MAP[pos2],6);pos2+=84;}
	asm mov ax,0xB800
	asm mov ds,ax	
	asm mov si,pos2		// Source
	asm les di,data3	// Destination
	asm add di,sprpos
	asm mov cx,3
	_loopB:
		asm movsw
		asm movsw
		asm movsw
		asm add si,78
		asm loop _loopB
		
	asm pop di
	asm pop si
	asm pop es
	asm pop ds
	
	
	//Get buffer 181.878
	Enable_TileData_Write();
	for (i = 0; i < 6;i+=2){
		memcpy((byte *)(0xA0000000+tilepos),&VGA[SpBKG[i+sprpos]<<5],8); tilepos+=32;
		memcpy((byte *)(0xA0000000+tilepos),&VGA[SpBKG[i+sprpos+6]<<5],8); tilepos+=32;
		memcpy((byte *)(0xA0000000+tilepos),&VGA[SpBKG[i+sprpos+12]<<5],8); tilepos+=32;
	}

	//Paste sprite data to buffer
	Update_Sprite(tile,y,&data[offset]);
	Disable_TileData_Write();
	
	//Paste sprite tiles
	asm push es
	asm push si
	asm push di
	
	asm mov ax,0xB800
	asm mov es,ax	
	asm mov di,pos0		// Destination ES:DI
	asm mov al,byte ptr tile
	asm mov cx,3
	_loop0:
		asm stosb
		asm inc di
		asm add al,3
		asm stosb
		asm inc di
		asm add al,3
		asm stosb
		asm add di,79
		asm sub al,5
		asm loop _loop0
	
	asm pop di
	asm pop si
	asm pop es
	
	last_sx[spr] = x; last_sy[spr] = y;
}

void Delete_Sprite(byte spr){
	int i;
	int pos1 = ((last_sy[spr]>>3)*84) + ((last_sx[spr]>>3)<<1);
	for (i=0;i<18;i+=6){memcpy(&VRAM_MAP[pos1],&SpBKG[i+(spr<<5)],6);pos1+=84;}
	spr_init[spr] = 0;
}

////////////////////////////
////////////////////////////
void Calibrate(){
	int spx = 0; int spy = 0;
	int dpx = 1; int dpy = 1;
	int x = 0;
	int y = 0;
	int i = 0;
	if (!Check_Graphics()){
		printf("This demo requires EGA or VGA card.\nEsta demo requiere una tarjeta EGA o VGA.");
		sleep(2);
		Exit_Demo();
	}
	system("cls");
	Clearkb();
	printf("\nWAIT!!!\n-------\n\nIf you are using an emulator, press 1.\n");
	printf("If you are using a real Hardware, ao486 FPGA or DosBox-X, press 2.\n\n");
	printf("Si usas un emulator, pulsa 1.\n");
	printf("Si usas hardware real, ao486 FPGA o DosBox-X, pulsa 2.\n\n");
	while(!i){
		Wait_Vsync();
		if(kbhit()) {
			key = getch();
			if (key == 49) Update_Sprite = &Update_Sprite_Emulator;
			else if (key == 50) Update_Sprite = &Update_Sprite_Hardware;
			else Update_Sprite = &Update_Sprite_Hardware;
			i = 1;
			Clearkb();
		}
	}
	system("cls");
	static_screen = 0;
	Set_40x30_TileMap_Mode();
	Set_Tiles();
	
	if (EGA == 0){
		ScrollTextMap = &ScrollTextMap_VGA;
		SplitScreen = &SplitScreen_VGA;
		Text_Palette = &Text_Palette_VGA;
	} else {
	 	ScrollTextMap = &ScrollTextMap_EGA;
		SplitScreen = &SplitScreen_EGA;
		Text_Palette = &Text_Palette_EGA;
	}

	//ScrollTextMap(x,(24*8)+y);
	SplitScreen(64);
	
	Clearkb();
	Set_Map(cal_map,84,24,0);
	for (i = 84*24; i < 84*44;i += 84*2){
		memcpy(&VRAM_MAP[i],&cal_map[84*22],84*2);
	}
	gotoxy(0,0);
	while(key != 32){//SPACE 
		ScrollTextMap(x&31,(23*8)+(y&31));
		Draw_Sprite(0,128,sprite_question,0,spx,spy);
		x++;y+=2;
		if(kbhit()) {
			key = getch();
			Clearkb();
			//0 = VGA REAL, 1 VGA EMU, 2 SVGA REAL, 3 SVGA EMU
			if (EGA== 0){
				if (key == 49) {Split_Value = 128; ScrollTextMap = &ScrollTextMap_VGA;}
				if (key == 50) {Split_Value = 64;  ScrollTextMap = &ScrollTextMap_VGA;}
				if (key == 51) {Split_Value = 128; ScrollTextMap = &ScrollTextMap_SVGA;}
				if (key == 52) {Split_Value = 64;  ScrollTextMap = &ScrollTextMap_SVGA;}
			}else {
				if (key == 49) {Split_Value = 128-20; ScrollTextMap = &ScrollTextMap_EGA;}
				if (key == 50) {Split_Value = 64-20;  ScrollTextMap = &ScrollTextMap_EGA;}
			}
		}
		SplitScreen(Split_Value);
		
		if (dpx == 1) spx++; else spx--;
		if (dpy == 1) spy++; else spy--;
		if (spx == 304 ) dpx = -1;
		if (spx == 0   ) dpx =  1;
		if (spy == 160 ) dpy = -1;
		if (spy == 0   ) dpy =  1;
	}
	Delete_Sprite(0);
	ScrollTextMap(0,0);
	SplitScreen(480);
	gotoxy(1,1);
	textattr(0x0F);
	memset(VRAM_MAP,21,84*40);
	//if (EGA) Exit_Demo();
}

//Fake booting
void Fake_boot(){
	int i = 0;
	int pos = 0;
	word mapram = 32768;
	word tileram = 2048;
	if (!EGA)Text_Palette(intro_palette);
	else Text_Palette(intro_palette_EGA);
	ScrollTextMap(0,0);
	outport(0x03D4,0x200A); //Disable cursor
	
	for (i = 0; i < 40*25*2; i+=32){
		cprintf("                                ");
		pos+=64;
		Wait_Vsync();
	}
	static_screen = 1;
	gotoxy(0,0);
	i = 0;
	while (i <100){
		Wait_Vsync();
		if (!(i&1))cprintf(" ");
		VRAM_MAP[i] = bios_map[i];
		i++;
	}

	sleep(1);
	textattr(0x0F);
	i = 0;
	while (i != mapram){
		Wait_Vsync();
		gotoxy(10,3);
		cprintf("MAP VRAM %i KB",i);
		i+=512;
	}
	outport(0x03D4,0x000A); //Enable cursor
	gotoxy(10,3); cprintf("MAP VRAM 32768 KB OK",i); Move_Cursor(21,4);
	sleep(1);
	outport(0x03D4,0x200A); //Disable cursor
	i = 0;
	while (i != tileram){
		Wait_Vsync();
		gotoxy(52,3);
		cprintf("TILE VRAM %i KB",i);
		i+= 32;
	}
	outport(0x03D4,0x000A); //Enable cursor
	gotoxy(52,3); cprintf("TILE VRAM 2048 KB OK",i); Move_Cursor(21,5);
	sleep(1);
	gotoxy(14,4); cprintf("ADLIB",i);
	gotoxy(14,4); 
	if (Adlib_Check()) cprintf("ADLIB OK",i);
	else cprintf("NO ADLIB ",i);
	Move_Cursor(10,6);
	sleep(1);
	
	gotoxy(42,1);
	
	Clearkb();
}

unsigned char fading_bars[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x02,0x00,0x02,0x02,0x02,0x02,0x02,0x00,0x01,
	0x04,0x01,0x04,0x04,0x04,0x04,0x04,0x00,0x02,
	0x07,0x02,0x07,0x06,0x06,0x06,0x06,0x01,0x04,
	0x09,0x03,0x09,0x08,0x08,0x08,0x08,0x01,0x06,
	0x0B,0x03,0x0B,0x0A,0x0A,0x0A,0x0A,0x01,0x08,
	0x0E,0x04,0x0E,0x0C,0x0C,0x0C,0x0C,0x02,0x0A,
	0x10,0x05,0x10,0x0E,0x0E,0x10,0x0E,0x02,0x0C,
	0x12,0x06,0x12,0x10,0x0D,0x15,0x10,0x02,0x0D,
	0x15,0x06,0x16,0x12,0x0F,0x18,0x12,0x03,0x0F,
	0x17,0x07,0x18,0x1D,0x10,0x1D,0x14,0x03,0x10,
	0x19,0x08,0x1A,0x20,0x10,0x20,0x14,0x04,0x12,
	0x1C,0x09,0x1D,0x22,0x10,0x25,0x14,0x05,0x13,
	0x1E,0x0A,0x1F,0x25,0x11,0x28,0x14,0x06,0x15,
	0x21,0x0B,0x23,0x28,0x11,0x2D,0x15,0x06,0x16,
};

void Fade_Purple_Bars(byte col){
	outportb(0x03C8,4); //Color 4
	outportb(0x03C9,fading_bars[col]);outportb(0x03C9,fading_bars[col+1]);outportb(0x03C9,fading_bars[col+2]);
	outportb(0x03C8,59); //Color 11 12
	outportb(0x03C9,fading_bars[col+3]);outportb(0x03C9,fading_bars[col+4]);outportb(0x03C9,fading_bars[col+5]);
	outportb(0x03C9,fading_bars[col+6]);outportb(0x03C9,fading_bars[col+7]);outportb(0x03C9,fading_bars[col+8]);	
}

void Scroll_Greetings(int x){
	word offset = 82+(x>>2);
	word offset1 = greetings_offset >> 2;
	byte y = 0;
	for (y = 0; y < 8; y++){
		VRAM_MAP[(84*64)+(84)+offset] = scroll_intro_map[offset1];
		offset+=84;
		offset1+=438*2;
	}
	greetings_offset+=2;
	if (greetings_offset == 438*8) greetings_offset = 0;
};

void Scroll_Small_Bar(int x){
	if (!(x&15)) memcpy(&VRAM_MAP[18*84],&small_scroll_bar[x>>3],80);
}

void Draw_Copper_Bar(int pos){
	word bar_offset = copper_fix_pos[pos];
	byte offset = bar_shade[copper_shade&1];
	memcpy(&VGA[3584+bar_offset],&copper_bar[offset],8);//Update tile
	memcpy(&VGA[3584+24+bar_offset],&copper_bar[offset],8);//Update tile
	memcpy(&VGA[3584+32+bar_offset],&copper_bar[offset+8],8);//Update tile
	memcpy(&VGA[3584+56+bar_offset],&copper_bar[offset+8],8);//Update tile
}

void Cycle_Copper_Bars_color(){
	
	switch (cycle_color_state){
		case 0: b++; if (b == 63) cycle_color_state = 1; break;
		case 1: r--; if (r ==  0) cycle_color_state = 2; break;
		case 2: g++; if (g == 63) cycle_color_state = 3; break;
		case 3: b--; if (b ==  0) cycle_color_state = 4; break;
		case 4: r++; if (r == 63) cycle_color_state = 5; break;
		case 5: g--; if (g ==  0) cycle_color_state = 0; break;
	}
	outportb(0x03C8,61);//Text Color 13 is located at vga color 61
	outportb(0x03C9,r);
	outportb(0x03C9,g);
	outportb(0x03C9,b);
	copper_cycle_color++;
}

void Run_Intro(){
	int i,j;
	int pos_x = 0;
	int pos_x2 = 176;
	int pos_y = 0;
	word offset = 0;
	int copper_y0 = 0;
	int copper_y1 = 32;
	int copper_y2 = 64;
	int copper_y3 = 96;
	int copper_y4 = 128;
	byte spx = 0;
	int frame = 0;
	word anim = 0;
	greetings_offset = 0;
	static_screen = 1;
	ScrollTextMap(0,30*8*3);
	if (!loop) Set_Map(intro_map,80,30,0);
	else Set_Map(intro_map,80,30,1);
	Clearkb();
	//Wave tittle
	j = 0;
	for (i = 0; i < 460; i++){
		if (EGA) ScrollTextMap(0,logo_wave[i]+(30*8*5)+20);
		else ScrollTextMap(0,logo_wave[i]+(30*8*5));
		if (i < 45) { memcpy(&VRAM_MAP[(84*62)+j],scroll_intro_map,42);j+=42;}
		Play_Music();
		if(kbhit()) {key = getch(); if (key == 27) break;}
	}
	SplitScreen(Split_Value);
	if (key == 27) Exit_Demo(); 
	for (i = 0; i < 250; i++){Wait_Vsync();Play_Music();}
	//Make time
	for (i = 0; i < 710; i++){
		Wait_Vsync();
		Draw_Sprite(0,128,sprite_ghost,frame,70+sine[spx],32+(sine[(spx<<1)+48])>>1);
		Enable_TileData_Write();
		Animate_TileData((16*7)+1,&intro_anim[offset&7],12);
		Disable_TileData_Write();
		Play_Music();
		offset++;
		spx++;
		if (spx > 128) frame = 1;
		else frame = 0;
		if(kbhit()) {key = getch(); if (key == 27) break;}
	}
	Delete_Sprite(0);
	if (key == 27) Exit_Demo(); 

	static_screen = 0;
	Clearkb();
	r = 63; g = 0; b = 0;
	cycle_color_state = 0;
	j = 0;
	
	for (i = 0; i < 14*9; i+=9) {
		ScrollTextMap(pos_x,(64*8) + greetings_wave_y[pos_y] + 4);
		Enable_TileData_Write();
		memset(&VGA[3584+j],0,32);
		j+=32;
		Disable_TileData_Write();
		Fade_Purple_Bars(i);
		Play_Music();
	}
	
	Move_Cursor(31,14);
	while (key != 27){
		if (EGA) ScrollTextMap(pos_x,(64*8) + greetings_wave_y[pos_y] + 4+15);
		else ScrollTextMap(pos_x,(64*8) + greetings_wave_y[pos_y] + 4);
		Enable_TileData_Write();
			Draw_Copper_Bar(copper_wave[copper_y0]);
			Draw_Copper_Bar(copper_wave[copper_y1]);
			Draw_Copper_Bar(copper_wave[copper_y2]);
			Draw_Copper_Bar(copper_wave[copper_y3]);
			Draw_Copper_Bar(copper_wave[copper_y4]);
		Disable_TileData_Write();
		
		Scroll_Greetings(pos_x);
		Scroll_Small_Bar(pos_x2);
		Cycle_Copper_Bars_color();
		
		pos_x+=2;pos_x2-=2;pos_y++;anim+=(14*8);
		copper_y0++;copper_y1++;copper_y2++;copper_y3++;copper_y4++;
		
		if (anim == (14*8*120)) anim = 0;
		if (pos_x2 == 0) pos_x2 = 176;
		if (pos_y == 92) pos_y = 0;
		
		if (copper_y0 == 302) copper_y0 = 0;
		if (copper_y1 == 302) copper_y1 = 0;
		if (copper_y2 == 302) copper_y2 = 0;
		if (copper_y3 == 302) copper_y3 = 0;
		if (copper_y4 == 302) copper_y4 = 0;
		copper_shade++;
		Play_Music();
		if (pos_x == 3528) break;
		if(kbhit()) key = getch();
	}
	if (key == 27) Exit_Demo(); 
	Move_Cursor(42,256);
	j = 0;
	for (i = 14*9; i > -1; i-=9) {
		ScrollTextMap(pos_x,(64*8) + greetings_wave_y[pos_y] + 4);
		Enable_TileData_Write();
		memset(&VGA[3584+j],0,32);
		j+=32;
		Disable_TileData_Write();
		Fade_Purple_Bars(i);
		Play_Music();
	}
	
	Delete_Sprite(0);
	Fade_Out_Map(80,6);
	static_screen = 0;
	ScrollTextMap(0,0);
	SplitScreen(480);
	Scene = 2;
	Clearkb();

};

void Rotate_Twist(int pos){
	unsigned char *data = &twister_animation_map[pos];
	/*int x = 17*2;
	int z = 0;
	for (z = 0; z < 30; z++){
		memcpy(&VRAM_MAP[x],data,40);
		x+=84;
	}*/

	asm push ds
	asm push es
	asm push si
	asm push di
	
	asm mov ax,0xB800
	asm mov es,ax
	asm mov di,17*2			// Destination VRAM  ES:DI
	asm lds	si,data			// Source RAM  DS:SI
	
	asm	or ch,ch
	asm mov cl,30
	_loop: //Update Lines
		asm movsw			 
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw			 
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm add di,84-40
		asm sub si,40
		asm loop _loop
	
	asm pop di
	asm pop si
	asm pop es
	asm pop ds
}

void Move_Twist(int pos){
	unsigned char *data;
	/*int x = 17*2;
	int z = 0;
	for (z = 0; z < 30; z++){
		memcpy(&VRAM_MAP[x],&twister_animation_map[twister_twist[pos]],40);
		x+=84;
		pos++;
	}*/
	int y;
	//122.132
	asm push ds
	asm push es
	asm push si
	asm push di
	
	//This C line does not destroy anything important
	//And it is as optimized as it can be when compiled
	data = &twister_animation_map[twister_twist[pos]];
	asm lds si,data			//Source DS:SI
	asm mov ax,0xB800
	asm mov es,ax
	asm mov di,17*2			// Destination VRAM  ES:DI	
	
	asm	or ch,ch
	asm mov cl,30
	_loop: //Replaced memcpy with this. It looks faster
		asm movsw			 
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw			 
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm movsw
		asm add di,84-40
		
		pos++;
		data = &twister_animation_map[twister_twist[pos]];
		asm lds si,data			//Source DS:SI
		asm mov ax,0xB800
		asm mov es,ax
		
		asm loop _loop
	
	asm pop di
	asm pop si
	asm pop es
	asm pop ds
}

void Rotate_3DText(int pos){
	//unsigned char *data;
	int x = (84*2)+(19*2);
	int pos1 = (pos+(32*4*10))%(32*4*120);
	int pos2 = (pos+(32*4*20))%(32*4*120);
	int pos3 = (pos+(32*4*30))%(32*4*120);
	
	// Draw "D"
	memcpy(&VRAM_MAP[x],&text_3d[pos+32],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos+32],32);x+=84*3;	
	
	// Draw "E"
	memcpy(&VRAM_MAP[x],&text_3d[pos1+64],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos1   ],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos1+64],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos1   ],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos1+64],32);x+=84*3;

	// Draw "M"
	memcpy(&VRAM_MAP[x],&text_3d[pos2+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos2+64],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos2+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos2+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos2+96],32);x+=84*3;
	
	//Draw "O"
	memcpy(&VRAM_MAP[x],&text_3d[pos3+64],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos3+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos3+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos3+96],32);x+=84;
	memcpy(&VRAM_MAP[x],&text_3d[pos3+64],32);
}

void Rotate_3DPC(int pos){
	//unsigned char *data;
	int x = (840)+(20*2);

	memcpy(&VRAM_MAP[x],&PC_3d[pos],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos+28],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos+28],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos+28],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos+28],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos   ],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos+56],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos+84],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos+112],28);x+=84;
	memcpy(&VRAM_MAP[x],&PC_3d[pos+84],28);x+=84;
}

void Run_Twister(){
	int i;
	byte text = 0;
	byte speed = 0;
	byte twister_mode = 0;
	word twister_pos = 0;
	word number = 0;
	word number2 = 0;
	static_screen = 1;
	if (EGA == 0)Text_Palette(twister_palette);
	else Text_Palette(twister_palette_EGA);
	Fade_In_Map(twister_map,80,1);
	for (i = 0; i < 30*84; i+=84){
		if (EGA == 1)ScrollTextMap(0,16);
		else ScrollTextMap(0,0);
		memcpy(&VRAM_MAP[(17*2)+i],twister_animation_map,40);
		Play_Music();
	}
	while (key != 27){
		if (EGA == 1)ScrollTextMap(0,16);
		else ScrollTextMap(0,0);
		Enable_TileData_Write();
		Animate_TileData(21,&heart_animation[number],1);
		Disable_TileData_Write();
		
		switch (twister_mode){
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
				Rotate_3DPC(twister_pos);
				twister_pos+=(28*5);
				if (twister_pos == 28*5*120) {twister_pos = 0;twister_mode++;}
			break;
			case 10:
				memset(&VRAM_MAP[(17*2)+twister_pos],0,40);
				VRAM_MAP[0x05EE + text] = twister_text[text+140];
				VRAM_MAP[0x0642 + text] = twister_text[text+154];
				VRAM_MAP[0x0696 + text] = twister_text[text+168];
				VRAM_MAP[0x06EA + text] = twister_text[text+182];
				VRAM_MAP[0x073E + text] = twister_text[text+196];
				if (text < 13) text++;
				twister_pos+=84;
				if (twister_pos == 84*30) {text = 0; twister_pos = 0;twister_mode++;}
			break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				Rotate_3DText(twister_pos);
				VRAM_MAP[0x073E + text] = twister_text[text+126];
				if (text < 13) text++;
				twister_pos+=(32*4);
				if (twister_pos == 32*4*120) {text = 0;twister_pos = 0;twister_mode++;}
			break;
			case 3:
				memset(&VRAM_MAP[(17*2)+twister_pos],0,40);
				VRAM_MAP[0x05EE + text] = twister_text[text+70];
				VRAM_MAP[0x0642 + text] = twister_text[text+84];
				VRAM_MAP[0x0696 + text] = twister_text[text+98];
				VRAM_MAP[0x06EA + text] = twister_text[text+112];
				if (text < 13) text++;
				twister_pos+=84;
				if (twister_pos == 84*30) {text = 0; twister_pos = 0;twister_mode++;}
			break;
			case 2:
				Move_Twist(twister_pos);
				twister_pos+=30;
				if (twister_pos == 30*480) {text = 0; twister_pos = 0;twister_mode++;}
			break;
			case 0:
			case 1:
				Rotate_Twist(twister_pos);
				VRAM_MAP[0x05EE + text] = twister_text[text];
				VRAM_MAP[0x0642 + text] = twister_text[text+14];
				VRAM_MAP[0x0696 + text] = twister_text[text+28];
				VRAM_MAP[0x06EA + text] = twister_text[text+42];
				VRAM_MAP[0x073E + text] = twister_text[text+56];
				if (text < 13) text++;
				twister_pos+=40;
				if (twister_pos == 120*40) {text = 0; twister_pos = 0;twister_mode++;}
			break;
		}
		number2+=(4*8);
		speed++;
		if (speed == 6) {speed = 0; number+=8;}
		if (number == 12*8) number = 0;
		if (number2 == 32*(8*4)) number2 = 0;
		Play_Music();
		if (twister_mode == 17) break;
		if(kbhit()) key = getch();
	}
	Fade_Out_Map(80,1);
	static_screen = 0;
	Enable_TileData_Write();
	Animate_TileData(21,&heart_animation[0],1);
	Disable_TileData_Write();
	Scene = 3;
	if (key == 27) Scene = 4; 
};

void Scroll_Parallax(int x){
	word offset = 82+(x>>2);
	word offset1 = pmap_offset>>2;
	byte y = 0;
	for (y = 0; y < 30; y++){
		VRAM_MAP[offset] = parallax_map[offset1];
		offset+=84;
		offset1+=130*2;
	}
	pmap_offset+=2;
	if (pmap_offset == 130*8) pmap_offset = 0;
};

void Fade_Out_Parallax(int x){
	word offset = 82+(x>>2);
	byte y = 0;
	for (y = 0; y < 30; y++){VRAM_MAP[offset] = 0x00;offset+=84;}
};

void Run_Parallax(){
	byte spy = 0;
	word pos_x = 0;
	word offset = 0;
	word offset0 = 0;
	if (EGA == 0)Text_Palette(parallax_palette);
	else Text_Palette(parallax_palette_EGA);
	pmap_offset = 328;
	Fade_In_Random(parallax_map,260);
	while (offset < 120){
		Wait_Vsync();
		Draw_Sprite(0,128,sprite_moon,0,pos_x++,48+(sine[spy++]>>1));
		Play_Music();
		offset++;
	}
	pos_x = 0;
	offset = 0;
	while (key != 27){
		if (EGA == 1)ScrollTextMap(pos_x,40);
		else ScrollTextMap(pos_x,0);
		Draw_Sprite(0,128,sprite_moon,0,pos_x+120,48+(sine[spy++]>>1));
		Enable_TileData_Write();
		Animate_TileData(16*13,&parallax_tiles[offset],35);
		Disable_TileData_Write();
		Scroll_Parallax(pos_x);
		
		if ((pos_x > 90*8)&&( pos_x < (90*8+262))){
			int ppos = pos_x - (90*8) -2;
			parallax_map[(29*130*2) + ppos] = 0x00;
			parallax_map[(29*130*2) + ppos + 1] = 0x6F;
		}
		
		offset+=(35*8);
		if (offset == (35*8*40)) offset = 0;
		pos_x+=2;
		
		Play_Music();
		if (pos_x == 130*8*4) break;
		if(kbhit()) key = getch();
	}
	Delete_Sprite(0);
	for (offset0 = 0; offset0 < 42*8;offset0+=2){
		if (EGA == 1)ScrollTextMap(pos_x,40);
		else ScrollTextMap(pos_x,0);
		Enable_TileData_Write();
		Animate_TileData(16*13,&parallax_tiles[offset],35);
		Disable_TileData_Write();
		Fade_Out_Parallax(pos_x);
		pos_x+=2;
		offset+=(35*8);
		if (offset == (35*8*40)) offset = 0;
		Play_Music();
	}
	//Clean screen for intro
	for (offset = 0; offset < 84*60;offset+=84){
		if (EGA == 1)ScrollTextMap(pos_x,40);
		else ScrollTextMap(pos_x,0);
		memset(&VRAM_MAP[offset],0,84);
		Play_Music();
	}
	
	Scene = 0;
	loop = 1;
	if (EGA == 0)Text_Palette(intro_palette);
	else Text_Palette(intro_palette_EGA);
	if (key == 27) Scene = 4; 
};

void main(){
	Calibrate();
	Fake_boot();

	while(key != 27){
		if (Scene == 0) Run_Intro();
		if (Scene == 2) Run_Twister();
		if (Scene == 3) Run_Parallax();
	}
	
	Exit_Demo();
}




