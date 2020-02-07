// Standard includes
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "spi.h"
#include "utils.h"

// Common interface includes
#include "uart_if.h"
#include "uart.h"
#include "timer_if.h"
#include "timer.h"
#include "pin_mux_config.h"

// Display includes
#include "Adafruit_GFX.h"
#include "Adafruit_OLED.h"
#include "Adafruit_SSD1351.h"

#define SPI_IF_BIT_RATE             100000
#define CONSOLE                     UARTA0_BASE
#define PAIRDEV                     UARTA1_BASE
#define PAIRDEV_PERIPH              PRCM_UARTA1

//keypad macros
#define DEL    10
#define ENTER  11
#define UNKNWN 12
//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern void (* const g_pfnVectors[])(void);

volatile unsigned long bitBufferPos;

volatile int timerCount;

static unsigned int buffer_Length = 10;
volatile int buffer_Position;
volatile unsigned int recv_buffer_Position;

char *buffer;
char *recvBuffer;

char arr[75];
char zero[80];
char one[80];
char two[80];
char three[80];
char four[80];
char five[80];
char six[80];
char seven[80];
char eight[80];
char nine[80];
char last[80];
char mute[80];

int button; //10 is Last, 11 is Mute, 12 is unknown
int prevButton; //indexed similar to above
int numPresses;

char numPad[10][4] = {
{' ',' ',' ',' '}, //0 is for spaces
{'!','!','!','!'}, //1 is for COLORS, ignore
{'A', 'B', 'C','C'},
{'D', 'E', 'F','F'},
{'G', 'H', 'I','I'},
{'J', 'K', 'L','L'},
{'M', 'N', 'O','O'},
{'P', 'Q', 'R', 'S'},
{'T', 'U', 'V', 'V'},
{'W', 'X', 'Y', 'Z'},
};

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

// an example of how you can use structs to organize your pin settings for easier maintenance
typedef struct PinSetting {
    unsigned long port;
    unsigned int pin;
} PinSetting;


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************
static void BoardInit(void);
static void stringsInit(void);
static void reinitArray(void);
bool fill_Buffer(char letter_INPUT);
void empty_Buffer(void);
void char_Delete(void);
int compareStrings(char* first, char* second);
char findText(void);

//// Handlers

//button press handler
static void GPIOA0IntHandler(void) {    // SW2 handler
    unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus(GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);

    if(bitBufferPos == 0){MAP_TimerEnable(TIMERA0_BASE, TIMER_A);}
    //Report("interval is %d, count is %d\r\n", timerCount, bufferPos);

    if(timerCount > 37){
        arr[bitBufferPos] = '1';
        timerCount = 0; //reset counter
    }else{
        arr[bitBufferPos] = '0';
        timerCount = 0; //once written, reset counter
    }

    bitBufferPos++;
}

//IR decoding
void TimerHandler(){
    Timer_IF_InterruptClear(TIMERA0_BASE);
    timerCount++; //counts up periodically, simulates time interval
}

//void UARTIntHandler(){
//    MAP_UARTIntDisable(UARTA1_BASE, UART_INT_RX);
//    char recvChar;
//    Message("UART INT\n");
//    //clear_Incoming();
//    memset(recvBuffer, 0, sizeof(recvBuffer));
//    recv_buffer_Position = 0;
//    while(UARTCharsAvail(PAIRDEV)){
//        recvChar = UARTCharGet(PAIRDEV);
//        recvBuffer[recv_buffer_Position] = recvChar;
//        ++recv_buffer_Position;
//        Report("UART Received Letter: %c\n", recvChar);
//    }
//    int i;
//    for(i = 0; i < recv_buffer_Position; i++){
//        //output_Display(recvBuffer[i], i);
//    }
//    MAP_UARTIntEnable(UARTA1_BASE, UART_INT_RX);
//}

//consecutive button presses
void TimerIntHandler(){
    Timer_IF_InterruptClear(TIMERA1_BASE);
    numPresses = 0;
    button = 12; //unknown button
}

////Helpers
int compareStrings(char* first, char* second){
    if (strstr(first, second) != NULL){
            return 1;
    }
    return 0;
}

void empty_Buffer(void){
    memset( buffer, 0, sizeof(buffer));
    buffer_Position = 0;
    clear_Outgoing();
}

