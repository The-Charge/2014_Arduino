/*
    seven_segment display from Boom Done
    Modified for 4 digets and taylored to Team 2619's liking
    by:  Mike Rehberg   Feb, 2014
*/


void(* resetFunc) (void) = 0; //declare reset function at address 0, from http://www.instructables.com/id/two-ways-to-reset-arduino-in-software/step2/using-just-software/

#include <SoftwareSerial.h>

const int latchpin = 2;
const int clockpin = 3;
const int datapin = 4;
const int startpin = 5;
const int modepin = 7;

#define MODETIMER LOW
#define MODESONAR HIGH

#define WAIT LOW

//   c.bafged
// 0-10111011-187
// 1-10100000-160
// 2-00110111-55
// 3-10110101-181
// 4-10101100-172
// 5-10011101-157
// 6-10011111-159
// 7-10110000-176
// 8-10111111-191
// 9-10111101-189
// A-10111110-190

// r-00000011
// d-10100111
// y-10101101

const int segdisp[11] = { 
  187, 160, 55, 181, 172, 157, 159, 176, 191, 189, 190 };
//const int ready[3] = {0b00000110, 0b10100111, 0b10101101}; // rdy
const int ready[4] = {
  55, 159, 160, 189}; // "2619"
const int inf[4] = {
  0b01000000, 0b10000000, 0b10000110, 0b00011110}; // .inf

// amount of time on the clock
int minutes = 2;
int seconds = 30;

// time to switch out of autonomous
#define AUTOMIN 2
#define AUTOSEC 20

/*===========*/
SoftwareSerial myInvertedConn(10,11, true); // RX, TX
uint8_t inByte = 0; // input byte from software serial
int byteNum = 0; // which ascii byte are we processing (0 -> 2)
int sonarSample[4] = {
  0,0,0}; // byte array of ascii values
int sampleNum=0; // which sample number we received (0->8) stored in an array
int sonarData[10] = {
  0,0,0,0,0,0,0,0,0}; // samples stored here to find median
int sonarMedian = 0;  // median from sonarData
/*===========*/

int dphack = 0;
#define SEG_DP 0x40

void setup()
{
  int i;
  pinMode(latchpin, OUTPUT);
  pinMode(clockpin, OUTPUT);
  pinMode(datapin, OUTPUT);

  // debug serial
  Serial.begin(115200);

  // set the data rate for the SoftwareSerial port, 9600 baud for sonar module
  myInvertedConn.begin(9600);

  clearDisplay();

  for (i = 0; i < 9; i++)
  {
    digitalWrite(latchpin, LOW);
    shiftOut(datapin, clockpin, MSBFIRST, ready[i%4]);
    digitalWrite(latchpin, HIGH);
  }

  while (digitalRead(startpin) == WAIT); // hang out until we are told to start
}

