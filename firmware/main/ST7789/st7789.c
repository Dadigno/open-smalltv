/**
 * @file    st7789.c
 * @brief   ST7789 driver for STM32
 * @details This driver is for ST7789 display controller.
 * 		It supports SPI interface and DMA transfer.
 * 		Please refer to the datasheet of ST7789 for more information.
 * 
 * 
 */

#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "st7789.h"

const char * TAG = "ST7789";

typedef struct {
	spi_device_handle_t hspi;

	uint16_t width;			// Width of display
	uint16_t height;		// Height of display
	st7789_rot_t rotation;	// Rotation of display

	uint16_t *disp_buf;	// Buffer for DMA transfer
}st7789_ctx_t;


st7789_ctx_t st7789_ctx;

/**
 * @brief Scrivo un registro 
 * 
 * @param dev_handle 
 * @param reg 
 * @param recv 
 * @param len 
 */
void spi_WriteReg(spi_device_handle_t dev_handle, uint8_t* buf, uint16_t len)
{    
    // spi_device_acquire_bus(dev_handle, portMAX_DELAY);
    spi_transaction_t SpiTransaction;
    memset(&SpiTransaction, 0, sizeof(SpiTransaction));			//Zero out the transaction ready to use
    SpiTransaction.length = len * 8;								//Transaction length is in bits
    SpiTransaction.tx_buffer = buf;	                        //Set the tx data buffer
    SpiTransaction.flags = 0;					
    spi_device_polling_transmit(dev_handle, &SpiTransaction);		//Waits until the transfer is complete
    // spi_device_release_bus(dev_handle);
}

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void ST7789_WriteCommand(uint8_t * cmd, uint16_t size)
{
	ST7789_DC_Clr();
	spi_WriteReg(st7789_ctx.hspi, cmd, size);
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void ST7789_WriteData(uint8_t *buff, size_t buff_size)
{
	ST7789_DC_Set();
	spi_WriteReg(st7789_ctx.hspi, buff, buff_size);
	ST7789_DC_Clr();

}

/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST7789_WriteSmallData(uint8_t data)
{
	ST7789_Select();
	ST7789_DC_Set();
	spi_WriteReg(st7789_ctx.hspi, &data, sizeof(data));
	ST7789_UnSelect();
}

/**
 * @brief Write command to ST7789 controller
 * @param reg Register to read
 * @param recv Receive buffer data
 * @param size Number of byte to receive
 * @return none
 */
// static void ST7789_ReadData(uint8_t reg, uint8_t *recv, uint32_t size)
// {

// 	// ST7789_Select();
// 	ST7789_DC_Clr();
// 	uint32_t ret = HAL_SPI_Transmit(st7789_ctx.hspi, &reg, 1, HAL_MAX_DELAY);
// 	if(ret)
// 		Error_Handler();

// //	ST7789_DC_Clr();
// 	ret = HAL_SPI_Receive(st7789_ctx.hspi, recv, size, HAL_MAX_DELAY);

// 	ST7789_UnSelect();

// 	if(ret)
// 		Error_Handler();

// 	//Restore SPI prescaler
// 	// st7789_ctx.hspi->Instance->CR1 |= pres;
// }

void MemsetBuffer(uint16_t *buf, uint16_t data, uint32_t size)
{
	while(size--)
		*(buf++) = data;

}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_SetRotation(uint8_t m)
{
	uint8_t reg = ST7789_MADCTL;
	ST7789_WriteCommand(&reg, 1);	// MADCTL
	switch (m) {
	case 0:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
		break;
	case 1:
		ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	case 2:
		ST7789_WriteSmallData(ST7789_MADCTL_RGB);
		break;
	case 3:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	default:
		break;
	}
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	ST7789_Select();
	uint16_t x_start = x0 , x_end = x1;
	uint16_t y_start = y0 , y_end = y1;
	
	uint8_t reg = ST7789_CASET;

	/* Column Address set */
	ST7789_WriteCommand(&reg, 1);
	{
		uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}

	/* Row Address set */
	reg = ST7789_RASET;
	ST7789_WriteCommand(&reg, 1);
	{
		uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}
	/* Write to RAM */
	reg = ST7789_RAMWR;
	ST7789_WriteCommand(&reg, 1);
	ST7789_UnSelect();
}

/**
 * @brief Initialize ST7789 controller
 * @param height Display height as you see
 * @param width Display width as you see
 * @param rot Displat orientation, landscape ore portrait
 * @return none
 */
void ST7789_Init(uint16_t height, uint16_t width, uint8_t rot)
{
	//Init spi
	gpio_config_t io_output_conf = {};
    io_output_conf.intr_type = GPIO_INTR_DISABLE;
    io_output_conf.mode = GPIO_MODE_OUTPUT;
    io_output_conf.pin_bit_mask = (1ULL<<ST7789_DC_PIN);
    io_output_conf.pull_up_en = 0;

    gpio_config(&io_output_conf);

    spi_bus_config_t buscfg = {
        .miso_io_num = NULL,
        .mosi_io_num = ST7789_SDA_PIN,
        .sclk_io_num = ST7789_SCL_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    //Initialize the SPI bus
	
    uint32_t ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t SpiDeviceCfg = {
		.clock_speed_hz = SPI_BUS_SPEED,				//Clock out at 10 MHz
		.mode = 0,								//<<< SPI mode 0
		.spics_io_num = ST7789_CS_PIN,			        //<<< CS pin number
		.queue_size = 1,						//<<< Number of transactions we want to be able to queue at a time using spi_device_queue_trans()
	};
	spi_bus_add_device(SPI_HOST, &SpiDeviceCfg, &st7789_ctx.hspi);		//<<<You can use ESP_ERROR_CHECK() on this call if you are having issues
	


	//Probe device
    // uint8_t recv[] = {0,0,0,0};
    // ST7789_ReadData(ST7789_RDDID, recv, 4);
	// printf("ST7789 ID: %02X %02X %02X %02X\r\n", recv[0], recv[1], recv[2], recv[3]);

	/* Select dimension */
	st7789_ctx.width = width;
	st7789_ctx.height = height;
	st7789_ctx.rotation = rot;

	if(st7789_ctx.rotation == ROT_PORTRAIT_180 || st7789_ctx.rotation == ROT_PORTRAIT)
	{
		st7789_ctx.width = height;
		st7789_ctx.height = width;
	}


	#ifdef USE_DMA
		/* Allocate buffer for DMA transfer */
		st7789_ctx.disp_buf = (uint16_t *)calloc(st7789_ctx.width * st7789_ctx.height, sizeof(uint16_t));
		if(st7789_ctx.disp_buf == NULL)
			Error_Handler();
	#endif

	

    uint8_t reg = ST7789_RAMWR;

    ST7789_WriteCommand(&reg, 1);
    vTaskDelay(10);

    reg = ST7789_SWRESET;
    ST7789_WriteCommand(&reg, 1);
    vTaskDelay(20);

    reg = ST7789_SLPOUT;
    ST7789_WriteCommand(&reg, 1);
    vTaskDelay(120);

    reg = ST7789_DISPON;
  	ST7789_WriteCommand (&reg, 1);	//	Main screen turned on
	vTaskDelay(10);

    reg = ST7789_NORON;
    ST7789_WriteCommand (&reg, 1);		//	Normal Display on
    vTaskDelay(10);

    reg = ST7789_RAM_CTRL;
    ST7789_WriteCommand(&reg, 1);
    ST7789_WriteSmallData(0x00);
    ST7789_WriteSmallData(0xF0); //F8 per big endian

	ST7789_SetRotation(st7789_ctx.rotation);	//	MADCTL (Display Rotation)

	reg = ST7789_COLMOD;
    ST7789_WriteCommand(&reg, 1);		//	Set color mode
    ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);

    reg = ST7789_FRAME_RATE_CTRL2;
	ST7789_WriteCommand (&reg, 1);				//	Frame rate control in normal mode
	ST7789_WriteSmallData (0x0F);			//	Default value (60HZ)

	reg = ST7789_PORCH_CTRL;
  	ST7789_WriteCommand(&reg, 1);				//	Porch control
	{
		uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		ST7789_WriteData(data, sizeof(data));
	}
	
	/* Internal LCD Voltage generator settings */
	reg = ST7789_GATE_CTRL;
    ST7789_WriteCommand(&reg , 1);				//	Gate Control
    ST7789_WriteSmallData(0x35);			//	Default value
    reg = ST7789_VCOM_SET;
    ST7789_WriteCommand(&reg, 1);				//	VCOM setting
    ST7789_WriteSmallData(0x1F);			//0x19	0.725v (default 0.75v for 0x20)
    reg = ST7789_LCM_CTRL;
    ST7789_WriteCommand(&reg, 1);				//	LCMCTRL
    ST7789_WriteSmallData (0x2C);			//	Default value

    reg = ST7789_VDV_VRH_EN;
    ST7789_WriteCommand (&reg, 1);				//	VDV and VRH command Enable
    {
        	uint8_t data[] = {0x01, 0xC3}; //LITTLE ENDIAN
        	ST7789_WriteData(data, sizeof(data));
    }

//    ST7789_WriteCommand (ST7789_VRH_SET);				//	VRH set
//    ST7789_WriteSmallData (0x12);			//	+-4.45v (defalut +-4.1v for 0x0B)
    reg = ST7789_VDV_SET;
    ST7789_WriteCommand (&reg, 1);				//	VDV set
    ST7789_WriteSmallData (0x20);			//	Default value

    reg = ST7789_POWER_CTRL;
    ST7789_WriteCommand (&reg, 1);				//	Power control
    ST7789_WriteSmallData (0xA4);			//	Default value
    ST7789_WriteSmallData (0xA1);			//	Default value
	/**************** Division line ****************/

    reg = ST7789_PV_GAMMA_CTRL;
	ST7789_WriteCommand(&reg, 1);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
//		uint8_t data[] = {0xD0, 0x08, 0x11, 0x08, 0x0C, 0x15, 0x39, 0x33, 0x50, 0x36, 0x13, 0x14, 0x29, 0x2D};
		ST7789_WriteData(data, sizeof(data));
	}

	reg = ST7789_NV_GAMMA_CTRL;
    ST7789_WriteCommand(&reg, 1);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
//    	uint8_t data[] = {0xD0, 0x08, 0x10, 0x08, 0x06, 0x06, 0x39, 0x44, 0x51, 0x0B, 0x16, 0x14, 0x2F, 0x31};
		ST7789_WriteData(data, sizeof(data));
	}

	reg = ST7789_INVON;
    ST7789_WriteCommand (&reg, 1);		//	Inversion ON

	reg = ST7789_TEON;
	ST7789_WriteCommand (&reg, 1);		//	Tear effect ON
	ST7789_WriteSmallData(0x00);

    reg = ST7789_DISPON;
  	ST7789_WriteCommand (&reg, 1);	//	Main screen turned on
	vTaskDelay(100);

    ST7789_Fill_Color(BLACK);				//	Fill with Black.
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill_Color(uint16_t color)
{
	ST7789_SetAddressWindow(0, 0, st7789_ctx.width, st7789_ctx.height);
	ST7789_Select();

	#ifdef USE_DMA //Usa un unico trasferimento nel caso di DMA
		MemsetBuffer(st7789_ctx.disp_buf, color, sizeof(st7789_ctx.disp_buf)/2);
		ST7789_WriteData((uint8_t*)st7789_ctx.disp_buf, sizeof(st7789_ctx.disp_buf));
	#else
		uint16_t j=0,i=0;
		for (i = 0; i < st7789_ctx.width; i++)
				for (j = 0; j < st7789_ctx.height; j++) {
					uint8_t data[] = {color >> 8, color & 0xFF};
					ST7789_WriteData(data, sizeof(data));
				}
	#endif
	ST7789_UnSelect();
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x >= st7789_ctx.width) || (y >= st7789_ctx.height))	
		return;
	
	ST7789_SetAddressWindow(x, y, x, y);
	uint8_t data[] = {color >> 8, color & 0xFF};
	ST7789_Select();
	ST7789_WriteData(data, sizeof(data));
	ST7789_UnSelect();
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
	if ((xEnd >= st7789_ctx.width) || (yEnd >= st7789_ctx.height))	
		return;
	ST7789_Select();
	uint16_t i, j;
	ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
	for (i = ySta; i <= yEnd; i++)
		for (j = xSta; j <= xEnd; j++) {
			uint8_t data[] = {color >> 8, color & 0xFF};
			ST7789_WriteData(data, sizeof(data));
		}
	ST7789_UnSelect();
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x <= 0) || (x > st7789_ctx.width) ||
		 (y <= 0) || (y > st7789_ctx.height))	return;
	ST7789_Select();
	ST7789_Fill(x - 1, y - 1, x + 1, y + 1, color);
	ST7789_UnSelect();
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        uint16_t color) {
	uint16_t swap;
    uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
		swap = x0;
		x0 = y0;
		y0 = swap;

		swap = x1;
		x1 = y1;
		y1 = swap;
        //_swap_int16_t(x0, y0);
        //_swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
		swap = x0;
		x0 = x1;
		x1 = swap;

		swap = y0;
		y0 = y1;
		y1 = swap;
        //_swap_int16_t(x0, x1);
        //_swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = ABS(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            ST7789_DrawPixel(y0, x0, color);
        } else {
            ST7789_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	ST7789_Select();
	ST7789_DrawLine(x1, y1, x2, y1, color);
	ST7789_DrawLine(x1, y1, x1, y2, color);
	ST7789_DrawLine(x1, y2, x2, y2, color);
	ST7789_DrawLine(x2, y1, x2, y2, color);
	ST7789_UnSelect();
}

