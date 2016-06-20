#include <EEPROM.h>
#include <LedControl.h>
#include <Encoder.h>

/* Instructions:

When the rhythms are playing, the display flips between page 1 (steps 1-8) and page 2 (steps 9-16).

Here's what the display shows...

Row 1: What step is Output 1 playing
Row 2: Output 1 Pattern (steps 1-8 or 9-16)

Row 3: What step is Output 2 playing
Row 4: Output 2 Pattern (steps 1-8 or 9-16)

Row 5: What step is Output 3 playing
Row 6: Output 3 Pattern (steps 1-8 or 9-16)

Row 7: Which channel is selected

- 2 dots on the left for Channel 1
- 2 dots in the middle for Channel 2
- 2 dots on the right for Channel 3


Row 8: Current triggers

1. Input trigger
2. -
3. Output 1 trigger
4. Output 1 off-beat trigger (when Output 1 isn't playing
5. Output 2 trigger
6. -
7. Output 3 trigger
8. -


- When you have Channel 1 selected and you rotate the N- and K- and Offset knobs, 
Rows 1 and 2 will respectively show the pattern length (N), pattern density (K) or Offset (O).

The same goes for Channel 2 (Rows 3 and 4) and Channel 3 (Rows 5 and 6)

- Rotating the Offset encoder clockwise rotates the steps up to one full rotation

Example:

X 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 - Original 16-step pattern (N = 16, K = 1)
0 X 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 - Offset of 1
0 0 X 0 0 0 0 0 0 0 0 0 0 0 0 0 0 - Offset of 2
0 0 0 X 0 0 0 0 0 0 0 0 0 0 0 0 0 - Offset of 3
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 X - Offset of 15
X 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 - Offset of 16


>>> Sneak-Thief's design notes <<<

- Added Encoder library to stabilize encoder use
- Cleaned up code a bit
- Created schematic with input/output protection: http://sneak-thief.com/modular/tombolas-euclidean-0.3.png
- Added reset gate input
- Added Offset control 

 To do 
- Error checking for eeprom reads / values to reduce risk of crashes 
- Add CV control of N, K and Offset using analog pins 5,6,7 



>>> Tombola's original design notes <<<

 To do 
- Error checking for eeprom reads / values to reduce risk of crashes 
- Find cause of ocassional skipped beats? 
 
 Done 
 - Connect 'off beat' for channel 1 to the spare output  
 - something causing channel 1 to stick - N changes don't appear 
 - When N is turned down, reduce K accordingly 
 - make tick pulse correctly 
 - Fix drawing of beats  as beats are playing 
 - binary display of K & N not right 
 - remove serial print / debug routine 
 - remove delay - replace with 'all pulses off after 5ms routine
 - Fix crashing issue with low n  
 - channels 1 and 3 require tweak to start running 
 - Display OK 
 - Pulse in working up to audio rates 
 - Got flashing working - system is outputting euclidean beats! 
 - Integrate encoders & eeprom 
 - Add encoder display - light up row as it's turning DONE
 - 3-way switch working 
 - Active channel indicator 
 - Sleep / wake system & animation 
 - DONE Remove serial printing - slows down?
 
 
 TO DO:
 - Implement offset (added by Sneak-Thief)
 - Add reset outputs and input and button
 - Add CV control of N and K (and later Offset) using analog pins 5,6,7 
 
 BUGS: 
 - FIXED Still display issues - 1111 on the end of beat displays - seems not to be clearing properly 
 - FIXED Display on beats where k=n - seems to show loads of binary 111 on the end
 - FIXED Fix the bottom row - currently output flashers reversed 
 
 
Hardware notes:
  
 Display: 
 din = d2
 clk = d3
 load = d4 
 
 Encoders: 
 Encoder 1a - k / beats = d5
 Encoder 1b - k / beats = d6 
 Encoder 2a - n / length = d7 
 Encoder 2b - n / length  = d8 
 Encoder 3a - Offset = d9 
 Encoder 3 b - offset = d10 

  
 Pulse outputs:
 1 = d11
 2 = d12
 3 = d13
 
 Switches / misc :
 reset switch = A1 
 pulse input = A0
 spare jack out = A3 
 encoder switch = A2  - 3 buttons share one analog input:
 
 10k resistor ladder around 3 push button switches 
 +5v -- 10k -- switch -- 10k -- switch -- 10k -- switch -- 10k -- GND 
 Other ends of switches going to Analog in 0 and also 10k to GND 
 
 */


