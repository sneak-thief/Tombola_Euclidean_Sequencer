# Tombola_Euclidean_Sequencer

Updated version of Tombola's modular euclidean sequencer:

https://www.muffwiggler.com/forum/viewtopic.php?t=45485&start=all&postdays=0&postorder=asc



Instructions:

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


Sneak-Thief's design notes

- Added Encoder library to stabilize encoder use
- Cleaned up code a bit
- Created schematic with input/output protection: http://sneak-thief.com/modular/tombolas-euclidean-0.3.png
- Added reset gate input
- Added Offset control 
- Added 10K pulldown resistor to switch outputs
- Changed 15K resistors to 10K
- Added more error checking for eeprom reads / values to reduce risk of crashes 

 To do 
- Add CV control of N, K and Offset using analog pins 5,6,7 
- OR Add gate control of two pairs of N, K or Offset using analog pins 4,5,6,7 


Tombola's original design notes

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
 
 