void loop()
{
  static int display_on = 1;

  // one-second timer
  static unsigned long prev_millis = 0;
  unsigned long curr_millis = millis();

  // SONAR CODE

  //  If we have data on the software serial port (Sonar)
  if (myInvertedConn.available())
  {
    inByte = myInvertedConn.read(); // read the byte from the port

    //if (inByte == 'R' || inByte == '\r') // these are space and carriage return
    if (inByte < 48 || inByte > 57) // if it's not an ASCII number
    {
      digitalWrite(13, !digitalRead(13)); // twiddle the LED
      byteNum = 0; // reset the byte count to 0 (for \r or 'R')
    }
    else
    {      
      sonarSample[byteNum] = inByte; // create the sample arary

      // if we have the 3rd (last) byte package it 0->2
      if (byteNum++ >= 2)
      {
        sonarData[sampleNum] = (sonarSample[0]-48)*100 + (sonarSample[1]-48)*10 + (sonarSample[2]-48);

        Serial.println(sonarData[sampleNum]); // print the data to serial for debug
        byteNum = 0;
        sampleNum++;
      }
    }

    // add samples until we have 9
    if (sampleNum >= 9)
    {
      bubbleSort(sonarData, 9); // sort the list of values

      sonarMedian = sonarData[5]; // this should be the median
      sampleNum = 0;  // set the number of samples back to zero to fill array

      // print debug to median to serial port
      Serial.print("median: ");
      Serial.println(sonarMedian);

      if (display_on && digitalRead(modepin)==MODESONAR)
      {
        displayNumber(sonarMedian);
        dphack = MODESONAR;
      }
    }
  }

  // ONCE PER SECOND CODE
  if(curr_millis - prev_millis > 1000) {     // code in here happens every second
    prev_millis = curr_millis;

    // update the timer (huge kludge for autonomous)
    if(minutes > 0 || seconds > 0) {
      if (seconds == AUTOSEC && minutes == AUTOMIN)
      {
        if (digitalRead(startpin) != WAIT)
          decrementCounter();
      }
      else
        decrementCounter();
    }
    else {
      display_on++; // make the display pulse when time is up
    }

    if (display_on & 1) 
    {
      if (digitalRead(modepin)==MODESONAR)
      {
        dphack = MODESONAR;
        displayNumber(sonarMedian);
      }
      else
      {
        dphack = MODETIMER;
        displayTime();
      }
    }
    else
    {
      clearDisplay();
    }


    if (display_on == 10) // using display_on as a timer as well
      resetFunc();
  }
}

void decrementCounter() {
  seconds--;
  if(seconds < 0) {
    seconds = 59;
    minutes--;
  }
}

void clearDisplay() {
  digitalWrite(latchpin, LOW);
  for(int i = 0; i < 4; i++) {
    shiftOut(datapin, clockpin, MSBFIRST, 0);
  }
  digitalWrite(latchpin, HIGH);
}

void displayNumber(int number) // Assuming four
{
  shiftOut(datapin, clockpin, MSBFIRST, 0);  // blank out the first display
  displayNumberForOne(number);
  displayNumberForOne(number);
  displayNumberForOne(number);
}

void displayNumberForOne(int number) {
  int hun = number/100;
  int i;

  digitalWrite(latchpin, LOW);
  if (number < 760) 
  {
    displayDigitForOne(segdisp[hun]);
    number -= (hun*100);
    displayDigitForOne(segdisp[number/10]);
    displayDigitForOne(segdisp[number%10]);
  }
  else // this is a dirty, dirty hack again
  {
    for (i = 0; i < 9; i++)
    {
      digitalWrite(latchpin, LOW);
      shiftOut(datapin, clockpin, MSBFIRST, inf [i%4]);
      digitalWrite(latchpin, HIGH);
    }
  }
  digitalWrite(latchpin, HIGH);
}

void displayTime() // assuming four displays
{
  shiftOut(datapin, clockpin, MSBFIRST, 0);  // blank out the first display
  displayTimeForOne();
  displayTimeForOne();
  displayTimeForOne();
}

void displayTimeForOne(){
  digitalWrite(latchpin, LOW);
  if (minutes == AUTOMIN && seconds >= AUTOSEC)  // this is a dirty, dirty hack
  {
    displayDigitForOne(segdisp[0xA]);
    displayDigitForOne(segdisp[(seconds - AUTOSEC)/10]);
  }
  else
  {
    displayDigitForOne(segdisp[minutes]);
    displayDigitForOne(segdisp[seconds/10]);
  }
  displayDigitForOne(segdisp[seconds%10]);
  digitalWrite(latchpin, HIGH);
}

void displayDigitForOne(int digit) {
  if (dphack == MODESONAR)
  {
    digit |= SEG_DP;
  }
  shiftOut(datapin, clockpin, MSBFIRST, digit);
}


// Bubble sorting function -- the most efficient sort on an 8-bit micro
void bubbleSort(int *a, int n)
{
  for (int i = 1; i < n; ++i)
  {
    int j = a[i];
    int k;
    for ( k = i - 1; (k >= 0) && (j < a[k]); k-- )
    {
      a[k + 1] = a[k];
    }

    a[k + 1] = j;
  }
}