// Debug Flag
#define debug 2 // 0= normal  1= Internal Clock  2= Internal Clock and SerialDump


// Encoder Pins for the Encoder.h library
Encoder EncK (10,9);
Encoder EncN (8,7);
Encoder EncO (6,5);


// Encoder Pins for internal use (must correspond to the previous pins)
#define enc1a 10
#define enc1b 9
#define enc2a 7
#define enc2b 8
#define enc3a 5
#define enc3b 6

#define sparepin 17 // Offbeat pin

LedControl lc=LedControl(2,3,4,1); // Matrix LED pins

#define brightness 0

#define display_update 2000 // how long active channel display is shown 

int length=50; //pulse length


unsigned long time;
unsigned long last_sync;

int channels = 3;
unsigned int beat_holder[3];

/*
Eeprom schema: 
 Channel 1: n = 1 k = 2 position = 7
 Channel 2: n = 3 k = 4 position = 8
 Channel 3: n = 5 k = 6 position = 9 
 */

unsigned int channelbeats[3][5]={
  {
    EEPROM.read(1),EEPROM.read(2),0,EEPROM.read(7), EEPROM.read(10)            }
  ,{
    EEPROM.read(3),EEPROM.read(4),0, EEPROM.read(8), EEPROM.read(11)           }
  ,{
    EEPROM.read(5),EEPROM.read(6),0,EEPROM.read(9), EEPROM.read(12)            }
}; // 0=n, 1=k, 2 = position , 3 = offset





int a; 
int changes=0;
boolean sleep=true;  // LED Sleep mode enable/disable
int masterclock=0; // Internal clock enable/disable
int read_head;
unsigned int  looptracker;

int old_total;//for knobs
int old_pulses;//for knobs

int pulseinput=0;
int newpulse;//for trigger in
int oldpulse=1;//for trigger in


boolean pulses_active = false; // is active while a beat pulse is playing 
boolean lights_active = false;   

int kknob;
int active_channel =1; // which channel is active? zero indexed 
int nknob;
int oknob; 
int maxn = 16; // maximums and minimums for n and k
int minn = 1;
int mink = 1; 
int maxo = 15; // maximums and minimums for o 
int mino = 0; 
int nn; 
int kk; 
int oo; 

unsigned long last_read; 
unsigned long last_changed; 
#define read_delay  50 // for debouncing 
int channel_switch;
int reset_button; 
int channel_switch_read;



void setup() {

  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,true);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,brightness);
  /* and clear the display */
  lc.clearDisplay(0);

 if ((EEPROM.read(1) > 16) || (EEPROM.read(2) > 16) || (EEPROM.read(3) > 16) || (EEPROM.read(4) > 16) ||(EEPROM.read(5) > 16) || (EEPROM.read(6) > 16)) { // if eprom is blank / corrupted, write some startup amounts

  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 1024; i++)
    EEPROM.write(i, 50);

    EEPROM.write(1,16);
    EEPROM.write(2,4);
    EEPROM.write(3,12);
    EEPROM.write(4,6);
    EEPROM.write(5,8);
    EEPROM.write(6,5);
    EEPROM.write(7,16);
    EEPROM.write(8,2);
    EEPROM.write(9,2);    
    EEPROM.write(10,16);
    EEPROM.write(11,2);
    EEPROM.write(12,2);  
 }

  digitalWrite(enc1a, HIGH);       // turn on pullup resistor
  digitalWrite(enc1b, HIGH);       // turn on pullup resistor
  digitalWrite(enc2a, HIGH);       // turn on pullup resistor
  digitalWrite(enc2b, HIGH);       // turn on pullup resistor
  digitalWrite(enc3a, HIGH);       // turn on pullup resistor
  digitalWrite(enc3b, HIGH);       // turn on pullup resistor
  if (debug == 2){
    Serial.begin(9600);
  }
  for (a=11;a<14;a++){
    pinMode (a,OUTPUT);
  }

  // DEFINE SPARE PIN AS OUTPUT PIN 
  pinMode (sparepin, OUTPUT);

  // initialise beat holders 

  for (int a=0;a<channels;a++){
    beat_holder[a] = euclid(channelbeats[a][0],channelbeats[a][1],channelbeats[a][3]);
  }

}




