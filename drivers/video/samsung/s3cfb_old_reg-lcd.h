/*
 */


//#include <mach/regs-gpio.h>

#include <plat/map-base.h>
#define S3C_VA_LCD	S3C_ADDR(0x00600000)	/* LCD */
/***************************************************************************/
/* LCD Registers for S3C2443/2450/S3C6400/6410 */
#define S3C_LCDREG(x)		((x) + lcd_regs)

/* LCD control registers */
#define S3C_VIDCON0		S3C_LCDREG(0x00)  	/* Video control 0 register */
#define S3C_VIDCON1		S3C_LCDREG(0x04)  	/* Video control 1 register */

#define S3C_VIDCON2		S3C_LCDREG(0x08)  	/* Video control 2 register */
#define S3C_PRTCON		S3C_LCDREG(0x0C)	/* Protection control register */
#define S3C_VIDTCON0		S3C_LCDREG(0x10)  	/* Video time control 0 register */
#define S3C_VIDTCON1		S3C_LCDREG(0x14)  	/* Video time control 1 register */
#define S3C_VIDTCON2		S3C_LCDREG(0x18)  	/* Video time control 2 register */
#define S3C_VIDTCON3		S3C_LCDREG(0x1C)  	/* Video time control 3 register */

#define S3C_WINCON0		S3C_LCDREG(0x20)  	/* Window control 0 register */
#define S3C_WINCON1		S3C_LCDREG(0x24)  	/* Window control 1 register */
#define S3C_WINCON2		S3C_LCDREG(0x28)  	/* Window control 2 register */
#define S3C_WINCON3		S3C_LCDREG(0x2C)  	/* Window control 3 register */
#define S3C_WINCON4		S3C_LCDREG(0x30)  	/* Window control 4 register*/


#define S3C_VIDOSD0A		S3C_LCDREG(0x40)  	/* Video Window 0 position control register */
#define S3C_VIDOSD0B		S3C_LCDREG(0x44)  	/* Video Window 0 position control register1 */
#define S3C_VIDOSD0C		S3C_LCDREG(0x48)  	/* Video Window 0 position control register */

#define S3C_VIDOSD1A		S3C_LCDREG(0x50)  	/* Video Window 1 position control register */
#define S3C_VIDOSD1B		S3C_LCDREG(0x54)  	/* Video Window 1 position control register */
#define S3C_VIDOSD1C		S3C_LCDREG(0x58)  	/* Video Window 1 position control register */
#define S3C_VIDOSD1D		S3C_LCDREG(0x5C)  	/* Video Window 1 position control register */

#define S3C_VIDOSD2A		S3C_LCDREG(0x60)  	/* Video Window 2 position control register */
#define S3C_VIDOSD2B		S3C_LCDREG(0x64)  	/* Video Window 2 position control register */
#define S3C_VIDOSD2C		S3C_LCDREG(0x68)  	/* Video Window 2 position control register */
#define S3C_VIDOSD2D		S3C_LCDREG(0x6C)  	/* Video Window 2 position control register */

#define S3C_VIDOSD3A		S3C_LCDREG(0x70)  	/* Video Window 3 position control register */
#define S3C_VIDOSD3B		S3C_LCDREG(0x74)  	/* Video Window 3 position control register */
#define S3C_VIDOSD3C		S3C_LCDREG(0x78)  	/* Video Window 3 position control register */

#define S3C_VIDOSD4A		S3C_LCDREG(0x80)  	/* Video Window 4 position control register */
#define S3C_VIDOSD4B		S3C_LCDREG(0x84)  	/* Video Window 4 position control register */
#define S3C_VIDOSD4C		S3C_LCDREG(0x88)  	/* Video Window 4 position control register */

#define S3C_VIDW00ADD2B0	S3C_LCDREG(0x94)  	/* LCD CONTROL 1 */
#define S3C_VIDW00ADD2B1	S3C_LCDREG(0x98)  	/* LCD CONTROL 1 */

