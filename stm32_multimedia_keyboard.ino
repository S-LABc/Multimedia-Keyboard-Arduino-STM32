/**
 * STM32 Multimedia Keyboard
 * 
 * ########## В А Ж Н О ##########
 * Скетч для клавиатуры управления мультимедийным контентом
 * Реализует то же, что сочетание [Fn] + [какая-то кнопка]
 * Скетч адаптирован для работы с двумя ядрами STM32
 * Для переключения закоментировать/раскоментировать #define __ARDUINO_CORE_STM32_FPISTM
 * Устанавливать библиотеку MediaKeyboard.h нужно только для ядра от fpistm
 * 
 * Форум STM32Duino https://www.stm32duino.com/
 * Статья про энкодер. Обработчик isWheel оттуда https://alexgyver.ru/encoder/
 * Статья от автора библиотеки MediaKeyboard.h https://www.onetransistor.eu/2020/03/usb-multimedia-keys-on-arduino-stm32.html
 * 
 ** Ядро для поддержки плат STM32 от rogerclarkmelbourne https://github.com/rogerclarkmelbourne/Arduino_STM32
 ** Для его работы нужно:
 *** Установить "Arduino SAM boards (Cortex-M3)" через менеджер плат
 *** Скопировать содержимое скаченного архива в папку "hardware" по пути C:\Users\[Имя пользователя]\Documents\Arduino
 *** Если папки "hardware" нет, ее нужно создать самостоятельно
 * 
 * Настройки платы: (которые использовал я)
 * Board              Generic STM32F103C Series
 * Variant            STM32F103CB(20k RAM. 128k Flash)
 * Uploud Method      STLink
 * CPU Speed          48Mhz (Slow - With USB)
 * Optimize           Smallest (Default)
 * 
 ** Ядро для поддержки плат STM32 от fpistm https://github.com/stm32duino/Arduino_Core_STM32
 ** Для использования перейти: Файл->Настройки->Ссылки менеджера плат, добавить https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json
 ** Затем, перейти: Инструменты->Плата->Менеджер плат, в поиске написать "STM32", установить STM32 Cores by STMicroelectronics
 ** Установить библиотеку для медиа клавиатуры https://github.com/onetransistor/MediaKeyboard
 * 
 * Настройки платы: (которые использовал я)
 * Board              Generic STM32F1 Series
 * Board Part Number  Generic F103C8
 * U(S)ART Support    Disabled (No Seral Support)
 * USB Support        HID (Keyboard and Mouse)
 * USB Speed          Low/Full Speed
 * Optimize           Smallest (-Os, Default)
 * C Runtime library  Newlib Nano (Default)
 * Uploud Method      SMT32CubeProgrammer (SWD)
 * 
 ** Связь со мной:
 ** YouTube https://www.youtube.com/channel/UCbkE52YKRphgkvQtdwzQbZQ
 ** Telegram https://www.t.me/slabyt
 ** Instagram https://www.instagram.com/romasklr
 ** VK https://vk.com/id395646965
 ** Facebook https://www.facebook.com/romasklyar94
 ** Twitter https://twitter.com/_SklyarRoman
 ** GitHub https://github.com/S-LABc
 * 
 ** COMPILED IN ARDUINO v1.8.13
 ** Copyright (C) 2021. v1.0 / Скляр Роман S-LAB
*/

/* Закоментировать, если ядро от rogerclarkmelbourne
   Раскоментировать, если ядро от fpistm */
//#define __ARDUINO_CORE_STM32_FPISTM

#ifndef __ARDUINO_CORE_STM32_FPISTM
#include <USBComposite.h>
#else
#include <MediaKeyboard.h>
#endif
/* Выводы энкодера и его кнопки */
#define PHASE_A         PA1
#define PHASE_B         PA0
#define BTN_ENC         PC15
/* Выводы отдельных нопкок */
#define BTN_PREV        PA7
#define BTN_PLAY_PAUSE  PA6
#define BTN_NEXT        PA5
/* Если ядро от rogerclarkmelbourne, добавляем макросы кодов кнопок */
#ifndef __ARDUINO_CORE_STM32_FPISTM
/* Коды кнопок медиаконтента */
#define VOLUME_UP        0xE9
#define VOLUME_DOWN      0xEA
#define VOLUME_MUTE      0xE2
#define MEDIA_PLAY_PAUSE 0xCD
#define MEDIA_NEXT       0xB5
#define MEDIA_PREV       0xB6
/* Настройка USB как мультимедийной клавиатуры */
USBHID HID;
const uint8_t reportDescription[] = {
   HID_CONSUMER_REPORT_DESCRIPTOR()
};
HIDConsumer MediaKeyboard(HID);
#endif