void loop() {
  /*
What's in the loop: 
   Update time variable 
   Check to see if it is time go go to sleep 
   Changes routine - update beat_holder when channelbeats changes - triggered by changes == true
   Trigger routines - on trigget update displays and pulse
   Read encoders 
   Read switches 
   
   */



  time=millis();

  // COPY OVER N, K & ) VARIABLES FOR EASE OF CODE READING 
  nn = channelbeats[active_channel][0];  
  kk = channelbeats[active_channel][1]; 
  oo = channelbeats[active_channel][3]; 

  // DEBUG PULSE TRIGGER & print out  
  if (debug >0 && time-last_sync > 250){
    Sync();
    if (debug ==2){
      Serial.print ("length =");
      Serial.print (nn);
      Serial.print (" density =");
      Serial.print (kk);
      Serial.print (" offset =");
      Serial.print (oo);
            Serial.print (" channel pot=");
      Serial.println (channel_switch_read);

      
    }   
  };

  // SLEEP ROUTINE 
  if (sleep == false && time-last_sync>1000000)
  {      
    sleepanim();
    lc.shutdown(0,true); 
    sleep = true; 
  }

  // UPDATE BEAT HOLDER WHEN KNOBS ARE MOVED

  if (changes > 0){  
    beat_holder[active_channel] = euclid(nn,kk,oo);
    lc.setRow(0,active_channel*2+1,0);//clear active row
    lc.setRow(0,active_channel*2,0);//clear line above active row


    if (changes == 1){  // 1 = K changes - display beats in the active channel 
      for (a=0;a<8;a++){ 
        if (bitRead (beat_holder[active_channel],nn-1-a)==1 && a<nn){
          lc.setLed(0,active_channel*2,7-a,true);
        }  
        if (bitRead (beat_holder[active_channel],nn-1-a-8)==1 && a+8<nn){
          lc.setLed(0,active_channel*2+1,7-a,true);
        }  
      }
    }

    if (changes == 2){ // 2 = N changes, display total length of beat 
      for (a=0;a<8;a++){ 
        if (a<nn){
          lc.setLed(0,active_channel*2,7-a,true);
        }  
        if (a+8<nn){
          lc.setLed(0,active_channel*2+1,7-a,true);
        }  
      }
    }


    if (changes == 3){  // 3 = Offset changes - display beats in the active channel 
      for (a=0;a<8;a++){ 
        if (bitRead (beat_holder[active_channel],nn-1-a)==1 && a<nn){
          lc.setLed(0,active_channel*2,7-a,true);
        }  
        if (bitRead (beat_holder[active_channel],nn-1-a-8)==1 && a+8<nn){
          lc.setLed(0,active_channel*2+1,7-a,true);
        }  
      }
    }

    changes = 0;  
    last_changed = time; 
  }

  // ANALOG PULSE TRIGGER 
  newpulse=map(analogRead(pulseinput),0,1024,0,3); // Pulse input 
  if (newpulse>oldpulse){
    #define debug 0                    // turn off internal clock if external clock received
    Sync();
  }
  oldpulse = newpulse;



  // READ K KNOB 

  kknob = EncodeReadK(); 
  if (kknob != 0 && time-last_read>read_delay) { 
    if (channelbeats[active_channel][1]+kknob > channelbeats[active_channel][0]) {
      kknob=0;
    }; // check within limits
    if (channelbeats[active_channel][1]+kknob < mink) {
      kknob=0;
    };


  // CHECK AGAIN FOR LOGIC
  if (channelbeats[active_channel][1] > channelbeats[active_channel][0]-1){
    channelbeats[active_channel][1] = channelbeats[active_channel][0]-1;};


    channelbeats[active_channel][1] = channelbeats[active_channel][1]+kknob; // update with encoder reading
    EEPROM.write((active_channel*2)+2,channelbeats[active_channel][1]); // write settings to 2/4/6 eproms 
    last_read = millis();
    changes = 1; // K change = 1
  }

  // READ N KNOB 


  
  nknob = EncodeReadN(); 
if    (channelbeats[active_channel][0] > 16)
      {channelbeats[active_channel][0] = 16;
      };

      
  if (nknob != 0 && time-last_read>read_delay) { 

    // Sense check n encoder reading to prevent crashes 

    if (nn+nknob > maxn) {
      nknob=0;   
    }; // check below maxn
    if (nn+nknob < minn) {
      nknob=0;   
    }; // check above minn

    if (kk > nn+nknob-1 && kk>1){// check if new n is lower than k + reduce K if it is 
       channelbeats[active_channel][1] = channelbeats[active_channel][1]+nknob;
    }; 

    if (oo > nn+nknob-1 && oo>1){// check if new n is lower than o + reduce o if it is 
      channelbeats[active_channel][3] = channelbeats[active_channel][3]+nknob;
    }; 

    channelbeats[active_channel][0] = nn+nknob; // update with encoder reading
    kk = channelbeats[active_channel][1];
    nn = channelbeats[active_channel][0];  // update nn for ease of coding 


    EEPROM.write((active_channel*2)+1,channelbeats[active_channel][0]); // write settings to 2/4/6 eproms 
    last_read = millis();
    changes = 2; // n change = 2 
  }



  // READ O KNOB 

  oknob = EncodeReadO(); 
  if (oknob != 0 && time-last_read>read_delay) { 

    // Sense check o encoder reading to prevent crashes 

    if (oo+oknob > channelbeats[active_channel][0]) {
      oknob=0;   
    }; // check below maxo
    if (oo+oknob < mino) {
      oknob=0;   
    }; // check above minn

      channelbeats[active_channel][3] = oo+oknob;
      oo = channelbeats[active_channel][3];  // update oo for ease of coding   


    EEPROM.write((active_channel*2)+1,channelbeats[active_channel][3]); // write settings to 2/4/6 eproms 
    last_read = millis();
    changes = 3; // O change = 3
  }



  // SELECT ACTIVE CHANNEL 
  channel_switch_read = analogRead(A2); 
  if (channel_switch_read<100){
    channel_switch = 3;
  };
  if (channel_switch_read>100 && channel_switch_read<200){
    channel_switch = 2;
  };
  if (channel_switch_read>200 && channel_switch_read<400){
    channel_switch = 1;
  };
  if (channel_switch_read>400){
    channel_switch = 0;
  };
  if (channel_switch !=3){
    active_channel = channel_switch;


    lc.setRow(0,6,false); //clear row 7 
    lc.setLed(0,6,5-(active_channel*2),true);
    lc.setLed(0,6,5-(active_channel*2)-1,true);
  }; 

  // ENABLE RESET BUTTON ** ADD FLASH RESET HERE ***
  reset_button = analogRead(A1);
  if (reset_button>500 && channelbeats[0][2]>0){
    for (a=0; a<channels; a++){
      channelbeats[a][2]=0; 

    }

  }

// TURN OFF ANY LIGHTS THAT ARE ON 
  if (time-last_sync>length && lights_active==true){
    for(a=0;a<channels;a++){  
      lc.setLed(0,7,5-(a*2),false);
              lc.setLed(0,7,4,false); // spare pin flash 

    }
      lights_active = false; 
  }


// FINISH ANY PULSES THAT ARE ACTIVE - PULSES LAST 1/4 AS LONG AS LIGHTS 
  if (time-last_sync>(length/4) && pulses_active==true){
    for(a=0;a<channels;a++){  
      digitalWrite(11+a,LOW);
        digitalWrite(sparepin, LOW);

    }
      pulses_active = false; 
  }



}




