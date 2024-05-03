// 定義音符和時值
#define C4 262
#define D4 294
#define Eb4 311
#define F4 349
#define G4 392
#define Ab4 415
#define A4 440
#define Bb4 466
#define C5 523
#define D5 587
#define Eb5 622
#define F5 698
#define G5 784
#define A5 880
#define Bb5 932
#define C6 1047
#define WHOLE 2000
#define HALF 1000
#define QUARTER 500
#define EIGHTH 250
#define SIXTEENTH 125

// 定義蜂鳴器的引腳和頻道
#define buzzer_pin 4

// 定義音樂的節拍
#define TEMPO 500

class sound
{
  public:
  bool muted=0;//是否靜音
  byte BGM_index=31;//指向這在播放的音高
  byte BGM_length=32;//音高長度
  int BGM[32]=
  {
    F4, Ab4, C5, F5, G5, Eb5, C5, Ab4, Bb4, D5, F5, Bb5, C6, A5, F5, D5, F4, Ab4, C5, F5, Bb5, F5, C5, Ab4, F4, C5, F5, C6
  };
  int BGM_duration[32]= 
  {
    QUARTER, QUARTER, QUARTER, QUARTER, EIGHTH, EIGHTH, QUARTER, QUARTER, QUARTER, QUARTER, QUARTER, QUARTER, EIGHTH, EIGHTH, QUARTER, QUARTER, QUARTER, QUARTER, QUARTER, QUARTER, EIGHTH, EIGHTH, QUARTER, QUARTER, QUARTER, QUARTER, HALF, HALF
  };
};
