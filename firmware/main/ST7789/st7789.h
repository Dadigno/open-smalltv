/**
 * @file    st7789.h
 * @brief   This file contains all the functions prototypes for the ST7789 driver.
 * @note    This driver is based on the ST7789 datasheet and the STM32 HAL library.
 *     The driver is designed to work with the STM32U5xx series microcontrollers.
 *     It  uses the SPI interface for communication with the ST7789 display.
 *     Supports DMA for faster data transfer.
 * 
 * @attention	You have to define pins for the display in the main.h file.
 */

#ifndef __ST7789_H
#define __ST7789_H

#include "fonts.h"


// #define USE_DMA
//#define CFG_NO_CS
// #define DMA_MIN_SIZE 16

/* Pin connection*/
#define ST7789_BL_PIN   8  // Backlight pin
#define ST7789_DC_PIN   4  //DC/RS
#define ST7789_RST_PIN  15 //Reset 
#define ST7789_SCL_PIN  6  // SPI clock pin
#define ST7789_SDA_PIN  7  // SPI data pin

#define ST7789_CS_PIN   5
#define SPI_HOST	    SPI2_HOST
#define SPI_BUS_SPEED  1000000 // SPI bus speed in Hz

/**
 * Definition of display rotation
 */
typedef enum {
	ROT_PORTRAIT = 0,
	ROT_LANDSCAPE,
	ROT_PORTRAIT_180,
	ROT_LANDSCAPE_180
}st7789_rot_t;

/**
 *Color of pen
 *If you want to use another color, you can choose one in RGB565 format.
 */

#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define GRAY        0X8430
#define BRED        0XF81F
#define GRED        0XFFE0
#define GBLUE       0X07FF
#define BROWN       0XBC40
#define BRRED       0XFC07
#define DARKBLUE    0X01CF
#define LIGHTBLUE   0X7D7C
#define GRAYBLUE    0X5458

#define LIGHTGREEN  0X841F
#define LGRAY       0XC618
#define LGRAYBLUE   0XA651
#define LBBLUE      0X2B12

/**
  * @brief  ST7789 Registers
  */
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PTLON   0x12
#define ST7789_NORON   0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR   0x30
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36


#define ST7789_RDDPM                     0x0AU   /* Read Display Power Mode */
#define ST7789_RDDMADCTL                 0x0BU   /* Read Display MADCTL */
#define ST7789_RDDCOLMOD                 0x0CU   /* Read Display Pixel Format */
#define ST7789_RDDIM                     0x0DU   /* Read Display Image Format */
#define ST7789_RDDSM                     0x0EU   /* Read Display Signal Mode */
#define ST7789_RDDSDR                    0x0FU   /* Read Display Self-Diagnostic Result */
#define ST7789_GAMMA                     0x26U   /* Gamma register */
#define ST7789_RGBSET                    0x2DU   /* Color SET */
#define ST7789_VSCRDEF                   0x33U   /* Vertical Scrolling Definition */
#define ST7789_TEOFF                     0x34U   /* Tearing Effect Line OFF */
#define ST7789_TEON                      0x35U   /* Tearing Effect Line ON */
#define ST7789_VSCRSADD                  0x37U   /* Vertical Scrolling Start Address */
#define ST7789_IDMOFF                    0x38U   /* Idle Mode OFF */
#define ST7789_IDMON                     0x39U   /* Idle Mode ON */
#define ST7789_WRITE_MEM_CONTINUE        0x3CU   /* Write Memory Continue */
#define ST7789_READ_MEM_CONTINUE         0x3EU   /* Read Memory Continue */
#define ST7789_SET_TEAR_SCANLINE         0x44U   /* Set Tear Scanline */
#define ST7789_GET_SCANLINE              0x45U   /* Get Scanline */
#define ST7789_WDB                       0x51U   /* Write Brightness Display register */
#define ST7789_RDDISBV                   0x52U   /* Read Display Brightness */
#define ST7789_WCD                       0x53U   /* Write Control Display register*/
#define ST7789_RDCTRLD                   0x54U   /* Read CTRL Display */
#define ST7789_WRCABC                    0x55U   /* Write Content Adaptive Brightness Control */
#define ST7789_RDCABC                    0x56U   /* Read Content Adaptive Brightness Control */
#define ST7789_WRITE_CABC                0x5EU   /* Write CABC Minimum Brightness */
#define ST7789_READ_CABC                 0x5FU   /* Read CABC Minimum Brightness */
#define ST7789_READ_ABCSDR               0x68U   /* Read Automatic Brightness Control Self-Diagnostic Result */
#define ST7789_READ_ID1                  0xDAU   /* Read ID1 */
#define ST7789_READ_ID2                  0xDBU   /* Read ID2 */
#define ST7789_READ_ID3                  0xDCU   /* Read ID3 */