// Euclid calculation function 

unsigned int euclid(int n, int k, int o){ // inputs: n=total, k=beats, o = offset
  int pauses = n-k;
  int pulses = k;
  int offset = o;
  int steps = n;
  int per_pulse = pauses/k;
  int remainder = pauses%pulses;  
  unsigned int workbeat[n];
  unsigned int outbeat;
  uint16_t outbeat2;
  unsigned int working;
  int workbeat_count = n;
  int a; 
  int b; 
  int trim_count;
  for (a=0;a<n;a++){ // Populate workbeat with unsorted pulses and pauses 
    if (a<pulses){
      workbeat[a] = 1;
    }
    else {
      workbeat [a] = 0;
    }
  }

  if (per_pulse>0 && remainder <2){ // Handle easy cases where there is no or only one remainer  
    for (a=0;a<pulses;a++){
      for (b=workbeat_count-1; b>workbeat_count-per_pulse-1;b--){
        workbeat[a]  = ConcatBin (workbeat[a], workbeat[b]);
      }
      workbeat_count = workbeat_count-per_pulse;
    }

    outbeat = 0; // Concatenate workbeat into outbeat - according to workbeat_count 
    for (a=0;a < workbeat_count;a++){
      outbeat = ConcatBin(outbeat,workbeat[a]);
    }


    if (offset >0) {
      outbeat2 = rightRotate(offset,outbeat,steps); // Add offset to the step pattern
    } else {
      outbeat2 = outbeat;
    }
        

    
    return outbeat2;
  }

  else { 


    int groupa = pulses;
    int groupb = pauses; 
    int iteration=0;
    if (groupb<=1){
    }
    while(groupb>1){ //main recursive loop


      if (groupa>groupb){ // more Group A than Group B
        int a_remainder = groupa-groupb; // what will be left of groupa once groupB is interleaved 
        trim_count = 0;
        for (a=0; a<groupa-a_remainder;a++){ //count through the matching sets of A, ignoring remaindered
          workbeat[a]  = ConcatBin (workbeat[a], workbeat[workbeat_count-1-a]);
          trim_count++;
        }
        workbeat_count = workbeat_count-trim_count;

        groupa=groupb;
        groupb=a_remainder;
      }


      else if (groupb>groupa){ // More Group B than Group A
        int b_remainder = groupb-groupa; // what will be left of group once group A is interleaved 
        trim_count=0;
        for (a = workbeat_count-1;a>=groupa+b_remainder;a--){ //count from right back through the Bs
          workbeat[workbeat_count-a-1] = ConcatBin (workbeat[workbeat_count-a-1], workbeat[a]);

          trim_count++;
        }
        workbeat_count = workbeat_count-trim_count;
        groupb=b_remainder;
      }




      else if (groupa == groupb){ // groupa = groupb 
        trim_count=0;
        for (a=0;a<groupa;a++){
          workbeat[a] = ConcatBin (workbeat[a],workbeat[workbeat_count-1-a]);
          trim_count++;
        }
        workbeat_count = workbeat_count-trim_count;
        groupb=0;
      }

      else {
        //        Serial.println("ERROR");
      }
      iteration++;
    }


    outbeat = 0; // Concatenate workbeat into outbeat - according to workbeat_count 
    for (a=0;a < workbeat_count;a++){
      outbeat = ConcatBin(outbeat,workbeat[a]);
    }

    if (offset >0) {
      outbeat2 = rightRotate(offset,outbeat,steps); // Add offset to the step pattern
    } else {
      outbeat2 = outbeat;
    }
        
 
    return outbeat2;

  }
}