#define S3C_VIDW00ADD0B0	S3C_LCDREG(0x0A0) 	/* Window 0 buffer start address register, buffer 0 */
#define S3C_VIDW00ADD0B1	S3C_LCDREG(0x0A4) 	/* Window 0 buffer start address register, buffer 1 */
#define S3C_VIDW01ADD0B0	S3C_LCDREG(0x0A8) 	/* Window 1 buffer start address register, buffer 0 */
#define S3C_VIDW01ADD0B1	S3C_LCDREG(0x0AC) 	/* Window 1 buffer start address register, buffer 1 */
#define S3C_VIDW02ADD0		S3C_LCDREG(0x0B0) 	/* Window 2 buffer start address register */
#define S3C_VIDW03ADD0		S3C_LCDREG(0x0B8) 	/* Window 3 buffer start address register */
#define S3C_VIDW04ADD0		S3C_LCDREG(0x0C0) 	/* Window 4 buffer start address register */
#define S3C_VIDW00ADD1B0	S3C_LCDREG(0x0D0) 	/* Window 0 buffer end address register, buffer 0 */
#define S3C_VIDW00ADD1B1	S3C_LCDREG(0x0D4) 	/* Window 0 buffer end address register, buffer 1 */
#define S3C_VIDW01ADD1B0	S3C_LCDREG(0x0D8) 	/* Window 1 buffer end address register, buffer 0 */
#define S3C_VIDW01ADD1B1	S3C_LCDREG(0x0DC) 	/* Window 1 buffer end address register, buffer 1 */
#define S3C_VIDW02ADD1		S3C_LCDREG(0x0E0) 	/* Window 2 buffer end address register */
#define S3C_VIDW03ADD1		S3C_LCDREG(0x0E8) 	/* Window 3 buffer end address register */
#define S3C_VIDW04ADD1		S3C_LCDREG(0x0F0) 	/* Window 4 buffer end address register */
#define S3C_VIDW00ADD2		S3C_LCDREG(0x100) 	/* Window 0 buffer size register */
#define S3C_VIDW01ADD2		S3C_LCDREG(0x104) 	/* Window 1 buffer size register */

#define S3C_VIDW02ADD2		S3C_LCDREG(0x108) 	/* Window 2 buffer size register */
#define S3C_VIDW03ADD2		S3C_LCDREG(0x10C) 	/* Window 3 buffer size register */
#define S3C_VIDW04ADD2		S3C_LCDREG(0x110) 	/* Window 4 buffer size register */

#define S3C_VIDINTCON0		S3C_LCDREG(0x130)	/* Indicate the Video interrupt control register */
#define S3C_VIDINTCON1		S3C_LCDREG(0x134) 	/* Video Interrupt Pending register */
#define S3C_W1KEYCON0		S3C_LCDREG(0x140) 	/* Color key control register */
#define S3C_W1KEYCON1		S3C_LCDREG(0x144) 	/* Color key value ( transparent value) register */
#define S3C_W2KEYCON0		S3C_LCDREG(0x148) 	/* Color key control register */
#define S3C_W2KEYCON1		S3C_LCDREG(0x14C) 	/* Color key value (transparent value) register */

#define S3C_W3KEYCON0		S3C_LCDREG(0x150)	/* Color key control register	*/
#define S3C_W3KEYCON1		S3C_LCDREG(0x154)	/* Color key value (transparent value) register	*/
#define S3C_W4KEYCON0		S3C_LCDREG(0x158)	/* Color key control register	*/
#define S3C_W4KEYCON1		S3C_LCDREG(0x15C)	/* Color key value (transparent value) register	*/
#define S3C_DITHMODE		S3C_LCDREG(0x170)	/* Dithering mode register.	*/

#define S3C_WIN0MAP		S3C_LCDREG(0x180)	/* Window color control	*/
#define S3C_WIN1MAP		S3C_LCDREG(0x184)	/* Window color control	*/
#define S3C_WIN2MAP		S3C_LCDREG(0x188)	/* Window color control	*/
#define S3C_WIN3MAP		S3C_LCDREG(0x18C)	/* Window color control	*/
#define S3C_WIN4MAP		S3C_LCDREG(0x190)	/* Window color control	*/
#define S3C_WPALCON		S3C_LCDREG(0x1A0)	/* Window Palette control register	*/

#define S3C_TRIGCON		S3C_LCDREG(0x1A4)	/* I80 / RGB Trigger Control Regiter	*/
#define S3C_I80IFCONA0		S3C_LCDREG(0x1B0)	/* I80 Interface control 0 for Main LDI	*/
#define S3C_I80IFCONA1		S3C_LCDREG(0x1B4)	/* I80 Interface control 0 for Sub LDI	*/
#define S3C_I80IFCONB0		S3C_LCDREG(0x1B8)	/* I80 Inteface control 1 for Main LDI	*/
#define S3C_I80IFCONB1		S3C_LCDREG(0x1BC)	/* I80 Inteface control 1 for Sub LDI	*/
#define S3C_LDI_CMDCON0		S3C_LCDREG(0x1D0)	/* I80 Interface LDI Command Control 0	*/
#define S3C_LDI_CMDCON1		S3C_LCDREG(0x1D4)	/* I80 Interface LDI Command Control 1	*/
#define S3C_SIFCCON0		S3C_LCDREG(0x1E0)	/* LCD i80 System Interface Command Control 0	*/
#define S3C_SIFCCON1		S3C_LCDREG(0x1E4)	/* LCD i80 System Interface Command Control 1	*/
#define S3C_SIFCCON2		S3C_LCDREG(0x1E8)	/* LCD i80 System Interface Command Control 2	*/

