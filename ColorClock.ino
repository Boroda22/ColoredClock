// Часы на WS2812b
// Версия 1.0
// Дата написания 10.11.2022

/* СОЕДИНЕНИЕ СИГМЕНТОВ
Последоватеольность соединения сигментов индикатора
   {G B A F E D C} {G B A F E D C} {верхняя точка} {нижняя точка} {G B A F E D C} {G B A F E D C}
   Количество светодиодов выберается относительно количества светотдиодов на сегмент

ВОЗМОЖНОСТИ
  Отображают дату, время, температуру, 255 цетов

*/

#include <microDS3231.h>
#include <FastLED.h>
#include <EEPROMex.h>
// #include <DHT.h>
#include <SoftwareSerial.h> // библиотека bluetooth
#include <AsyncStream.h>    //
#include <Wire.h>
#include <GParser.h>

// Настройки пинов подключения к Ардуино +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define BT_TX_PIN 2 // pin bluetooth TX
#define BT_RX_PIN 3 // pin bluetooth RX
#define DATA_PIN 7  // pin вывод данных на светодиоды
// #define DHT_PIN 5                 // pin датчика температуры только плюсовая до десятых
//  Настройки +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// #define DHTTYPE DHT22              // Тип датчика температуры
#define LED_TYPE WS2812 // Тип светодиодов (WS2812, WS2811 и т.д.)
#define COLOR_ORDER GRB // Порядок цветов (если при включении часы не красные менять буквы местами)
#define SEGMENT_LEDS 1  // Количество светодиодов на сегмент (точки по 1 светодиоду)
#define TIME_EXIT 10    // Время выхода из меню после последнего нажатия кнопок (секунд)
#define ANIME_DELAY 20  // Замедление анимации при смемене цвета в миллисекундах (не более 30)
#define ZERO_WATCH      // Закоментировать строку для отображения первого нуля в часах
#define NUM_LEDS ((SEGMENT_LEDS * 28) + 2)
#define POINT_UP (14 * SEGMENT_LEDS)
#define POINT_DW ((14 * SEGMENT_LEDS) + 1)
#define COLOR_CONNECT 0xFF1FC2
// Настройки ночного режима +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define TIME_NIGHT_BEGIN 21  // Время включения ночного режима (в часах)
#define TIME_NIGHT_END 7     // Время выключения ночного режима (в часах)
#define COLOR_NIGHT 0x008FB3 // Цвет часов в ночном режиме
#define COLOR_NOONE 0x00CCFF // Цвет часов в дневном режиме
// #define COLOR_DEF   0x00FF66       // Цвет часов по умолчанию
//  Настройки отображения температуры ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define TEMP_1_SYMBOL 10 // Символ градусов датчика температуры  (10 - "0", 11 - "С")
#define TEMP_1_COLOR 1   // Вариант цвета 1 датчика (0 - как часы, 1 - кр-ор-ж-зел-гол-с, 2 - кр-роз-фиол-с)
#define TEMP_1_CORRECT 0 // Корректировка показания датчика температуры (в гр. С, десятые через точку)
#define TEMP_1_MIN 20    // Минимальная температура датчика (отображается синим)
#define TEMP_1_MAX 30    // Максимальная температура датчика (отображается красным)
// Defauil настройки  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define TIME_TEMP 4 // Установка времени отображения температуры 0...8 секунд (0 - отк.)
#define TIME_DATE 0 // Установка времени отображения даты 0...3 секунд (0 - отк.)
#define SET_COLOR 0 // Настройка цвета (если 0 то автоматическая) 0...20
#define SET_LIGHT 0 // Настройка яркости (если 0 то автоматическая) 0...25
#define SET_TEMPS 1 // Включение отображения температуры (0 - отк. 1 - вкл.)
#define SET_PRESS 1 // Включение отображения температуры и давления с BMP280 (0 - отк. 1 - темп., 2 - давл., 3 - оба)
#define SET_SHOWS 0 // Включение отображения анимации при смене минут (0 - отк. 1 - вкл. 2 - вкл. ночной режим)
#define TIME_TIME 5 // Установка количества отображения температуры и даты в минуту 1...5 раз
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MicroDS3231 RTC; // часы реального времени DS3231
// DHT dht(DHT_PIN, DHTTYPE); // датчик темперматуры
SoftwareSerial mySerial(BT_TX_PIN, BT_RX_PIN); // bluetooth
AsyncStream<100> bt(&mySerial, '#');           // указали Stream-объект и символ конца

