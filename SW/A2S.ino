// =========================================================================================
// === DStage A2S - ABS to Speed adapter ===================================================
// =========================================================================================
// Version 0.3 2024.09.04
// Designed for Arduino Nano V3 with ATMega328PB and DStage A2S v3.0 hardware
// IMPORTANT: this program will NOT WORK with ATMega328P!!!
// 
// email: dstagegarage@gmail.com
// YT: www.youtube.com/DStageGarage
// GitHub: https://github.com/DStageGarage/ABS2SPEED/
// =========================================================================================

// Set to 1 if factor is to be provided with a dipswitch or 0 if it will be hardcoded in code (requires HARDCODED_FACTOR to be set!)
#define USE_DIPSWITCH 1
// Set to desired frequency recalculation factor if dipswitch is not used
#define HARDCODED_FACTOR 5.67

// Set to 1 if speed limit is to be used (requires setting the speedometr limit!)
#define USE_SPEEDO_LIMIT 0
// Set to 1 if staging is to TURNED ON (requires setting the speedometr limit!)
#define USE_STAGING 0
// Set maximum output frequency in Hz at which speedometer reaches its limit if limit and/or staging is to be used
#define MAX_SPEEDO_FREQ 205
// Set pre staging delay in miliseconds if staging is to be used (wait before moving the speedo needle, speedo may require a moment to power up)
#define PRE_STAGING_DELAY 2000
// Set mid staging delay in miliseconds if staging is to be used (how long max frequency is to be send, it may take a moment for the needle to reach max value)
#define MID_STAGING_DELAY 2000
// Set post staging delay in miliseconds if staging is to be used (wait after frequency is set back to 0, it may take a moment for the needle to reach 0)
#define POST_STAGING_DELAY 2000

// Internal definitions - DO NOT MODIFY
// Inputs from the dipswitch
#define SET0 12
#define SET1 11
#define SET2 10
#define SET3 9
#define SET4 8
#define SET5 7
#define SET6 6
#define SET7 5
#define SET8 4
#define SET9 3

// Min half period calculated as counter cycles from max frequency of the speedo, MCU oscilator and prescaller
#define MIN_HALF_PERIOD (((16000000 / 256) / MAX_SPEEDO_FREQ) / 2)
// =========================================================================================

// global variables
unsigned char t3_ovf;
unsigned int period, timestamp;
float factor;


void setup() 
{
  cli();                                              // globaly turn off the interrupts

  #if USE_DIPSWITCH == 0
    factor = HARDCODED_FACTOR/2;                      // hardcoded factor
  #else
    factor = dipswitch_setup_and_read();              // dipswitch defined factor - read at startup
  #endif

  period = 0;                                         // set initial frequency to 0
  speed_out_setup();                                  // and init the output

  // if staging is to be used
  #if USE_STAGING == 1
    delay(PRE_STAGING_DELAY);                         // wait before moving the needle
    
    sei();                                            // globaly turn on the interrupts to allow output signal to work
    period = (float)MIN_HALF_PERIOD / factor;         // set max speed frequency
    delay(MID_STAGING_DELAY);                         // and wait for a while

    period = 0;                                       // set speed to 0
    delay(POST_STAGING_DELAY);                        // and wait for a while
    cli();                                            // globaly turn off the interrupts to setup input in peace
  #endif

  speed_in_setup();                                   // setup the input
  
  sei();                                              // globaly turn on the interrupts
}


void loop() 
{
  // stop speed signal when nothing on the input
  if(t3_ovf > 1)
    period = 0;
}


// =========================================================================================
// === speed in ============================================================================
// =========================================================================================
void speed_in_setup()
{
  TCCR3A = (0 << 6) | (0 << 4) | (0 << 0);            // Normal mode, top 0xFFFF, no output
  TCCR3B = (1 << 7) | (1 << 6) | (0 << 3) | (4 << 0); // Normal mode, top 0xFFFF, noice canceler, rising edge, prescaler 256 
  TCCR3C = 0;
  TIMSK3 = (1 << 5) | (1 << 0);                       // input capture interrupt, overflow interrupt

  PORTE &= ~(1 << 2);                                 // no pull-up on PE2
  DDRE &= ~(1 << 2);                                  // PE2 as input ICP3

  t3_ovf = 0;                                         // clear overflow count
  period = 0;
}

