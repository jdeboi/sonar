
#include <NewPing.h>
#include <WaveHC.h>
#include <WaveUtil.h>
/*
 * Define macro to put error messages in flash memory
 */
#define error(msg) error_P(PSTR(msg))

SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

uint8_t dirLevel; // indent level for file/dir names    (for prettyprinting)
dir_t dirBuf;     // buffer for directory reads

boolean playWave;
// Function definitions (we define them here, but the code is below)
void play(FatReader &dir);

int echoPin = 6;
int trigPin = 7;
int duration;
float distance;
int timeout = 9000;

// CHANGE THIS VALUE FOR ADJUSTING
// HIGHER FOR FARTHER, LOWER FOR CLOSER
int triggerDistance = 8000;
boolean isPlaying;
int count = 0;
int max_count = 50;

boolean triggered = false;

//NewPing sonar(trigPin, echoPin, triggerDistance);

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
 
  isPlaying = false;
  Serial.begin(9600);           // set up Serial library at 9600 bps for debugging
  initialize(); 
}

void loop() {
  if(checkSonar()) {
    Serial.println("checked");
    root.rewind();
    play(root);
  }
  else {
//    Serial.println("no good");
    wave.stop();
  }
}

/*
 * play recursively - possible stack overflow if subdirectories too nested
 */
void play(FatReader &dir) {
  FatReader file;
  while (dir.readDir(dirBuf) > 0) {    // Read every file in the directory one at a time
  
    // Skip it if not a subdirectory and not a .WAV file
    if (!DIR_IS_SUBDIR(dirBuf)
         && strncmp_P((char *)&dirBuf.name[8], PSTR("WAV"), 3)) {
      continue;
    }

    Serial.println();            // clear out a new line
    
    for (uint8_t i = 0; i < dirLevel; i++) {
       Serial.write(' ');       // this is for prettyprinting, put spaces in front
    }
    if (!file.open(vol, dirBuf)) {        // open the file in the directory
      error("file.open failed");          // something went wrong
    }
    
    if (file.isDir()) {                   // check if we opened a new directory
      putstring("Subdir: ");
      printEntryName(dirBuf);
      Serial.println();
      dirLevel += 2;                      // add more spaces
      // play files in subdirectory
      play(file);                         // recursive!
      dirLevel -= 2;    
    }
    else {
      // Aha! we found a file that isnt a directory
      putstring("Playing ");
      printEntryName(dirBuf);              // print it out
      if (!wave.create(file)) {            // Figure out, is it a WAV proper?
        putstring(" Not a valid WAV");     // ok skip it
      } else {
        Serial.println();                  // Hooray it IS a WAV proper!
//        if(checkSonar())
        wave.play();                       // make some noise!
        isPlaying = true;
        uint8_t n = 0;
        while (wave.isplaying) {// playing occurs in interrupts, so we print dots in realtime
          if(!checkSonar())
            break;
          putstring(".");
          if (!(++n % 32))Serial.println();
          delay(5);
        }
        wave.stop();       
        sdErrorCheck();                    // everything OK?
        // if (wave.errors)Serial.println(wave.errors);     // wave decoding errors
      }
    }
  }
}

boolean checkSonar2() {
  // Send ping, get ping time in microseconds (uS).
  //unsigned int uS = sonar.ping();
  //distance = uS/58.2;
  Serial.println(distance);
  if (distance <= triggerDistance) return true;
  return false;
}

boolean checkSonar() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH, timeout);
  //Serial.print("duration ");
  //Serial.println(duration);
  boolean switchModes = false;
  if ( duration > 100 && duration < triggerDistance){
    count++;
    if(isPlaying)
      switchModes = false;
    else
      switchModes = true;
  }
  else{
    count++;
    if(isPlaying)
      switchModes = true;
    else
      switchModes = false;
  }
  //erial.print("count:");
  //Serial.println(count);
  if(switchModes && count > max_count){
    count = 0;
    isPlaying = !isPlaying;
    Serial.print("\n\n\nSWITCH");
    Serial.println(isPlaying);
  }
  if(count > max_count)
    count = 0;
  return isPlaying;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
void initialize() {
putstring_nl("\nWave test!");  // say we woke up!
  
  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(FreeRam());

  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    error("Card init. failed!");  // Something went wrong, lets print out why
  }
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
  
  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {   // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                           // we found one, lets bail
  }
  if (part == 5) {                     // if we ended up not finding one  :(
    error("No valid FAT partition!");  // Something went wrong, lets print out why
  }
  
  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(), DEC);     // FAT16 or FAT32?
  
  // Try to open the root directory
  if (!root.openRoot(vol)) {
    error("Can't open root dir!");      // Something went wrong,
  }
  
  // Whew! We got past the tough parts.
  putstring_nl("Files found (* = fragmented):");

  // Print out all of the files in all the directories.
  root.ls(LS_R | LS_FLAG_FRAGMENTED);
}  


/////////////////////////////////// HELPERS
/*
 * print error message and halt
 */
void error_P(const char *str) {
  PgmPrint("Error: ");
  SerialPrint_P(str);
  sdErrorCheck();
  while(1);
}


/*
 * print error message and halt if SD I/O error, great for debugging!
 */
void sdErrorCheck(void) {
  if (!card.errorCode()) return;
  PgmPrint("\r\nSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  PgmPrint(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}