CRGB leds[NUM_LEDS];
CRGB TEMPER_COLOR;         // цвет отображения температуры
CRGB LED_COLOR = 0;        // цвет светодиодов
CRGB DEF_COLOR = 0x00FF66; // цвет по умолчанию

byte digits[22] = {0b01111110,  // 0
                   0b01000010,  // 1
                   0b00110111,  // 2
                   0b01100111,  // 3
                   0b01001011,  // 4
                   0b01101101,  // 5
                   0b01111101,  // 6
                   0b01000110,  // 7
                   0b01111111,  // 8
                   0b01101111,  // 9
                   0b00001111,  // º 10
                   0b00111100,  // C 11
                   0b01011011,  // H 12
                   0b01011111,  // A 13
                   0b01101101,  // S 14
                   0b00000001,  // - 15
                   0b01110011,  // d 16
                   0b01111001,  // b 17
                   0b00000000,  //   18
                   0b00111001,  // t 19
                   0b00111000,  // L 20
                   0b00011111}; // P 21
// массив стандартных цветов
uint32_t standardColors[] = {0xFF3366,  // красный
                             0xFF6C00,  // оранж
                             0xFFCC00,  // желтый
                             0x00FF33,  // зеленый
                             0x3399FF,  // голубой
                             0x3300FF}; // синий

boolean eeprom_flag;
// boolean temp_flag = false;
boolean read_eeprom = true;
boolean mode_flag = true;
boolean night_flag = false;
boolean isConnected = false;
unsigned long t;
unsigned long opdating_time;
// byte color;
// byte old_color = 0;
byte mode = 55;  // Режим работы (по умолчанию отображать версию прошивки)
byte brightness; // Яркость
byte points = 0; // Состотяние точек
byte figure[4];  // Цыфры часов
byte setups[9];  // 0, TIME_TEMP, TIME_DATE, SET_COLOR, SET_LIGHT, SET_TEMPS, SET_SHOWS, TIME_TIME, SET_PRESS
// int temp1;        // Температура
int time_tp = 2000; // Промежуточное время вывода температуры
DateTime tm;        // Текущее время на часах

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CheckConnectionBt()
{ // Проверка подключения по bluetooth
  if (mySerial.isListening())
    isConnected = true;
  else
    isConnected = false;
}
void ParseCommand(const char *cmd)
{ // парсинг комманды
  String tmpCmd(cmd);
  // Serial.print("cmd: ");
  // Serial.println(tmpCmd);
  if (tmpCmd == "%")
  { // режим часов
    mode = 0;
    t = millis();
  }
  if (tmpCmd == "t")
  { // режим температуры
    mode = 1;
    t = millis();
  }
  if (tmpCmd == "d")
  { // режим даты
    mode = 2;
    t = millis();
    mySerial.println(RTC.getDateString() + "-" + RTC.getTimeString());
  }
  if (tmpCmd == "$")
  { // версия прошивки
    mode = 55;
    t = millis();
  }
  if (tmpCmd == "*")
  { // анимация
    // ResetClock();
    Animation();
  }
  if (tmpCmd.startsWith("setdate"))
  { // команда установки времени
    SetDate(tmpCmd);
  }
  if (tmpCmd.startsWith("setcolor="))
  { // команда установки текущего цвета
    SetCurrentColor(tmpCmd);
  }
  if (tmpCmd.startsWith("stdcolor="))
  { // команда установки текущего цвета
    SetStandardColor(tmpCmd);
  }

  cmd = "";
}
void SetDate(const String &cmd)
{ // Установка даты
  int startSubStr = cmd.indexOf('[');
  int endtSubStr = cmd.indexOf(']');
  if (startSubStr < 0 || endtSubStr < 0)
  {
    return;
  };
  String params = cmd.substring(startSubStr + 1, endtSubStr);
  Serial.println(params);
  GParser data(params.c_str(), ',');
  data.split();
  RTC.setTime(data.getInt(0), data.getInt(1), data.getInt(2), data.getInt(3), data.getInt(4), data.getInt(5));
  mySerial.println("OK");
}
void BrightnessCheck()
{                   // Установка яркости
  brightness = 250; // по умолчанию
  // сбавляем яркость в ночном режиме
  if (night_flag)
  {
    brightness = 100;
  }
  LEDS.setBrightness(brightness);
}

void TempColor(byte set_sh, int temp_x, int temp_min, int temp_max)
{ // Функция изменения цвета от температуры
  byte sum_color;
  if (set_sh == 0)
    TEMPER_COLOR = LED_COLOR;
  if (temp_x < temp_min * 10)
    temp_x = temp_min * 10;
  if (temp_x > temp_max * 10)
    temp_x = temp_max * 10;
  if (set_sh == 1)
    sum_color = map(temp_x, temp_min * 10, temp_max * 10, 170, 0);
  if (set_sh == 2)
    sum_color = map(temp_x, temp_min * 10, temp_max * 10, 170, 255);
  TEMPER_COLOR = CHSV(sum_color, 255, 255);
}