void char_Delete(void){
    if(buffer_Position == -1){return;}
    buffer[buffer_Position] = '\0';
    erase_InChar(buffer_Position);
    --buffer_Position;
}

//MAIN LOGIC

//find text by button
char findText(void){
    char letter[1] = "x";
    if(button != 1 && button != ENTER && button != DEL){
        letter[0] = numPad[button][numPresses % 4]; //modulo by 4 to cycle through the characters
    }else{
        if(prevButton == ENTER){letter[0] = '\n';}
        if(prevButton == DEL){letter[0] = 'd';}
    }

    return letter[0];
}

//add character to string that will be sent out
bool fill_Buffer(char letter_INPUT){
    if (letter_INPUT == 'd'){  //ignore if full if delete is pressed
        return false;
    }else if(letter_INPUT == '\n'){  //ignore if full if enter is pressed
        return false;
    }else{
        if (buffer_Position < buffer_Length){ //buffer not full
            if(letter_INPUT == 'x'){
                return true; //don't write if weird input
            }else{
                buffer[buffer_Position] = letter_INPUT;
                ++buffer_Position;
            }
            return false;  //false if buffer not full
        }else if(buffer_Position == 9){ //buffer full
            return true;   //true if buffer full
        }
    }

   return true;
}

static void reinitArray(void){
    int i;
    for (i = 0; i < 75; i++){
        arr[i] = '0';
    }
}

//bits to number
static void decode_and_write(void){
    //Report("Array is: %s\r\n", arr);
    if (compareStrings(arr, zero) == 1){
        MAP_UtilsDelay(800000);
        Report("0\r\n");
        button = 0;
    }
    else if (compareStrings(arr, one) == 1){
        MAP_UtilsDelay(800000);
        Report("1\r\n");
        button = 1;
    }
    else if (compareStrings(arr, two) == 1){
        MAP_UtilsDelay(800000);
        Report("2\r\n");
        button = 2;
    }
    else if (compareStrings(arr, three) == 1){
        MAP_UtilsDelay(800000);
        Report("3\r\n");
        button = 3;
    }
    else if (compareStrings(arr, four) == 1){
        MAP_UtilsDelay(800000);
        Report("4\r\n");
        button = 4;
    }
    else if (compareStrings(arr, five) == 1){
        MAP_UtilsDelay(800000);
        Report("5\r\n");
        button = 5;
    }
    else if (compareStrings(arr, six) == 1){
        MAP_UtilsDelay(800000);
        Report("6\r\n");
        button = 6;
    }
    else if (compareStrings(arr, seven) == 1){
        MAP_UtilsDelay(800000);
        Report("7\r\n");
        button = 7;
    }
    else if (compareStrings(arr, eight) == 1){
        MAP_UtilsDelay(800000);
        Report("8\r\n");
        button = 8;
    }
    else if (compareStrings(arr, nine) == 1){
        MAP_UtilsDelay(800000);
        Report("9\r\n");
        button = 9;
    }
    else if (compareStrings(arr, last) == 1){
        MAP_UtilsDelay(800000);
        Report("LAST(DELETE)\r\n");
        button = DEL;
    }
    else if (compareStrings(arr, mute) == 1){
        MAP_UtilsDelay(800000);
        Report("MUTE(ENTER)\r\n");
        button = ENTER;
    }
    else{
        Message("unknown\r\n");
        button = UNKNWN;
    }

    reinitArray();
    char toWrite = "";
    MAP_UtilsDelay(800000);

    switch(button){
    case ENTER:
        //TODO: send message
        empty_Buffer();
        break;
    case DEL:
        char_Delete();
        break;
    case UNKNWN:
        //if invalid input found, want to reset
        Timer_IF_Stop(TIMERA1_BASE, TIMER_A);
        numPresses = 0;
        Report("unknown character pressed");
        break;
    default:
        if(button == prevButton){
            //delete the char, then write the next one in the same button range ONLY if buttonpresses hasnt been reset
            toWrite = findText();
            if(numPresses > 0){char_Delete();}
            toWrite = findText();
            if(fill_Buffer(toWrite) == true){break;}

            input_Display(buffer[buffer_Position-1], buffer_Position); //display next in the button range on OLED
            numPresses++;
            Timer_IF_Start(TIMERA1_BASE, TIMER_A, 1000); //want to start the timer after displaying first char
        }else{
            Timer_IF_Stop(TIMERA1_BASE, TIMER_A); //if the data is different, no need to wait for button presses
            numPresses = 0; //reset button presses
            toWrite = findText();
            if(fill_Buffer(toWrite) == true){break;}
            input_Display(buffer[buffer_Position-1], buffer_Position);
            prevButton = button;
        }
    }

    prevButton = button;
}