/*Function to right rotate n by d bits*/


uint16_t rightRotate(int shift, uint16_t value, uint8_t pattern_length) {

uint16_t mask = ((1 << pattern_length) - 1);
value &= mask;
return ((value >> shift) | (value << (pattern_length - shift))) & mask;

}


 


// Function to find the binary length of a number by counting bitwise 
int findlength(unsigned int bnry){
  boolean lengthfound = false;
  int length=1; // no number can have a length of zero - single 0 has a length of one, but no 1s for the sytem to count
  for (int q=32;q>=0;q--){
    int r=bitRead(bnry,q);
    if(r==1 && lengthfound == false){
      length=q+1;
      lengthfound = true;
    }
  }
  return length;
}

// Function to concatenate two binary numbers bitwise 
unsigned int ConcatBin(unsigned int bina, unsigned int binb){
  int binb_len=findlength(binb);
  unsigned int sum=(bina<<binb_len);
  sum = sum | binb;
  return sum;
}


// routine triggered by each beat
void Sync(){ 
  if (sleep == true)// wake up routine & animation 
  {  
    lc.shutdown(0,false); 
    sleep = false;
    wakeanim();
  }


  // clear bottom row 
  //lc.setRow(0,7,0);

  if(masterclock%2==0){ // tick bottom left corner on and off with clock 
    lc.setLed(0,7,7,true);
  }
  else{
    lc.setLed(0,7,7,false);
  }


  // Cycle through channels 
  for(a=0;a<channels;a++){

    read_head = channelbeats[a][0]-channelbeats[a][2]-1;  
  if (a != active_channel  || time-last_changed>display_update) // don't clear or draw cursor if channel is being changed 
{


    lc.setRow(0,a*2,0);//clear line above active row
    
    if (channelbeats[a][2]<8){
    
    for (int c=0;c<8;c++){ 
      if (bitRead (beat_holder[a],channelbeats[a][0]-1-c)==1 && c<channelbeats[a][0]){
        lc.setLed(0,a*2,7-c,true);
      }  
    }
    }
   else {
        for (int c=8;c<16;c++){  
     
      if (bitRead (beat_holder[a],channelbeats[a][0]-1-c)==1 && c<channelbeats[a][0]){
            lc.setLed(0,a*2,15-c,true);
           } 
    }
   }


    lc.setRow(0,a*2+1,0);//clear active row
    // draw cursor 
    if (channelbeats[a][2]<8){  
      lc.setLed(0,a*2+1,7-channelbeats[a][2],true); // write cursor less than 8
    }
    else if(channelbeats[a][2]>=8 && channelbeats[a][2]<16){
      lc.setLed(0,a*2+1,15-channelbeats[a][2],true); // write cursor more than 8
    }  
  
}
  // turn on pulses on channels where a beat is present  
  if (bitRead (beat_holder[a],read_head)==1){
    digitalWrite(11+a,HIGH); // pulse out 
    lc.setLed(0,7,5-(a*2),true); // bottom row flash 
    pulses_active = true;   
        lights_active = true;   
  }

  // send off pulses to spare output for the first channel 
  if (bitRead (beat_holder[a],read_head)==0 && a == 0){ // only relates to first channel 
    digitalWrite(sparepin,HIGH); // pulse out 
    lc.setLed(0,7,4,true); // bottom row flash 
    pulses_active = true;   
        lights_active = true;   
  }

  

  // move counter to next position, ready for next pulse  
  channelbeats[a][2]++;
  if (channelbeats[a][2]>=channelbeats[a][0]){
    channelbeats[a][2] = 0; 
  }
  }



  masterclock++;
  if (masterclock>=16){
    masterclock=0;
  };

  looptracker++;

length = ((time-last_sync)/5);
  last_sync = time;

}