/** 
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_Select();
	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawPixel(x0 + x, y0 + y, color);
		ST7789_DrawPixel(x0 - x, y0 + y, color);
		ST7789_DrawPixel(x0 + x, y0 - y, color);
		ST7789_DrawPixel(x0 - x, y0 - y, color);

		ST7789_DrawPixel(x0 + y, y0 + x, color);
		ST7789_DrawPixel(x0 - y, y0 + x, color);
		ST7789_DrawPixel(x0 + y, y0 - x, color);
		ST7789_DrawPixel(x0 - y, y0 - x, color);
	}
	ST7789_UnSelect();
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
{
	if ((x >= st7789_ctx.width) || (y >= st7789_ctx.height))
		return;
	if ((x + w - 1) >= st7789_ctx.width)
		return;
	if ((y + h - 1) >= st7789_ctx.height)
		return;

	ST7789_Select();
	ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);
	ST7789_WriteData((uint8_t *)data, 2 * w * h);
	ST7789_UnSelect();
}

/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST7789_InvertColors(uint8_t invert)
{
	ST7789_Select();
	uint8_t reg = invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */;
	ST7789_WriteCommand(&reg, 1);
	ST7789_UnSelect();
}

/** 
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
	uint32_t i, b, j;
	ST7789_Select();
	ST7789_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

	for (i = 0; i < font.height; i++) {
		b = font.data[(ch - 32) * font.height + i];
		for (j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				uint8_t data[] = {color >> 8, color & 0xFF};
				ST7789_WriteData(data, sizeof(data));
			}
			else {
				uint8_t data[] = {bgcolor >> 8, bgcolor & 0xFF};
				ST7789_WriteData(data, sizeof(data));
			}
		}
	}
	ST7789_UnSelect();
}

/** 
 * @brief Write a string 
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
	ST7789_Select();
	while (*str) {
		if (x + font.width >= st7789_ctx.width) {
			x = 0;
			y += font.height;
			if (y + font.height >= st7789_ctx.height) {
				break;
			}

			if (*str == ' ') {
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}
		ST7789_WriteChar(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}
	ST7789_UnSelect();
}

/** 
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	ST7789_Select();
	uint8_t i;

	/* Check input parameters */
	if (x >= st7789_ctx.width ||
		y >= st7789_ctx.height) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= st7789_ctx.width) {
		w = st7789_ctx.width - x;
	}
	if ((y + h) >= st7789_ctx.height) {
		h = st7789_ctx.height - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		ST7789_DrawLine(x, y + i, x + w, y + i, color);
	}
	ST7789_UnSelect();
}