//Init Methods
static void
BoardInit(void) {
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
    
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

static void initTimer(void){
    Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, TimerIntHandler);

    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);
    MAP_TimerLoadSet(TIMERA0_BASE, TIMER_A, (SYS_CLK/1000000) * 50);
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_A, TimerHandler);
}

static void stringsInit(void){
    strcpy(zero,  "101100101111010011010110");
    strcpy(one,   "101111110111010000001110");
    strcpy(two,   "110000111101001111011000");
    strcpy(three, "011000101010001111010101");
    strcpy(four,  "110001011100011110100011");
    strcpy(five,  "101101100010100100111101");
    strcpy(six,   "111101001011011000101101");
    strcpy(seven, "111101010011011000101011");
    strcpy(eight, "110001011110011110100001");
    strcpy(nine,  "111010110001010001011110");
    strcpy(last,  "111110010101110001101010");
    strcpy(mute,  "110001011100111110100011");
}

static void initOLED(void){
    Adafruit_Init(); //initialize OLED
    boot_Up(); //clear array
    menu_Start(); //create canvas
}

static void initComms(void){
    //UART1 handler setup
    /*MAP_UARTIntRegister(UARTA1_BASE,UARTIntHandler);
    MAP_UARTIntEnable(UARTA1_BASE,UART_INT_RX);
    MAP_UARTFIFOLevelSet(UARTA1_BASE,UART_FIFO_TX1_8,UART_FIFO_RX1_8);
    MAP_UARTConfigSetExpClk(PAIRDEV,MAP_PRCMPeripheralClockGet(PAIRDEV_PERIPH),
                             UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                              UART_CONFIG_PAR_NONE));*/

    MAP_PRCMPeripheralReset(PRCM_GSPI);

    //configure SPI
    MAP_SPIReset(GSPI_BASE);
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    MAP_SPIEnable(GSPI_BASE); //enable SPI
    MAP_SPICSEnable(GSPI_BASE); //enable chip select
    Report("SPI enabled");
}

//Main
 int main() {
    unsigned long ulStatus;
    buffer_Position = 0;
    recv_buffer_Position = 0;
    buffer = (char*)malloc(buffer_Length * sizeof(char));
    recvBuffer = (char*)malloc(buffer_Length* sizeof(char));

    BoardInit();
    PinMuxConfig();
    InitTerm();
    ClearTerm();

    //init UART and SPI
    initComms();

    MAP_GPIOIntRegister(GPIOA0_BASE, GPIOA0IntHandler); //register interrupt to handler
    MAP_GPIOIntTypeSet(GPIOA0_BASE, 0x40, GPIO_RISING_EDGE); //configure falling edge interrupt

    ulStatus = MAP_GPIOIntStatus(GPIOA0_BASE, false);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus); //clear interrupts

    // clear global variables
    bitBufferPos=0;
    timerCount = 0;

    // Enable GPIO interrupt
    MAP_GPIOIntEnable(GPIOA0_BASE, 0x40);

    //init message
    Message("\t\t****************************************************\n\r");
    Message("\t\t\tPush the remote to generate an interrupt\n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");

    initTimer(); //initialize timer interrupt
    stringsInit(); //create number to bit mapping
    reinitArray(); //empty array buffer that will be filled by IR data
    initOLED();
    while (1) {
        if (bitBufferPos > 49) {
            Timer_IF_Stop(TIMERA0_BASE, TIMER_A);
            MAP_GPIOIntDisable(GPIOA0_BASE, 0x40);
            decode_and_write();
            bitBufferPos = 0;
            MAP_GPIOIntEnable(GPIOA0_BASE, 0x40);
        }
    }
}


