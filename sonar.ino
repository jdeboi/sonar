
int echoPin = 7;
int trigPin = 8;
int duration;
int timeout = 9000;
float fraction = 0.3;
float aveDuration;
float distance;

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  checkSonar();
}

void checkSonar() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH, timeout);
  if(duration == 0) {
    if(aveDuration > 120) {
      duration = timeout;
    }
    else duration = aveDuration;
  }
  /*
  if(duration == 0) {
    if(aveDuration > 130) {
      Serial.println("timed out");
      duration = timeout;
    }
    else duration = aveDuration;
  }
  //else Serial.println(duration);
  */
 
  aveDuration = aveDuration * (1.0-fraction) + duration * fraction;
  distance = aveDuration/58.2; 
  Serial.println(distance);
  delay(150);
}