// capture on rising edge of input signal
ISR(TIMER3_CAPT_vect)
{
  if(t3_ovf > 1)
  {
    timestamp = ICR3;                                 // save new timestamp
    // clear overflow and wait till next event
    t3_ovf = 0;
    return;
  }

  if(t3_ovf == 1)
  {
    // if there was one overflow calculate time from last capture to verflow and add current capture
    period = (0xFFFF - timestamp) + ICR3;
    t3_ovf = 0;
  }
  else
  {
    period = ICR3 - timestamp;  
  }
  timestamp = ICR3;                                   // save new timestamp
}

// timer overflow is normal in this mode but happening twice means no movement
ISR(TIMER3_OVF_vect)
{
  // increment overflow count
  t3_ovf++;
  if(t3_ovf > 100)
    t3_ovf = 100;
}
// =========================================================================================


// =========================================================================================
// === speed out ===========================================================================
// =========================================================================================
void speed_out_setup()
{
  OCR4A = 0x0000;
  TCCR4A = (0 << 6) | (0 << 4) | (0 << 0);            // CTC mode, top OCR4A, no output
  TCCR4B = (0 << 7) | (0 << 6) | (1 << 3) | (4 << 0); // CTC mode, top OCR4A, prescaler 256 
  TCCR4C = 0;
  TIMSK4 = (0 << 5) | (1 << 1);                       // output compare A interrupt

  PORTD &= ~(1 << 2);                                 // L state on PD2
  DDRD |= 1 << 2;                                     // PD2 as outout for speed

  PORTB &= ~(1 << 5);                                 // L state on PB5
  DDRB |= 1 << 5;                                     // PB5 as Arduino Nano build in LED outout
}


// timer compare match = top value reached in CTC mode interrupt
ISR(TIMER4_COMPA_vect)
{
  unsigned int temp_half_period;

  if(period == 0)
  {
    PORTD &= ~(1 << 2);                               // PD2 state L, speed 0km/h
    OCR4A = 0x8FFF;                                   // next check in half of maximum period
    return;                                           // exit interrupt
  }

  PORTD ^= (1 << 2);                                  // toggle PD2 output to speedometer
  PORTB ^= (1 << 5);                                  // toggle PB5 - LED

  // calculate next half period
  temp_half_period = (float)period * factor;
  
  // if speedo limit is to be used
  #if USE_SPEEDO_LIMIT == 1
    if(temp_half_period < MIN_HALF_PERIOD)            // if speed higher than speedo limit
      temp_half_period = MIN_HALF_PERIOD;             // limit the output frequency
  #endif

  OCR4A = temp_half_period;
}
// =========================================================================================


// =========================================================================================
// === dipswitch factor ====================================================================
// =========================================================================================
#if USE_DIPSWITCH == 1
  float dipswitch_setup_and_read()
  {
    // set pull-ups on dipswitch pins
    pinMode(SET0, INPUT_PULLUP);
    pinMode(SET1, INPUT_PULLUP);
    pinMode(SET2, INPUT_PULLUP);
    pinMode(SET3, INPUT_PULLUP);
    pinMode(SET4, INPUT_PULLUP);
    pinMode(SET5, INPUT_PULLUP);
    pinMode(SET6, INPUT_PULLUP);
    pinMode(SET7, INPUT_PULLUP);
    pinMode(SET8, INPUT_PULLUP);
    pinMode(SET9, INPUT_PULLUP);

    unsigned int temp = 0;
    temp = digitalRead(SET0) << 0;
    temp |= digitalRead(SET1) << 1;
    temp |= digitalRead(SET2) << 2;
    temp |= digitalRead(SET3) << 3;
    temp |= digitalRead(SET4) << 4;
    temp |= digitalRead(SET5) << 5;
    temp |= digitalRead(SET6) << 6;
    temp |= digitalRead(SET7) << 7;
    temp |= digitalRead(SET8) << 8;
    temp |= digitalRead(SET9) << 9;
    temp ^= 0x3FF;                                    // negate the 10bit value as dipswitch is active low

    return (float)temp * 0.005;                       // factor equals dipswitch decimal value * 0.01 but internaly factor has to be divide by 2 thus 0.005
  }
#endif
// =========================================================================================
