/*
  Shinichiro Nakamuraさんの音楽再生ライブラリ”A tiny MML parser”を使ってmicro:bitでジングルベルを鳴らしてみました。
  https://www.cubeatsystems.com/tinymml/video.html

  Tone命令のdev_notone、dev_toneはたまきちさんのホームページ「micro:bitをArduino環境で使う （6） PPIを使った単音出力」を参考にさせていただきました。
  http://nuneno.cocolog-nifty.com/blog/2018/01/microbitardui-1.html?fbclid=IwAR0K0BTBlwvyM2uq5PUNdsPe6l7_BYaxBzfrhtYKJ2NUtdc3fO78XGcmieU

  ジングルベルの曲データはこちらを参考にさせていただきました。
  ※A tiny MML parserは１オクターブ上げる、１オクターブ下げるが参考にしたMMLと反対でしたので、修正しています。
  http://byakuya.sp.land.to/MML/list?page=4

  Copyright (c) 2018 Kei Takagi
  Released under the MIT license
  http://opensource.org/licenses/mit-license.php
*/

#include <mml.h>
#include "note.h"

// タイマー基本周期
#define TIMERFREQ   1000000L
// 利用タイマー資源
#define TONE_TIMER  NRF_TIMER0

const uint16_t notefreq[] PROGMEM = {
  32,    34,    36,    38,    41,    43,    46,    48,    51,    55,    58,    61,
  65,    69,    73,    77,    82,    87,    92,    97,   103,   110,   116,   123,
  130,   138,   146,   155,   164,   174,   184,   195,   207,   220,   233,   246,
  261,   277,   293,   311,   329,   349,   369,   391,   415,   440,   466,   493,
  523,   554,   587,   622,   659,   698,   739,   783,   830,   880,   932,   987,
  1046,  1108,  1174,  1244,  1318,  1396,  1479,  1567,  1661,  1760,  1864,  1975,
  2093,  2217,  2349,  2489,  2637,  2793,  2959,  3135,  3322,  3520,  3729,  3951,
  4186,  4434,  4698,  4978,  5274,  5587,  5919,  6271,  6644,  7040,  7458,  7902,
  8372,  8869,  9397,  9956, 10548, 11175, 11839, 12543, 13289, 14080, 14917, 15804,
  16744, 17739, 18794, 19912, 21096, 22350, 23679, 25087, 26579, 28160, 29834, 31608,
  33488, 35479, 37589, 39824, 42192, 44701, 47359, 50175,
};

// ジングルベル
const char song_text[] PROGMEM = "A8A8AA8A8AA8 >C8<F8.G16A4R4 B-8B-8B-8.B-16B-8A8A8A16A16A8G8G8F8G8>C<R8 A8A8AA8A8AA8 >C8<F8.G16A4R4 B-8B-8B-8.B-16B-8A8A8A8>C8C8<B-8G8F4R";

NOTE note;
MML mml;
MML_OPTION mml_opt;

void setup()
{
  /*
     Initialize the MML module.
  */
  mml_init(&mml, mml_callback, 0);
  MML_OPTION_INITIALIZER_DEFAULT(&mml_opt);

  /*
     Initialize the note module.
  */
  int tempo_default = 120;
  note_init(&note, tempo_default, mml_opt.bticks);
}

void loop()
{
  /*
     Setup the MML module.
  */
  mml_setup(&mml, &mml_opt, (char *)song_text);

  /*
     Fetch the MML text command.
  */
  while (mml_fetch(&mml) == MML_RESULT_OK) {
  }
  delay(1000);
}

static void mml_callback(MML_INFO *p, void *extobj)
{
  switch (p->type) {
    case MML_TYPE_TEMPO:
      {
        MML_ARGS_TEMPO *args = &(p->args.tempo);
        note_init(&note, args->value, mml_opt.bticks);
      }
      break;
    case MML_TYPE_NOTE:
      {
        MML_ARGS_NOTE *args = &(p->args.note);
        note_tone(&note, args->number, args->ticks);
      }
      break;
    case MML_TYPE_REST:
      {
        MML_ARGS_REST *args = &(p->args.rest);
        note_rest(&note, args->ticks);
      }
      break;
  }
}

static uint32_t get_note_length_ms(NOTE *p, uint32_t note_ticks)
{
  return (60000) * note_ticks / p->bpm / p->bticks;
}

void note_init(NOTE *p, int bpm, int bticks)
{
  p->bpm = bpm;
  p->bticks = bticks;
}

void note_tone(NOTE *p, int number, int ticks)
{
  uint16_t freq = pgm_read_word(&(notefreq[number]));
  uint32_t ms = get_note_length_ms(p, ticks);
  do {
    dev_tone(0, freq);
    delay(ms);
  } while (0);
}

void note_rest(NOTE *p, int ticks)
{
  uint32_t ms = get_note_length_ms(p, ticks);
  do {
    dev_notone();
    delay(ms);
  } while (0);
}

//
// 音の停止
// 引数
//
void dev_notone() {
  TONE_TIMER->TASKS_STOP = 1;  // タイマストップ
}


//
// 音出し
// 引数
//  pin     : PWM出力ピン
//  freq    : 出力周波数 (Hz) 15～ 50000
//
void dev_tone(uint8_t pin, uint16_t freq) {
  uint32_t ulPin;
  uint32_t f = TIMERFREQ / freq;

  // GPIOTEの設定：LEDピン・トグルタスクを定義する
  ulPin = g_ADigitalPinMap[pin];   // TonePinの実ピン番号の取得
  NRF_GPIOTE->CONFIG[0] =          // チャネル0に機能設定
    (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |            // タスクモード
    (ulPin << GPIOTE_CONFIG_PSEL_Pos) |                              // ピン番号設定
    (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |  // 動作指定：トグル
    (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);        // ピン出力初期値
  NRF_GPIOTE->POWER = 1;                                             // GPIOTE有効

  //タイマ設定
  TONE_TIMER->TASKS_STOP = 1;                          // タイマストップ
  TONE_TIMER->TASKS_CLEAR = 1;                         // カウンタクリア
  TONE_TIMER->MODE = TIMER_MODE_MODE_Timer;            // モード設定：タイマモード
  TONE_TIMER->PRESCALER   = 4;                         // プリスケーラ設定：16分周(1MHz)
  TONE_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit;   // カウンタ長設定：16ビット長指定
  TONE_TIMER->CC[0] = f / 2;                           // コンパレータ0の設定(出力周波数設定)

  TONE_TIMER->SHORTS =                                 // ショートカット設定：クリアタスク指定
    (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);

  // PPIの設定(チャネル0を利用)
  //   TIMER0 コンパレータ0一致イベント と GPIOTE(ch0)LEDピン・トグルタスク を結び付ける
  NRF_PPI->CH[0].TEP  = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];       // PPI.ch0 にLEDピン・トグルタスク設定
  NRF_PPI->CH[0].EEP  = (uint32_t)&TONE_TIMER->EVENTS_COMPARE[0];  // PPI ch0 にコンパレータ0一致イベント設定
  NRF_PPI->CHENSET   |= PPI_CHENSET_CH0_Enabled;                   // PPI ch0 有効

  TONE_TIMER->TASKS_START = 1;   // タイマスタート
}