/* 3 functions to read each encoder   
 returns +1, 0 or -1 dependent on direction 
 Contains no internal debounce, so calls should be delayed 
 */

int EncodeReadK(){ 

  int result=0;
  if (EncK.read() == 0) { 
  EncK.write(0);
  result=0;
  }
  else if (EncK.read() < 0) {  
    result=-1;
    EncK.write(0);
  }  
  else if (EncK.read() > 0) { 
    result=1;
    EncK.write(0);
  }  
  return result;
}

int EncodeReadN(){ 

  int result=0;
  if (EncN.read() == 0) { 
  EncN.write(0);
  result=0;
  }
  else if (EncN.read() < 0) {  
    result=-1;
    EncN.write(0);
  }  
  else if (EncN.read() > 0) { 
    result=1;
    EncN.write(0);
  }  
  return result;
}

int EncodeReadO(){ 

  int result=0;
  if (EncO.read() == 0) { 
  EncO.write(0);
  result=0;
  }
  else if (EncO.read() < 0) {  
    result=-1;
    EncO.write(0);
  }  
  else if (EncO.read() > 0) { 
    result=1;
    EncO.write(0);
  }  
  return result;
}


// Matrix LED wake-up and sleep animation

void wakeanim(){
  for (a=4; a>0;a--){
    lc.setRow(0,a,255); 
    lc.setRow(0,8-a,255); 
    delay(100);
    lc.setRow(0,a,0);
    lc.setRow(0,8-a,0); 
  }
}
void sleepanim(){
  for (a=0; a<4;a++){
    lc.setRow(0,a,255); 
    lc.setRow(0,8-a,255); 
    delay(200);
    lc.setRow(0,a,0);
    lc.setRow(0,8-a,0); 
  }

}  






