/**
 * https://www.codesdope.com/cpp-array-of-objects/ — создание массива объектов
 */

#include <Arduino.h>
#include <iarduino_RF433_Transmitter.h>                   // Подключаем библиотеку для работы с передатчиком FS1000A
#include "SensorData.h"

iarduino_RF433_Transmitter radio(12);                     // Создаём объект radio для работы с библиотекой iarduino_RF433, указывая номер вывода к которому подключён передатчик
SensorData sensorData[2];

const int ledPin = 13;

const int buttonPin = 2;
bool buttonState = false;

// // =========== Датчик температуры воды ============
// // библиотека для работы с протоколом 1-Wire
// #include <OneWire.h>
// // библиотека для работы с датчиком DS18B20
// #include <DallasTemperature.h>
// // сигнальный провод датчика
// #define ONE_WIRE_BUS 5
// // создаём объект для работы с библиотекой OneWire
// OneWire oneWire(ONE_WIRE_BUS);
// DallasTemperature sensor(&oneWire);
// //==============================================

void updateTimerButtonState() {
  buttonState = !buttonState;
  volatile byte ledState = buttonState;
  digitalWrite(ledPin, ledState);
}

// void initWaterTemperatureSensor() {
  // =========== Датчик температуры воды ============
  // начинаем работу с датчиком
  // sensor.begin();
  // устанавливаем разрешение датчика от 9 до 12 бит
  // sensor.setResolution(12);
  //==============================================
// }

// float getWaterTemperature() {
  // =========== Датчик температуры воды ============
  // переменная для хранения температуры
  // float waterTemperature;
  // отправляем запрос на измерение температуры
  // sensor.requestTemperatures();
  // считываем данные из регистра датчика
  // waterTemperature = sensor.getTempCByIndex(0);
  // выводим температуру в Serial-порт
  // return waterTemperature;
// }

float getAirTemperature() {
  const int B = 4275;               // B value of the thermistor
  const long R0 = 100000;            // R0 = 100k

  int rawData = analogRead(A1);
  
  float R = 1023.0/rawData-1.0;
  R = R0*R;
  float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15;
  return temperature;
}


void initRadio() {  
  radio.begin();                                        // Инициируем работу передатчика FS1000A (в качестве параметра можно указать скорость ЧИСЛО бит/сек, тогда можно не вызывать функцию setDataRate)
  radio.setDataRate     (i433_100BPS);                   // Указываем скорость передачи данных (i433_5KBPS, i433_4KBPS, i433_3KBPS, i433_2KBPS, i433_1KBPS, i433_500BPS, i433_100BPS), i433_1KBPS - 1кбит/сек
  radio.openWritingPipe (5);                            // Открываем 5 трубу для передачи данных (передатчик может передавать данные только по одной из труб: 0...7)
}                                                     // Если повторно вызвать функцию openWritingPipe указав другой номер трубы, то передатчик начнёт передавать данные по вновь указанной трубе

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(buttonPin), updateTimerButtonState, FALLING);

  initRadio();
  // initWaterTemperatureSensor();
}                                                       

void loop() {
  sensorData[0] = {
    sensorId: 1,
    sensorValue: float(buttonState)
  };


  sensorData[1] = {
    sensorId: 2,
    sensorValue: getAirTemperature()
  };

  // sensorData[2] = {
  //   sensorId: 3,
  //   sensorValue: getWaterTemperature()
  // };
  Serial.print("Sending data");
  radio.write(&sensorData, sizeof(sensorData));              // отправляем данные из массива data указывая сколько байт массива мы хотим отправить
}
