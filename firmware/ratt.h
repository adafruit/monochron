#define halt(x)  while (1)

#define DEBUGGING 0
#define DEBUG(x)  if (DEBUGGING) { x; }
#define DEBUGP(x) DEBUG(putstring_nl(x))

// This is a tradeoff between sluggish and too fast to see
#define MAX_BALL_SPEED 5 // note this is in vector arith.
#define ball_radius 2 // in pixels

// If the angle is too shallow or too narrow, the game is boring
#define MIN_BALL_ANGLE 20

// how fast to proceed the animation, note that the redrawing
// takes some time too so you dont want this too small or itll
// 'hiccup' and appear jittery
#define ANIMTICK_MS 75

// Beeep!
#define ALARM_FREQ 4000
#define ALARM_MSONOFF 300
 
#define MAXSNOOZE 600 // 10 minutes

// how many seconds we will wait before turning off menus
#define INACTIVITYTIMEOUT 10 

/*************************** DISPLAY PARAMETERS */

// how many pixels to indent the menu items
#define MENU_INDENT 8

// Where the HOUR10 HOUR1 MINUTE10 and MINUTE1 digits are
// in pixels
#define DISPLAY_H10_X 30
#define DISPLAY_H1_X 45
#define DISPLAY_M10_X 70
#define DISPLAY_M1_X 85

// buffer space from the top
#define DISPLAY_TIME_Y 4

// how big are the pixels (for math purposes)
#define DISPLAY_DIGITW 10
#define DISPLAY_DIGITH 16

#define RIGHTPADDLE_X (SCREEN_W - PADDLE_W - 10)
#define LEFTPADDLE_X 10

// Paddle size (in pixels) and max speed for AI
#define PADDLE_H 12
#define PADDLE_W 3
#define MAX_PADDLE_SPEED 5

// How big our screen is in pixels
#define SCREEN_W 128
#define SCREEN_H 64

// How thick the top and bottom lines are in pixels
#define BOTBAR_H 2
#define TOPBAR_H 2

// Specs of the middle line
#define MIDLINE_W 1
#define MIDLINE_H (SCREEN_H / 16) // how many 'stipples'


/* not used
#define ALARMBOX_X 20
#define ALARMBOX_Y 24
#define ALARMBOX_W 80
#define ALARMBOX_H 20
*/

/*************************** PIN DEFINITIONS */
// there's more in KS0108.h for the display

#define ALARM_DDR DDRB
#define ALARM_PIN PINB
#define ALARM_PORT PORTB
#define ALARM 6

#define PIEZO_PORT PORTC
#define PIEZO_PIN  PINC
#define PIEZO_DDR DDRC
#define PIEZO 3


/*************************** ENUMS */

// Whether we are displaying time (99% of the time)
// alarm (for a few sec when alarm switch is flipped)
// date/year (for a few sec when + is pressed)
#define SCORE_MODE_TIME 0
#define SCORE_MODE_DATE 1
#define SCORE_MODE_YEAR 2
#define SCORE_MODE_ALARM 3

// Constants for how to display time & date
#define REGION_US 0
#define REGION_EU 1
#define TIME_12H 0
#define TIME_24H 1

// displaymode
#define NONE 99
#define SHOW_TIME 0
#define SHOW_DATE 1
#define SHOW_ALARM 2
#define SET_TIME 3
#define SET_ALARM 4
#define SET_DATE 5
#define SET_BRIGHTNESS 6
#define SET_VOLUME 7
#define SET_REGION 8
#define SHOW_SNOOZE 9
#define SET_SNOOZE 10

#define SET_MONTH 11
#define SET_DAY 12
#define SET_YEAR 13

#define SET_HOUR 101
#define SET_MIN 102
#define SET_SEC 103

#define SET_REG 104

#define SET_BRT 105

//DO NOT set EE_INITIALIZED to 0xFF / 255,  as that is
//the state the eeprom will be in, when totally erased.
#define EE_INITIALIZED 0xC3
#define EE_INIT 0
#define EE_ALARM_HOUR 1
#define EE_ALARM_MIN 2
#define EE_BRIGHT 3
#define EE_VOLUME 4
#define EE_REGION 5
#define EE_TIME_FORMAT 6
#define EE_SNOOZE 7

/*************************** FUNCTION PROTOTYPES */

uint8_t leapyear(uint16_t y);
void clock_init(void);
void initbuttons(void);
void tick(void);
void setsnooze(void);
void initanim(void);
void initdisplay(uint8_t inverted);
void step(void);
void setscore(void);
void draw(uint8_t inverted);

void set_alarm(void);
void set_time(void);
void set_region(void);
void set_date(void);
void print_timehour(uint8_t h, uint8_t inverted);
void print_alarmhour(uint8_t h, uint8_t inverted);
void display_menu(void);
void drawArrow(uint8_t x, uint8_t y, uint8_t l);
void setalarmstate(void);
void beep(uint16_t freq, uint8_t duration);
void printnumber(uint8_t n, uint8_t inverted);
uint8_t intersectrect(uint8_t x1, uint8_t y1, uint8_t w1, uint8_t h1,
					  uint8_t x2, uint8_t y2, uint8_t w2, uint8_t h2);

uint8_t calculate_keepout(float theball_x, float theball_y, float theball_dx, float theball_dy, uint8_t *keepout1, uint8_t *keepout2);

void drawbigdigit(uint8_t x, uint8_t y, uint8_t n, uint8_t inverted);
void drawmidline(uint8_t inverted);

float random_angle_rads(void);

uint8_t i2bcd(uint8_t x);

uint8_t readi2ctime(void);

void writei2ctime(uint8_t sec, uint8_t min, uint8_t hr, uint8_t day,
		  uint8_t date, uint8_t mon, uint8_t yr);