#define S3C_LDI_CMD0		S3C_LCDREG(0x280)	/* I80 Inteface LDI Command 0	*/
#define S3C_LDI_CMD1		S3C_LCDREG(0x284)	/* I80 Inteface LDI Command 1	*/
#define S3C_LDI_CMD2		S3C_LCDREG(0x288)	/* I80 Inteface LDI Command 2	*/
#define S3C_LDI_CMD3		S3C_LCDREG(0x28C)	/* I80 Inteface LDI Command 3	*/
#define S3C_LDI_CMD4		S3C_LCDREG(0x290)	/* I80 Inteface LDI Command 4	*/
#define S3C_LDI_CMD5		S3C_LCDREG(0x294)	/* I80 Inteface LDI Command 5	*/
#define S3C_LDI_CMD6		S3C_LCDREG(0x298)	/* I80 Inteface LDI Command 6	*/
#define S3C_LDI_CMD7		S3C_LCDREG(0x29C)	/* I80 Inteface LDI Command 7	*/
#define S3C_LDI_CMD8		S3C_LCDREG(0x2A0)	/* I80 Inteface LDI Command 8	*/
#define S3C_LDI_CMD9		S3C_LCDREG(0x2A4)	/* I80 Inteface LDI Command 9	*/
#define S3C_LDI_CMD10		S3C_LCDREG(0x2A8)	/* I80 Inteface LDI Command 10	*/
#define S3C_LDI_CMD11		S3C_LCDREG(0x2AC)	/* I80 Inteface LDI Command 11	*/

#define S3C_W2PDATA01		S3C_LCDREG(0x300)	/* Window 2 Palette Data of the Index 0,1	*/
#define S3C_W2PDATA23		S3C_LCDREG(0x304)	/* Window 2 Palette Data of the Index 2,3	*/
#define S3C_W2PDATA45		S3C_LCDREG(0x308)	/* Window 2 Palette Data of the Index 4,5	*/
#define S3C_W2PDATA67		S3C_LCDREG(0x30C)	/* Window 2 Palette Data of the Index 6,7	*/
#define S3C_W2PDATA89		S3C_LCDREG(0x310)	/* Window 2 Palette Data of the Index 8,9	*/
#define S3C_W2PDATAAB		S3C_LCDREG(0x314)	/* Window 2 Palette Data of the Index A, B	*/
#define S3C_W2PDATACD		S3C_LCDREG(0x318)	/* Window 2 Palette Data of the Index C, D	*/
#define S3C_W2PDATAEF		S3C_LCDREG(0x31C)	/* Window 2 Palette Data of the Index E, F	*/
#define S3C_W3PDATA01		S3C_LCDREG(0x320)	/* Window 3 Palette Data of the Index 0,1	*/
#define S3C_W3PDATA23		S3C_LCDREG(0x324)	/* Window 3 Palette Data of the Index 2,3	*/
#define S3C_W3PDATA45		S3C_LCDREG(0x328)	/* Window 3 Palette Data of the Index 4,5	*/
#define S3C_W3PDATA67		S3C_LCDREG(0x32C)	/* Window 3 Palette Data of the Index 6,7	*/
#define S3C_W3PDATA89		S3C_LCDREG(0x330)	/* Window 3 Palette Data of the Index 8,9	*/
#define S3C_W3PDATAAB		S3C_LCDREG(0x334)	/* Window 3 Palette Data of the Index A, B	*/
#define S3C_W3PDATACD		S3C_LCDREG(0x338)	/* Window 3 Palette Data of the Index C, D	*/
#define S3C_W3PDATAEF		S3C_LCDREG(0x33C)	/* Window 3 Palette Data of the Index E, F	*/
#define S3C_W4PDATA01		S3C_LCDREG(0x340)	/* Window 3 Palette Data of the Index 0,1	*/
#define S3C_W4PDATA23		S3C_LCDREG(0x344)	/* Window 3 Palette Data of the Index 2,3	*/