void DigitOut(byte digit_1, byte digit_2, byte digit_3, byte digit_4, byte point, CRGB d_color)
{ // Функция вывода на диоды
  byte digit[4] = {digit_1, digit_2, digit_3, digit_4};
  for (byte s = 0; s <= 3; s++)
  {
    byte cursor = figure[s];
    for (byte k = 0; k <= 6; k++)
    {
      for (byte i = 1; i <= SEGMENT_LEDS; i++)
      {
        if ((digits[digit[s]] & 1 << k) == 1 << k)
          leds[cursor] = d_color;
        else
          leds[cursor] = 0x000000;
        cursor++;
      }
    }
  }
  if (point == 0 && !isConnected)
  {
    leds[POINT_DW] = 0x000000;
    leds[POINT_UP] = 0x000000;
  }
  else if (point == 1 && !isConnected)
  {
    leds[POINT_DW] = d_color;
    leds[POINT_UP] = 0x000000;
  }
  else if (point == 1 && isConnected)
  {
    leds[POINT_DW] = d_color;
    leds[POINT_UP] = COLOR_CONNECT;
  }
  else if (point == 2 && !isConnected)
  {
    leds[POINT_DW] = d_color;
    leds[POINT_UP] = d_color;
  }
  else if (point == 2 && isConnected)
  {
    leds[POINT_DW] = d_color;
    leds[POINT_UP] = COLOR_CONNECT;
  }
}

void Animate(int v)
{ // Функция анимации
  static uint8_t hue = 0;
  leds[v] = CHSV(hue++, 255, 255);
  FastLED.show();
  for (byte m = 0; m < NUM_LEDS; m++)
  {
    leds[m].nscale8(250);
  }
  delay(ANIME_DELAY / SEGMENT_LEDS);
}

void Animation()
{ // Фунция анимации при смене минуты
  for (int i = 0; i < NUM_LEDS; i++)
    Animate(i);
  for (int i = NUM_LEDS - 1; i >= 0; i--)
    Animate(i);
}
void SetCurrentColor(const String &cmd)
{ // Функция установки текущего цвета
  int lengthCmd = cmd.length();
  String valueColor = cmd.substring(9, lengthCmd);

  uint8_t len = valueColor.length();
  // Serial.print("tmpVal= ");
  // Serial.println(valueColor);
  uint32_t val = 0;
  for (int i = 0; i < len; i++)
  {
    val <<= 4;
    uint8_t d = valueColor[i];
    d -= (d <= '9') ? 48 : ((d <= 'F') ? 55 : 87);
    val |= d;
  }
  DEF_COLOR = val;
  LED_COLOR = val;
}

void SetStandardColor(const String &cmd)
{
  int lengthCmd = cmd.length();
  String value = cmd.substring(9, lengthCmd);
  int index = atoi(value.c_str());
  if (index > 6)
  {
    index = 6;
  }

  LED_COLOR = standardColors[index - 1];
  DEF_COLOR = LED_COLOR;
}