/** 
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	ST7789_Select();
	/* Draw lines */
	ST7789_DrawLine(x1, y1, x2, y2, color);
	ST7789_DrawLine(x2, y2, x3, y3, color);
	ST7789_DrawLine(x3, y3, x1, y1, color);
	ST7789_UnSelect();
}

/** 
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	ST7789_Select();
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	}
	else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	}
	else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) {
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	}
	else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		ST7789_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
	ST7789_UnSelect();
}

/** 
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	ST7789_Select();
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);
	ST7789_DrawLine(x0 - r, y0, x0 + r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
		ST7789_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

		ST7789_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
		ST7789_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
	ST7789_UnSelect();
}


/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST7789_TearEffect(uint8_t tear)
{
	ST7789_Select();
	uint8_t reg = tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */;
	ST7789_WriteCommand(&reg ,1);
	ST7789_UnSelect();
}

/**
 *
 */
void ST7789_VerticalScroll(uint16_t pixel)
{
	uint8_t reg = ST7789_VSCRDEF;
	ST7789_WriteCommand(&reg ,1);
	ST7789_WriteSmallData (0x00);
	ST7789_WriteSmallData (0x00);
	ST7789_WriteSmallData (0x01);
	ST7789_WriteSmallData (0x40);
	ST7789_WriteSmallData (0x00);
	ST7789_WriteSmallData (0x00);

	reg = ST7789_VSCRSADD;
	ST7789_WriteCommand(&reg , 1);
	pixel = ( 319 - 10 ) - pixel;
	ST7789_WriteSmallData (pixel >> 8);
	ST7789_WriteSmallData (pixel);
}


