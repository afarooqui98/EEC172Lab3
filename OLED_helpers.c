
/* These functions are based on the Arduino test program at
*  https://github.com/adafruit/Adafruit-SSD1351-library/blob/master/examples/test/test.ino
*
*  You can use these high-level routines to implement your
*  test program.
*/

// TODO Configure SPI port and use these libraries to implement
// an OLED test program. See SPI example program.

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"

//I2C
#include "i2c_if.h"


extern int cursor_x;
extern int cursor_y;

float p = 3.1415926;

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define	RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
#define PINK            0xF810
#define DARKGRAY        0x8410
#define GRAY            0xC618
//#define
//#define
//#define
//#define
//#define
//#define
//#define


//*****************************************************************************
//  function delays 3*ulCount cycles
void delay(unsigned long ulCount){
	int i;

  do{
    ulCount--;
		for (i=0; i< 65535; i++) ;
	}while(ulCount);
}

void boot_Up(void)
{
    delay(10);
    fillScreen(BLACK);
    delay(10);
        char boot[] = {'B','e','g','i','n',' ','I','R','.','E','X','E'};
        unsigned int i;
        unsigned int font_pos = 0;
        for (i = 0; i < 128; i+=7)
        {
            if (font_pos == 12)
            {
                break;
            }
            else{
            drawChar( i, 0, boot[font_pos], GRAY, BLACK, 1);
            ++font_pos;
            }
        }

}

void menu_Start(void)
{
    delay(200);

    fillScreen(BLACK);
    char out[] = {'O','u','t','g','o','i','n','g',':',' ',' ',' '};
    unsigned int i;
    unsigned int font_pos = 0;
    for (i = 0; i < 128; i+=7)
    {
        if (font_pos == 12)
        {
            break;
        }
        else{
        drawChar( i, 0, out[font_pos], GREEN, BLACK, 1);
        ++font_pos;
        }
    }
    char in[] = {'I','n','c','o','m','i','n','g',':',' ',' ',' '};
    unsigned int j;
    font_pos = 0;
    for (j = 0; j < 128; j+=7)
    {
        if (font_pos == 12)
        {
            break;
        }
        else{
        drawChar( j, 68, in[font_pos], RED, BLACK, 1);
        ++font_pos;
        }
    }

    fillRect( 5, 10, 121, 50, DARKGRAY);
    fillRect( 7, 12, 117, 46, BLACK);
    fillRect( 5, 78, 121, 50, DARKGRAY);
    fillRect( 7, 80, 117, 46, BLACK);
}

void input_Display( char INPUT, int POSITION)
{
    int x = 10 * POSITION;
    int y = 30;
    drawChar( x, y, INPUT, WHITE, BLACK, 1 );
}

void output_Display( char OUTPUT, int POSITION)
{
    int x = 10 * POSITION;
    int y = 98;
    drawChar( x, y, OUTPUT, WHITE, BLACK, 1 );
}

void clear_Outgoing ( void )
{
    fillRect( 7, 12, 117, 46, BLACK);
}

void clear_Incoming ( void )
{
    fillRect( 7, 80, 117, 46, BLACK);
}

void erase_InChar ( int POSITION )
{
    int x = 10 * POSITION;
    int y = 30;
//    drawChar( x, y, ' ', BLACK, BLACK, 1 );
    fillRect( x, y, 5, 7, BLACK);
}
//void