#define ST7789_RAM_CTRL                  0xB0U   /* RAM Control */
#define ST7789_RGB_INTERFACE_CTRL        0xB1U   /* Porch Setting */
#define ST7789_PORCH_CTRL                0xB2U   /* RGB Interface Signal Control */
#define ST7789_FRAME_RATE_CTRL1          0xB3U   /* Frame Rate Control 1 (In partial mode/ idle colors)) */
#define ST7789_PARTIAL_CTRL              0xB5U   /* N/A */
#define ST7789_GATE_CTRL                 0xB7U   /* Gate Control */
#define ST7789_GATE_TIMING_ADJUSTMENT    0xB8U   /* Timing Adjustement */
#define ST7789_DIGITAL_GAMMA_ENABLE      0xBAU   /* Digital Gamma Enable */
#define ST7789_VCOM_SET                  0xBBU   /* VCOM Setting */
#define ST7789_PWR_SAVING_MODE           0xBCU   /* LCM Control */
#define ST7789_DISPLAY_OFF_PWR_SAVE      0xBDU   /* N/A */
#define ST7789_LCM_CTRL                  0xC0U   /* N/A */
#define ST7789_ID_CODE_SETTING           0xC1U   /* ID Code Setting */
#define ST7789_VDV_VRH_EN                0xC2U   /* VDV and VRH Command Enable */
#define ST7789_VRH_SET                   0xC3U   /* VRH Set */
#define ST7789_VDV_SET                   0xC4U   /* VDV Set */
#define ST7789_VCOMH_OFFSET_SET          0xC5U   /* VCOM Offset Set */
#define ST7789_FRAME_RATE_CTRL2          0xC6U   /* Frame Rate Control 2 (In Normal Mode) */
#define ST7789_CABC_CTRL                 0xC7U   /* CABC Control */
#define ST7789_REG_VALUE_SELECTION1      0xC8U   /* Register Value Selection 1 */
#define ST7789_REG_VALUE_SELECTION2      0xCAU   /* Register Value Selection 2 */
#define ST7789_PWM_FREQ_SELECTION        0xCCU   /* PWM Frequency Selection */
#define ST7789_POWER_CTRL                0xD0U   /* Power Control 1 */
#define ST7789_EN_VAP_VAN_SIGNAL_OUTPUT  0xD2U   /* Enable VAP/VAN signal output */
#define ST7789_COMMAND2_ENABLE           0xDFU   /* Command 2 Enable */
#define ST7789_PV_GAMMA_CTRL             0xE0U   /* Positive Voltage Gamma Control */
#define ST7789_NV_GAMMA_CTRL             0xE1U   /* Negative Voltage Gamma Control */
#define ST7789_GAMMA_RED_TABLE           0xE2U   /* Digital Gamma Look-up Table for Red */
#define ST7789_GAMMA_BLUE_TABLE          0xE3U   /* Digital Gamma Look-up Table for Blue */
#define ST7789_GATE_CTRL2                0xE4U   /* Gate Control */
#define ST7789_SPI2_ENABLE               0xE7U   /* SPI2 Enable */
#define ST7789_PWR_CTRL2                 0xE8U   /* Power Control 2 */
#define ST7789_EQUALIZE_TIME_CTRL        0xE9U   /* Equalize time control */
#define ST7789_PROGRAM_MODE_CTRL         0xECU   /* Program Mode Control */
#define ST7789_PROGRAM_MODE_ENABLE       0xFAU   /* Program Mode Enable */
#define ST7789_NVM_SETTING               0xFCU   /* NVM Setting */
#define ST7789_PROGRAM_ACTION            0xFEU   /* Program action */


/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */

/* Page Address Order ('0': Top to Bottom, '1': the opposite) */
#define ST7789_MADCTL_MY  0x80
/* Column Address Order ('0': Left to Right, '1': the opposite) */
#define ST7789_MADCTL_MX  0x40
/* Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode) */
#define ST7789_MADCTL_MV  0x20
/* Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite) */
#define ST7789_MADCTL_ML  0x10
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST7789_MADCTL_RGB 0x00

#define ST7789_RDID1   0xDA
#define ST7789_RDID2   0xDB
#define ST7789_RDID3   0xDC
#define ST7789_RDID4   0xDD

/* Advanced options */
#define ST7789_COLOR_MODE_16bit 0x55    //  RGB565 (16bit)
#define ST7789_COLOR_MODE_18bit 0x66    //  RGB666 (18bit)

/* Chip Reset macro definition */
#define ST7789_RST_Clr()                       asm("nop")
#define ST7789_RST_Set()                       asm("nop")

/* Chip Select macro definition */
#define ST7789_Select()                        asm("nop")
#define ST7789_UnSelect()                      asm("nop")

/* Data/Command macro definition */
#define ST7789_DC_Set()                        gpio_set_level(ST7789_DC_PIN, 1) //GPIO_PIN_SET
#define ST7789_DC_Clr()                        gpio_set_level(ST7789_DC_PIN, 0) //GPIO_PIN_RESET

#define ABS(x) ((x) > 0 ? (x) : -(x))

/* Basic functions. */
void ST7789_Init(uint16_t height, uint16_t width, uint8_t rot);
void ST7789_SetRotation(uint8_t m);
void ST7789_Fill_Color(uint16_t color);
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color);

/* Graphical functions. */
void ST7789_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);
void ST7789_InvertColors(uint8_t invert);

/* Text functions. */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor);

/* Extented Graphical functions. */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/* Command functions */
void ST7789_TearEffect(uint8_t tear);
void ST7789_VerticalScroll(uint16_t pixel);

/* Simple test function. */
void ST7789_Test(void);


#endif
