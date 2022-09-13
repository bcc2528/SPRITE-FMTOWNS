#include <stdio.h>
#include <dos.h>
#include <egb.h>
#include <spr.h>
#include <snd.h>
#include <his.h>

char work[1536];

#define VSYNCclear 0x05ca
#define VSYNCintNumber 11
#define stackSize 1000
char HIS_stack[ stackSize ];
int Vsync_Count;

void VSYNChandler( void )
{
	Vsync_Count++;

	/******** ÇuÇrÇxÇmÇbäÑÇËçûÇ›å¥àˆÉNÉäÉAÉåÉWÉXÉ^Ç÷ÇÃèëÇ´çûÇ› ********/
	_outb( VSYNCclear, 0 );
}

int KurosawaRand()
{
	static unsigned short lastRnd = 0;

	lastRnd ^= 0x9630U;
	lastRnd -= 0x6553U;
	lastRnd =  (lastRnd << 2) | (lastRnd >> 14);

	return (signed short)lastRnd;
}

void Display_init(int mode)
{
	switch (mode)
	{
		case 1:  // TBIOS Mode5 (31.47KHz Noninterlace)
			EGB_resolution(work, 1, 7);
			EGB_resolution(work, 0, 7);
			break;
		case 2:  // TBIOS Mode7 (15.73KHz Noninterlace)
			EGB_resolution(work, 1, 8);
			EGB_resolution(work, 0, 8);
			break;
		default: // TBIOS Mode8 (15.73KHz Interlace)
			EGB_resolution(work, 1, 5);
			EGB_resolution(work, 0, 5);
			break;
	}

	EGB_writePage(work, 1);
	EGB_displayStart(work, 0, 64, 0);
	EGB_displayStart(work, 2, 2, 2);
	EGB_displayStart(work, 3, 256, 240);
	EGB_writePage(work, 0);
	EGB_displayStart(work, 0, 64, 0);
	EGB_displayStart(work, 2, 2, 2);
	EGB_displayStart(work, 3, 256, 240);
	EGB_color(work, 0, 0x7fff);
	EGB_displayPage(work, 0, 3);
}

void DEF_SPR()
{
	int i;

	short SPPAT[]=
	{
		0x0000, 0x3200, 0x0023, 0x0000, 0x0000, 0x4532, 0x2354, 0x0000,
		0x2000, 0x7445, 0x5447, 0x0002, 0x5200, 0x8774, 0x4778, 0x0025,
		0x4520, 0x9c77, 0x77c9, 0x0254, 0x4730, 0xd987, 0x789d, 0x0374,
		0x5752, 0xc877, 0x778c, 0x2575, 0x6843, 0x7745, 0x5477, 0x3486,
		0x8b43, 0x4577, 0x7754, 0x34b8, 0xb852, 0x7788, 0x8877, 0x258b,
		0xc430, 0x88cb, 0xbc88, 0x034c, 0x8520, 0xccbc, 0xcbcc, 0x0258,
		0x5200, 0xeac8, 0x8cae, 0x0025, 0x2000, 0xc845, 0x548c, 0x0002,
		0x0000, 0x4532, 0x2354, 0x0000, 0x0000, 0x3200, 0x0023, 0x0000,
	};

	short PAL[]=
	{
		0+0*32+0*1024, 0+0*32+0*1024, 4+0*32+0*1024, 10+0*32+0*1024,
		14+0*32+0*1024, 16+0*32+0*1024,  17+0*32+0*1024, 18+0*32+0*1024,
		27+0*32+9*1024, 29+0*32+21*1024, 28+1*32+18*1024, 27+4*32+11*1024,
		29+4*32+14*1024, 30+16*32+26*1024, 29+29*32+30*1024, 31+31*32+31*1024
	};

	for (i = 0 ; i < 896 ; i++)
	{
		SPR_define(0, 128 + i, 1, 1, (char *)SPPAT);
	}

	for (i = 0 ; i < 256 ; i++)
	{
		SPR_setPaletteBlock(256 + i, 1, (char *)PAL);
	}

}

int main()
{
	int i;
	int joy;
	int chatter = 0;
	int n = 227, n_pre = 0;
	int fps = 0;
	int Vsync_pre;
	int mode = 0, m_pre = mode;

	unsigned short x[1024], y[1024];
	short vx[1024], vy[1024];

	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0; //sprite index
	int sprram_offset;

	char para[64];
	WORD(para+0) = 16;
	WORD(para+2) = 64;
	WORD(para+4) = 27;

	EGB_init(work, 1536);
	Display_init(mode);

	SPR_init();
	DEF_SPR();

	Vsync_Count = 0;
	HIS_stackArea( HIS_stack , stackSize );
	HIS_setHandler( VSYNCintNumber , VSYNChandler );
	HIS_enableInterrupt( VSYNCintNumber );

	for(i = 0; i < 1024; i++)
	{
		x[i] = (KurosawaRand() & 0x7f) + 80;
		y[i] = (KurosawaRand() & 0x7f) + 80;
		vx[i] = (KurosawaRand() & 7) - 3;
		vy[i] = (KurosawaRand() & 7) - 3;
		SPR_setAttribute(i, 1, 1, 128, 0x8100);
		SPR_setPosition(0, i, 1, 1, 400, 400);
	}

	EGB_writePage(work, 1);
	SPR_display(1,n);

	while(1)
	{
		SPR_display(2, 0);
		fps = Vsync_Count;
		Vsync_Count = 0;

		for(i = 0; i < n;i++)
		{
			sprram_offset = (1023 - i) << 2;
			x[i] += vx[i];
			sprram[sprram_offset++]=x[i];
			if(x[i] > 239)
			{
				vx[i] = -vx[i];
			}

			y[i] += vy[i];
			sprram[sprram_offset]=y[i];
			if(y[i] > 223)
			{
				vy[i] = -vy[i];
			}
		}

		if(n != n_pre || mode != m_pre)
		{
			n_pre = n;
			m_pre = mode;

			EGB_writePage(work, 0);
			EGB_color(work, 1, 0x8000);
			EGB_partClearScreen(work);
			if(!fps) fps = 1;

			switch(mode)
			{
				case 1:
					sprintf(para+6, "%2d fps|%4d sprites | 240p", 60/fps, n);
					break;
				case 2:
					sprintf(para+6, "%2d fps|%4d sprites | 240i", 60/fps, n);
					break;
				default:
					sprintf(para+6, "%2d fps|%4d sprites | 480p", 60/fps, n);
					break;
			}
			EGB_sjisString(work, para);
			EGB_writePage(work, 1);
		}

		SND_joy_in_1(0, &joy);

		joy = joy & 0x3f;

		if(joy == 0x3f || chatter == 1)
		{
			chatter = 0;
			continue;
		}
		
		switch(joy)
		{
			case 0x1f:
				Vsync_pre = Vsync_Count;
				mode++;
				if(mode >= 3) mode = 0;
				Display_init(mode);
				while(Vsync_Count <=6 ){}
				Vsync_Count = Vsync_pre;
				break;
			case 0x33:
				SPR_display(0, 1);
				EGB_init(work, 1536);
				HIS_detachHandler( VSYNCintNumber );
				return 0;
			case 0x2f:
				n = 227;
				break;
			case 0x37:
				n++;
				break;
			case 0x3b:
				n--;
				break;
			case 0x3e:
				n+=10;
				break;
			case 0x3d:
				n-=10;
				break;
		}

		if(n > 1024)
		{
			n = 1024;
		}
		else if(n < 1)
		{
			n = 1;
		}

		SPR_display(1, n);

		chatter = 1;

	}

}