void ColorSelect()
{ // Функция выбора цвета часов от настроек
  // if (night_flag){
  // LED_COLOR = COLOR_NIGHT;}
  // else{
  // LED_COLOR = COLOR_NOONE;}

  LED_COLOR = DEF_COLOR;
}
void ShowTemper()
{ // Функция отображения температуры
  //  mode = 1;
  //  float t = dht.readTemperature();
  //  if (isnan(t))
  //  {
  //    return;
  //  }
  //  else{
  //    float x,y;
  //    x = modff(t, &y); // получаем целую и дробную части
  //    byte a = (byte)y / 10;
  //    byte b = (byte)y % 10;
  //    byte c = ((byte)x * 10) / 10;
  //    LED_COLOR = 0x0000FF;
  //    DigitOut(a, b, c, 10, 1, LED_COLOR);
  //  }
}
void ShowDate()
{ // Функция отображения даты
  mode = 2;
  byte a = tm.date / 10;
  byte x = tm.date % 10;
  byte y = tm.month / 10;
  byte z = tm.month % 10;
  LED_COLOR = 0x00CCFF;
  DigitOut(a, x, y, z, 1, LED_COLOR);
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Работа с памятью +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void (*resetFunc)(void) = 0; // Функция перезагрузкки МК.
void ReadEEPROM()
{ // Чтение
  for (byte i = 1; i <= 8; i++)
    setups[i] = EEPROM.readByte(i);
  read_eeprom = false;
}

void UpdateEEPROM()
{ // Запись
  for (byte i = 1; i <= 8; i++)
    EEPROM.updateByte(i, setups[i]);
}

void EepromTick()
{ // Проверка
  if (eeprom_flag && mode_flag)
  {
    for (byte i = 1; i <= 8; i++)
    {
      if (setups[i] != EEPROM.readByte(i))
      {
        UpdateEEPROM();
        break;
      }
    }
    eeprom_flag = false;
  }
}

void ResetClock()
{ // Сброс на настройки по умолчанию
  byte def_setup[9] = {0, TIME_TEMP, TIME_DATE, SET_COLOR, SET_LIGHT, SET_TEMPS, SET_SHOWS, TIME_TIME, SET_PRESS};
  // setups[i]  i  =  _      1          2         3          4          5          6           7         8
  for (byte i = 1; i <= 8; i++)
    EEPROM.updateByte(i, def_setup[i]);
  delay(100);
  resetFunc();
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void setup()
{
  pinMode(BT_TX_PIN, INPUT);                                     // RX
  pinMode(BT_RX_PIN, OUTPUT);                                    // TX
  LEDS.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS); // Установить тип светодиодной ленты
  figure[0] = 0;
  figure[1] = 7 * SEGMENT_LEDS;
  figure[2] = (14 * SEGMENT_LEDS) + 2;
  figure[3] = (21 * SEGMENT_LEDS) + 2;
  t = millis();
  opdating_time = millis();

  Serial.begin(115200);
  mySerial.begin(115200);
  mySerial.setTimeout(100);
  // dht.begin();

  RTC.setTime(COMPILE_TIME);

  LED_COLOR = standardColors[3];
}

// ОСНОВНОЙ ЦИКЛ
void loop()
{
  // CheckConnectionBt();
  // чтение данных с bluetooth
  if (bt.available())
  { // если данные получены
    // Serial.println(bt.buf); // выводим их (как char*)
    ParseCommand(bt.buf);
  };
  //-----------------------------------------------
  if (read_eeprom)
    ReadEEPROM();
  BrightnessCheck(); // Проверка яркости
  DateTime Now;      // текущее время
  if (millis() - opdating_time > 300)
  {
    tm = RTC.getTime(); // чтение текущего времени
    opdating_time = millis();
  }

  if (mode == 0)
  { // установка времени стандартный режим
    Now.hour = tm.hour;
    Now.minute = tm.minute;
    Now.second = tm.second;
    Now.date = tm.date;
    Now.month = tm.month;
    Now.year = tm.year;
    if (Now.hour <= TIME_NIGHT_END || Now.hour >= TIME_NIGHT_BEGIN)
      night_flag = true;
    else
      night_flag = false;
  }

  // ------------------------------------------   switch   -----------------------------------
  switch (mode)
  {
  case 0:
  { // Нормальный режим отображение времени
    if (Now.minute % 10 == 0 && Now.second == 0)
    { // (кратно 5 мин)
      ColorSelect();
      Animation();
      // if (setups[6] != 0){
      // Animation();
      //}
    }
    if (Now.second % 2 == 0)
      points = 2;
    else
      points = 0;
    byte a = Now.hour / 10;
    byte x = Now.hour % 10;
    byte y = Now.minute / 10;
    byte z = Now.minute % 10;
#ifdef ZERO_WATCH
    if (Now.hour < 10)
    {
      DigitOut(18, x, y, z, points, LED_COLOR);
    }
    else
#endif
      DigitOut(a, x, y, z, points, LED_COLOR);
    mode_flag = true;
    // t= millis();
    // tp = 0;
    break;
  }
  case 1:
  { // Режим температуры
    // ShowTemper();
    // if(millis() - t > 5000) {
    // Animation();
    // ColorSelect();
    // mode = 0;
    //}
    break;
  }
  case 2:
  { // Режим даты
    ShowDate();
    if (millis() - t > 5000)
    {
      Animation();
      ColorSelect();
      mode = 0;
    }
    break;
  }
  case 55:
  { // Режим версии проошивки
    LED_COLOR = 0xFF0000;
    DigitOut(15, 1, 0, 15, 1, LED_COLOR);
    if (millis() - t > 5000)
    {
      mode = 0;
      ColorSelect();
    }
    break;
  }
  }

  EepromTick();   // Проверка на сохранение в пямать
  FastLED.show(); // Вывод на диоды
  //----------------------------------------------------------
}
