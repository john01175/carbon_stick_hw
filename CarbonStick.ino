#include "config.h"
#include <WiFiManager.h>

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <TinyGPSPlus.h>
#include <TimeLib.h>

WiFiManager wifiManager;

//GPS
TinyGPSPlus gps;
float latitude, longitude;

//ULTRASONIC
long duration1;
float distanceCm1;
long duration2;
float distanceCm2;

//WATER
float waterReading;

char Date[]  = "2000-00-00T00:00:00";
byte last_second, Second, Minute, Hour, Day, Month;
int Year;
#define time_offset 28800  // define a clock offset of 3600 seconds (8 hour) ==> UTC + 8
void getGps(float& latitude, float& longitude); //GPS function

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
//#define WIFI_SSID "PLDTHOMEFIBR48a08_JB"
//#define WIFI_PASSWORD "MusicaGraceJb@1234"

// Insert Firebase project API Key
#define API_KEY "AIzaSyBHSJ5ZTNy3YedpjT9kSzNAeVkwMKdU9fk"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://carbonstick-6b931-default-rtdb.asia-southeast1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

void setup(){

  Serial.begin(115200);

  Serial2.begin(9600); //for GPS G17(rx), G16(tx)

  //wifi connect  
  wifiConnect();
  delay(1000);

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(waterSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(vibrationPin, OUTPUT);

  delay(3000);

  /*WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
*/
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop(){

  ReadUltrasonic1();
  ReadUltrasonic2();
  ReadWaterSensor();
  BuzzerLogic();
  VibrationLogic();

  getGps(latitude, longitude);

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
   
    
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "CGPS1/Latitude", latitude)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

     // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "CGPS1/Longitude", longitude)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setString(&fbdo, "CGPS1/Date", String(Date))){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setString(&fbdo, "CGPS1/Alarm", "NA")){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

//getGps() Function
void getGps(float& latitude, float& longitude)
{
  // Can take up to 60 seconds
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (Serial2.available()){
      if (gps.encode(Serial2.read())){
        newData = true;
        break;
      }
    }
  }

  if (newData) //If newData is true
  {
    latitude = gps.location.lat();
    longitude = gps.location.lng();

    // get time from GPS module
      if (gps.time.isValid())
      {
        Minute = gps.time.minute();
        Second = gps.time.second();
        Hour   = gps.time.hour();
      }
 
      // get date drom GPS module
      if (gps.date.isValid())
      {
        Day   = gps.date.day();
        Month = gps.date.month();
        Year  = gps.date.year();
      }
 
      if(last_second != gps.time.second())  // if time has changed
      {
        last_second = gps.time.second();
 
        // set current UTC time
        setTime(Hour, Minute, Second, Day, Month, Year);
        // add the offset to get local time
        adjustTime(time_offset);
 
        
 
        // update date array
        Date[2] = (year()  / 10) % 10 + '0';
        Date[3] =  year()  % 10 + '0';
        Date[5]  =  month() / 10 + '0';
        Date[6] =  month() % 10 + '0';
        Date[8]  =  day()   / 10 + '0';
        Date[9]  =  day()   % 10 + '0';

        // update time array
        Date[17] = second() / 10 + '0';
        Date[18] = second() % 10 + '0';
        Date[14]  = minute() / 10 + '0';
        Date[15] = minute() % 10 + '0';
        Date[11]  = hour()   / 10 + '0';
        Date[12]  = hour()   % 10 + '0';
 
      }

    newData = false;
  }
  else {
    Serial.println("No GPS data is available");
    latitude = 0;
    longitude = 0;
  }
}

void ReadUltrasonic1()
{
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration1 = pulseIn(echoPin1, HIGH);

  // Calculate the distance
  distanceCm1 = duration1 * SOUND_SPEED/2;

  // Prints the distance in the Serial Monitor
  Serial.print("Distance 1 (cm): ");
  Serial.println(distanceCm1);
}

void ReadUltrasonic2()
{
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration2 = pulseIn(echoPin2, HIGH);

  // Calculate the distance
  distanceCm2 = duration2 * SOUND_SPEED/2;

  // Prints the distance in the Serial Monitor
  Serial.print("Distance 2 (cm): ");
  Serial.println(distanceCm2);
}

void BuzzerLogic()
{
  if(distanceCm1 <= SENSOR1_MIN_DIST_ALARM)
  {
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
  }

  else if(distanceCm2 <= SENSOR2_MIN_DIST_ALARM)
  {
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
  }

  else if(waterReading >= WATERSENSOR_ALARM_LEVEL){
       digitalWrite(buzzerPin, HIGH);
       delay(1000);
       digitalWrite(buzzerPin, LOW);
       delay(1000);
       digitalWrite(buzzerPin, HIGH);
       delay(1000);
      digitalWrite(buzzerPin, LOW);
      delay(1000);
      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      digitalWrite(buzzerPin, LOW);
      delay(1000);
  }

  else{
    digitalWrite(buzzerPin, LOW);
  }
}

void VibrationLogic(){

  if(distanceCm1 <= SENSOR1_MIN_DIST_ALARM){

    digitalWrite(vibrationPin, HIGH);
    delay(1000);

    digitalWrite(vibrationPin, LOW);
    delay(1000);

    digitalWrite(vibrationPin, HIGH);
    delay(1000);

    digitalWrite(vibrationPin, LOW);
    delay(1000);

    digitalWrite(vibrationPin, HIGH);
    delay(1000);

    digitalWrite(vibrationPin, LOW);
    delay(1000);
  }

  else if(distanceCm2 <= SENSOR2_MIN_DIST_ALARM){

    digitalWrite(vibrationPin, HIGH);
    delay(1000);

    digitalWrite(vibrationPin, LOW);
    delay(1000);

    digitalWrite(vibrationPin, HIGH);
    delay(1000);

    digitalWrite(vibrationPin, LOW);
    delay(1000);

    digitalWrite(vibrationPin, HIGH);
    delay(1000);

    digitalWrite(vibrationPin, LOW);
    delay(1000);
  }

  else if(waterReading >= WATERSENSOR_ALARM_LEVEL){
      digitalWrite(vibrationPin, HIGH);
      delay(1000);

      digitalWrite(vibrationPin, LOW);
      delay(1000);

      digitalWrite(vibrationPin, HIGH);
      delay(1000);

      digitalWrite(vibrationPin, LOW);
      delay(1000);

      digitalWrite(vibrationPin, HIGH);
      delay(1000);

      digitalWrite(vibrationPin, LOW);
      delay(1000);
   }

  else{
    digitalWrite(vibrationPin, LOW);
  }
}

void ReadWaterSensor()
{
  waterReading = analogRead(waterSensorPin);

  Serial.print("Water: "); Serial.println(waterReading);

  

}