#define S3C_TFTPAL2(x)		S3C_LCDREG((0x300 + (x)*4))
#define S3C_TFTPAL3(x) 		S3C_LCDREG((0x320 + (x)*4))
#define S3C_TFTPAL4(x)		S3C_LCDREG((0x340 + (x)*4))
#define S3C_TFTPAL0(x)		S3C_LCDREG((0x400 + (x)*4))
#define S3C_TFTPAL1(x)		S3C_LCDREG((0x800 + (x)*4))

/* Window 0~4 Position Control A register - VIDOSDxA */
#define S3C_VIDOSDxA_OSD_LTX_F(x)			(((x)&0x7FF)<<11)
#define S3C_VIDOSDxA_OSD_LTY_F(x)			(((x)&0x7FF)<<0)

/* Window 0~4 Position Control B register - VIDOSDxB */
#define S3C_VIDOSDxB_OSD_RBX_F(x)			(((x)&0x7FF)<<11)
#define S3C_VIDOSDxB_OSD_RBY_F(x)			(((x)&0x7FF)<<0)

/* Window 0 Position Control C register - VIDOSD0C */
#define  S3C_VIDOSD0C_OSDSIZE(x)			(((x)&0xFFFFFF)<<0)


/* Window 1~2 Position Control D register - VIDOSDxD */
#define  S3C_VIDOSDxD_OSDSIZE(x)			(((x)&0xFFFFFF)<<0)

/* Frame buffer Start Address register - VIDWxxADD0 */
#define S3C_VIDWxxADD0_VBANK_F(x) 			(((x)&0xFF)<<23) /* the end address of the LCD frame buffer. */
#define S3C_VIDWxxADD0_VBASEU_F(x)			(((x)&0xFFFFFF)<<0) /* Virtual screen offset size (the number of byte). */

/* Frame buffer End Address register - VIDWxxADD1 */
#define S3C_VIDWxxADD1_VBASEL_F(x) 			(((x)&0xFFFFFF)<<0)  /* the end address of the LCD frame buffer. */

/* Frame buffer Size register - VIDWxxADD2 */
#define S3C_VIDWxxADD2_OFFSIZE_F(x)  			(((x)&0x1FFF)<<13) /* Virtual screen offset size (the number of byte). */
#define S3C_VIDWxxADD2_PAGEWIDTH_F(x)			(((x)&0x1FFF)<<0) /* Virtual screen page width (the number of byte). */

#define S3C_VIDCON0_VIDOUT(x)  				(((x)&0x7)<<26)
#define S3C_VIDCON0_VIDOUT_RGB_IF			(0<<26)
#define S3C_VIDCON0_VIDOUT_TV				(1<<26)
#define S3C_VIDCON0_VIDOUT_I80IF0			(2<<26)
#define S3C_VIDCON0_VIDOUT_I80IF1			(3<<26)
#define S3C_VIDCON0_VIDOUT_TVNRGBIF 			(4<<26)
#define S3C_VIDCON0_VIDOUT_TVNI80IF0			(6<<26)
#define S3C_VIDCON0_VIDOUT_TVNI80IF1			(7<<26)
#define S3C_VIDCON0_VIDOUT_MASK				(7<<26)

#define S3C64XX_GPICON			(S3C64XX_GPI_BASE + 0x00)
#define S3C64XX_GPIDAT			(S3C64XX_GPI_BASE + 0x04)
#define S3C64XX_GPIPUD			(S3C64XX_GPI_BASE + 0x08)

#define S3C64XX_GPJCON			(S3C64XX_GPJ_BASE + 0x00)
#define S3C64XX_GPJDAT			(S3C64XX_GPJ_BASE + 0x04)
#define S3C64XX_GPJPUD			(S3C64XX_GPJ_BASE + 0x08)

struct struct_plat_log_mark {

	u32 special_mark_1;

	u32 special_mark_2;

	u32 special_mark_3;

	u32 special_mark_4;

	void *p_main;

	void *p_radio;

	void *p_events;
};

struct struct_kernel_log_mark {

	u32 special_mark_1;

	u32 special_mark_2;

	u32 special_mark_3;

	u32 special_mark_4;

	void *p__log_buf;

};

struct struct_frame_buf_mark {

	u32 special_mark_1;

	u32 special_mark_2;

	u32 special_mark_3;

	u32 special_mark_4;

	void *p_fb;
	u32 resX;
	u32 resY;
	u32 bpp;
	u32 frames;
};

struct struct_marks_ver_mark {

	u32 special_mark_1;

	u32 special_mark_2;

	u32 special_mark_3;

	u32 special_mark_4;

	u32 log_mark_version;

	u32 framebuffer_mark_version;
};