/* Переменные для функций действий кнопок и энкодера */
boolean btn_enc_flag = false, btn_prev_flag = false, btn_play_pause_flag = false, btn_next_flag = false;
/* Переменные для обработчика прерываний */
volatile boolean encoder_reset = false, encoder_flag = false, turnLeft = false, turnRight = false;
volatile byte encoder_state = 0x00, previous_encoder_state = 0x00;

void setup() {
  /* Настройка выводов на вход */
  pinMode(PHASE_A, INPUT_PULLUP);
  pinMode(PHASE_B, INPUT_PULLUP);
  pinMode(BTN_ENC, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_PLAY_PAUSE, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  /* Настройка прерываний энкодера */
  attachInterrupt(digitalPinToInterrupt(PHASE_A), isWheel, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PHASE_B), isWheel, CHANGE);
#ifndef __ARDUINO_CORE_STM32_FPISTM
  //enableDebugPorts(); // enableDebugPorts() Раскоментируейте, если PB3 или PB4 или PA15 используются в вашем проекте
  USBComposite.setManufacturerString("Sklyar Roman S-LAB");
  USBComposite.setProductString("Media Keyboard");
  USBComposite.setSerialString("v1.0");
  HID.begin(reportDescription, sizeof(reportDescription));
  while (!USBComposite); // Ждем успешного соединения
#else
  MediaKeyboard.begin();
  delay(1000); // Для подгрузки драйверов
#endif
}

/* Обработка событий от энкодера и кнопок */
void loop() {
  wheelEvents();
  btnEvents();
}

/* Обработчик прерываний энкодера */
void isWheel() {
  encoder_state = digitalRead(PHASE_B) | digitalRead(PHASE_A) << 1;
  if (encoder_state == 0x00) { // Сброс
    encoder_reset = true;
  }
  if (encoder_reset && encoder_state == 0x03) {
    if (previous_encoder_state == 0x02) { // Лево
      turnLeft = true;
    }
    if (previous_encoder_state == 0x01) { // Право
      turnRight = true;
    }
    encoder_reset = false;
    encoder_flag = true;
  }
  previous_encoder_state = encoder_state;
}

/* Действия при повороте энкодера */
void wheelEvents() {
  if (encoder_flag) {
    if (turnLeft) {
      MediaKeyboard.press(VOLUME_DOWN);
      turnLeft = false;
    }
    else if (turnRight) {
      MediaKeyboard.press(VOLUME_UP);
      turnRight = false;
    }
    MediaKeyboard.release();
    encoder_flag = false;
  }
}

/* Действия по нажатию на кнопки */
void btnEvents() {
  if (!btn_enc_flag && digitalRead(BTN_ENC) == LOW) {
    MediaKeyboard.press(VOLUME_MUTE);
    MediaKeyboard.release();
    btn_enc_flag = true;
  }
  if (btn_enc_flag && digitalRead(BTN_ENC) == HIGH) {
    btn_enc_flag = false;
  }
  if (!btn_prev_flag && digitalRead(BTN_PREV) == LOW) {
    MediaKeyboard.press(MEDIA_PREV);
    MediaKeyboard.release();
    btn_prev_flag = true;
  }
  if (btn_prev_flag && digitalRead(BTN_PREV) == HIGH) {
    btn_prev_flag = false;
  }
  if (!btn_play_pause_flag && digitalRead(BTN_PLAY_PAUSE) == LOW) {
    MediaKeyboard.press(MEDIA_PLAY_PAUSE);
    MediaKeyboard.release();
    btn_play_pause_flag = true;
  }
  if (btn_play_pause_flag && digitalRead(BTN_PLAY_PAUSE) == HIGH) {
    btn_play_pause_flag = false;
  }
  if (!btn_next_flag && digitalRead(BTN_NEXT) == LOW) {
    MediaKeyboard.press(MEDIA_NEXT);
    MediaKeyboard.release();
    btn_next_flag = true;
  }
  if (btn_next_flag && digitalRead(BTN_NEXT) == HIGH) {
    btn_next_flag = false;
  }
}