/** 
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void ST7789_Test(void)
{
	ST7789_Fill_Color(WHITE);
	vTaskDelay(1000);
	ST7789_WriteString(10, 20, "Speed Test", Font_11x18, RED, WHITE);
	vTaskDelay(1000);
	ST7789_Fill_Color(CYAN);
    vTaskDelay(500);
	ST7789_Fill_Color(RED);
    vTaskDelay(500);
	ST7789_Fill_Color(BLUE);
    vTaskDelay(500);
	ST7789_Fill_Color(GREEN);
    vTaskDelay(500);
	ST7789_Fill_Color(YELLOW);
    vTaskDelay(500);
	ST7789_Fill_Color(BROWN);
    vTaskDelay(500);
	ST7789_Fill_Color(DARKBLUE);
    vTaskDelay(500);
	ST7789_Fill_Color(MAGENTA);
    vTaskDelay(500);
	ST7789_Fill_Color(LIGHTGREEN);
    vTaskDelay(500);
	ST7789_Fill_Color(LGRAY);
    vTaskDelay(500);
	ST7789_Fill_Color(LBBLUE);
    vTaskDelay(500);
	ST7789_Fill_Color(WHITE);
	vTaskDelay(500);

	ST7789_WriteString(10, 10, "Font test.", Font_16x26, GBLUE, WHITE);
	ST7789_WriteString(10, 50, "Hello!", Font_7x10, RED, WHITE);
	ST7789_WriteString(10, 75, "Hello!", Font_11x18, YELLOW, WHITE);
	ST7789_WriteString(10, 100, "Hello!", Font_16x26, MAGENTA, WHITE);
	vTaskDelay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Rect./Line.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawRectangle(30, 30, 100, 100, WHITE);
	vTaskDelay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Rect.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledRectangle(30, 30, 50, 50, WHITE);
	vTaskDelay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Circle.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawCircle(60, 60, 25, WHITE);
	vTaskDelay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Cir.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledCircle(60, 60, 25, WHITE);
	vTaskDelay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Triangle", Font_11x18, YELLOW, BLACK);
	ST7789_DrawTriangle(30, 30, 30, 70, 60, 40, WHITE);
	vTaskDelay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Tri", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledTriangle(30, 30, 30, 70, 60, 40, WHITE);
	vTaskDelay(1000);

}
