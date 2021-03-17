#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <iarduino_RF433_Receiver.h>                      // Подключаем библиотеку для работы с приёмником MX-RM-5V
#include "MillisTimer.h"

#include "SensorData.h"
SensorData sensorData[4];

int radioStatusLedPin = A0;

LiquidCrystal_I2C lcd(0x27, 20, 4);

iarduino_RF433_Receiver radio(3);                         // Создаём объект radio для работы с библиотекой iarduino_RF433, указывая номер вывода к которому подключён приёмник (можно подключать только к выводам использующим внешние прерывания)

MillisTimer timer = MillisTimer();
bool timerStatus = false;
String timerState = "OFF";
unsigned long DEFAULT_TIMEOUT = 2400000;

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
    Serial.begin(9600);
    initLcd();
    initRadio();
    resetTimer(timer);

    pinMode(radioStatusLedPin, OUTPUT);
}

void loop(){
    if(radio.available()){   
        digitalWrite(radioStatusLedPin, 25);
        lcd.setCursor(0, 0);
        lcd.print("Timer     ");
        lcd.setCursor(0, 1);
        lcd.print("Air temp1");
        lcd.setCursor(0, 2);
        lcd.print("Humidity");
        lcd.setCursor(0, 3);
        lcd.print("Air temp2");


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

        SensorData humiditySensor = sensorData[2];
        lcd.setCursor(10, 2);
        lcd.print(humiditySensor.sensorValue);

        SensorData airTemperatureSensor = sensorData[3];
        lcd.setCursor(10, 3);
        lcd.print(airTemperatureSensor.sensorValue);

        delay(100);
    } else {
        digitalWrite(radioStatusLedPin, 0);
    }                                                    // Если вызвать функцию available с параметром в виде ссылки на переменную типа uint8_t, то мы получим номер трубы, по которой пришли данные (см. урок 26.5)
    
    if (timer.isRunning() & !timer.expired()) {
        renderTimer(timer);
    }
}
