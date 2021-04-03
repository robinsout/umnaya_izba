#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <iarduino_RF433_Receiver.h>                      // Подключаем библиотеку для работы с приёмником MX-RM-5V
#include "MillisTimer.h"

#include "SensorData.h"
SensorData sensorData[4];

#include <SoftwareSerial.h>
SoftwareSerial espSerial(9, 10);      // TX, RX on ESP8266

#include "ThingSpeakApiKey.h"
ThingSpeakApiKey thingSpeakApiKey;

bool DEBUG = true;   //show more logs
int responseTime = 2000; //communication timeout

int radioStatusLedPin = A0;

LiquidCrystal_I2C lcd(0x27, 20, 4);

iarduino_RF433_Receiver radio(3);                         // Создаём объект radio для работы с библиотекой iarduino_RF433, указывая номер вывода к которому подключён приёмник (можно подключать только к выводам использующим внешние прерывания)

MillisTimer timer = MillisTimer();
bool timerStatus = false;
String timerState = "OFF";
unsigned long DEFAULT_TIMEOUT = 2400000;

/*
* Name: sendToWifi
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/
String sendToWifi(String command, const int timeout, boolean debug){
  String response = "";
  espSerial.println(command); // send the read character to the esp8266
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(espSerial.available())
    {
    // The esp has data so display its output to the serial window 
    char c = espSerial.read(); // read the next character.
    response+=c;
    }  
  }
  if(debug)
  {
    Serial.println(response);
  }
  return response;
}

/*
* Name: sendData
* Description: Function used to send string to tcp client using cipsend
* Params: 
* Returns: void
*/
String sendData(String str, int responseTime, bool DEBUG){
    sendToWifi("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80",responseTime,DEBUG);

    String len="";
    len+=str.length();
    String response = "";
    response += sendToWifi("AT+CIPSEND="+len,responseTime,DEBUG);
    delay(100);
    response += sendToWifi(str,responseTime,DEBUG);
    delay(100);
    return response;
}

void initSerial() {
    Serial.begin(9600);
    espSerial.begin(9600);
}

void initRadio() {
    radio.begin();                                        // Инициируем работу приёмника MX-RM-5V (в качестве параметра можно указать скорость ЧИСЛО бит/сек, тогда можно не вызывать функцию setDataRate)
    radio.setDataRate     (i433_100BPS);                   // Указываем скорость приёма данных (i433_5KBPS, i433_4KBPS, i433_3KBPS, i433_2KBPS, i433_1KBPS, i433_500BPS, i433_100BPS), i433_1KBPS - 1кбит/сек
    radio.openReadingPipe (5);                            // Открываем 5 трубу для приема данных (если вызвать функцию без параметра, то будут открыты все трубы сразу, от 0 до 7)
//  radio.openReadingPipe (2);                            // Открываем 2 трубу для приёма данных (таким образом можно прослушивать сразу несколько труб)
//  radio.closeReadingPipe(2);                            // Закрываем 2 трубу от  приёма данных (если вызвать функцию без параметра, то будут закрыты все трубы сразу, от 0 до 7)
    radio.startListening  ();                             // Включаем приемник, начинаем прослушивать открытую трубу
//  radio.stopListening   ();                             // Выключаем приёмник, если потребуется
}

void initLcd() {
    lcd.begin();
    lcd.backlight();
    lcd.print("= UMNAYA BANYA =");
    delay(1000);
}

String checkZeroSign(int value) {
    if (value < 10) {
        return "0" + String(value);
    }
    return String(value);
}

String formatTime(unsigned long timerValue) {
    String minutes = checkZeroSign((timerValue / 1000)  / 60);
    String seconds = checkZeroSign((timerValue / 1000) % 60);

    return minutes + ":" + seconds;
}

void renderTimer(MillisTimer &mt) {
    lcd.setCursor(10, 0);
    lcd.print(formatTime(mt.getRemainingTime()) + "  ");
}

void expireTimer(MillisTimer &mt) {
    timerState = "EXPIRED";
    mt.reset();
    lcd.setCursor(10, 0);
    lcd.print("EXPIRED");
}

void resetTimer(MillisTimer &mt) {
    timerState = "OFF";
    mt.reset();
    lcd.setCursor(10, 0);
    lcd.print("OFF   ");
}

void startTimer(MillisTimer &mt) {
    timerState = "RUNNING";
    mt.setInterval(DEFAULT_TIMEOUT);
    mt.expiredHandler(expireTimer);
    mt.setRepeats(1);
    mt.start();
}

void setup(){
    initLcd();
    initRadio();
    resetTimer(timer);
    initSerial();

    pinMode(radioStatusLedPin, OUTPUT);
}

void loop(){
    if(radio.available()){                          // Если вызвать функцию available с параметром в виде ссылки на переменную типа uint8_t, то мы получим номер трубы, по которой пришли данные (см. урок 26.5)
        digitalWrite(radioStatusLedPin, 25);
        lcd.setCursor(0, 0);
        lcd.print("Timer     ");
        lcd.setCursor(0, 1);
        lcd.print("Air temp1");
        lcd.setCursor(0, 2);
        lcd.print("Air temp2");
        lcd.setCursor(0, 3);
        lcd.print("Humidity");

        radio.read(&sensorData, sizeof(sensorData));                  // Читаем данные в массив data и указываем сколько байт читать

        SensorData timerData = sensorData[0];
        bool timerTrigger = timerData.sensorValue != 0;

        if ((timerTrigger != timerStatus)) {
            timerStatus = timerTrigger;
            if( timerState == "RUNNING" ) {
                resetTimer(timer);
            }
            else if( (timerState == "OFF") | (timerState == "EXPIRED") ) {
                startTimer(timer);
            }
        }
        
        SensorData sensorDuo = sensorData[1];
        lcd.setCursor(10, 1);
        lcd.print(sensorDuo.sensorValue);

        SensorData airTemperatureSensor = sensorData[2];
        lcd.setCursor(10, 2);
        lcd.print(airTemperatureSensor.sensorValue);

        SensorData humiditySensor = sensorData[3];
        lcd.setCursor(10, 3);
        lcd.print(humiditySensor.sensorValue);

        String GET = "GET /update";
        GET += "?api_key="+thingSpeakApiKey.apiKey;
        GET += "&field1="+String(sensorDuo.sensorValue);
        GET += "&field2="+String(airTemperatureSensor.sensorValue);
        GET += "&field3="+String(humiditySensor.sensorValue);
        GET += "\r\n\r\n";
        sendData(GET,responseTime,DEBUG);
    } else {
        digitalWrite(radioStatusLedPin, 0);
    }

    if (timer.isRunning() & !timer.expired()) {
        renderTimer(timer);
    }
}
