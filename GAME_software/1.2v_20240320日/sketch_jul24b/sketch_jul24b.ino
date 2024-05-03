#include  <Arduino.h>
#include  "game0.h"
#include  "game1.h"
#include  "game2.h"
#include  "timepass.h"
#include  <Adafruit_NeoPixel.h>
#include  <TM1637Display.h>
#include  "menu.h"
#include  "sound.h"

////pin腳位置////
const byte ws2812_data=22;//ws2812資料輸出
const byte seg7[2][2]={{18,19},{16,17}};//[][0:data 1:clk]
const byte keyboard_pin[2][5]={{39,36,34,35,32},{33,25,26,27,14}};//按鍵pin腳0:y 1:x 2:確認(A) 3:暫停(B) 4:確認搖桿連接
const byte randomSeedpin=23;//隨機數種子讀取pin腳;


////搖桿變數////
volatile bool keyboard[4]={0,0,0,0};//紀錄按鍵是否被按下(AB鍵)
volatile byte Joystick[4]={0,0,0,0};//紀錄搖桿是否被按下(yx軸的方向)
int Joystick_log[2][2][6]={0};//[選擇搖桿][0:y 1:x][讀取紀錄]
const int voltage_level[2]={3000,1000};//電位判斷0:高 1:低
byte player_connect[4]={0,0,0,0};//0:哪知搖桿先連接(0:無搖桿連接 1:搖桿0先連 2:搖桿1先連) 1:搖桿0是否被連接 2:搖桿1是否被連接 3:玩家數
int debouncetime=200;//200毫秒
int Joystick_wait=300;//300毫秒

////遊戲設定變數////
byte begin=0;//是否已選擇遊戲
byte win0=0,win1=0;//輸贏(0:未贏 1:贏 2:暫停選單選擇中回主選單)
bool menu_choose=0;//暫停選單的選項0:繼續 1:主選單
bool end_skip=0;//遊戲結束畫面是否跳過
int fraction[2][4]={0};//分數[玩家][0:現在遊戲分數 1:遊戲0最高分 2:遊戲1最高分 3:遊戲2最高分]
game0 Tetris0;//創建玩家0俄羅斯方塊
game0 Tetris1;//創建玩家1俄羅斯方塊
game1 Tic_tac_toe;//創建圈圈叉叉
game2 snake0;//創建玩家0貪食蛇
game2 snake1;//創建玩家0貪食蛇

////計時器變數////
timep time_game/*預設八個位置*/;//0:計時遊玩時間 1:玩家0落下時間計算(俄羅斯方塊) 2:玩家1落下時間計算(俄羅斯方塊) 3:玩家0移動計時(貪食蛇) 3:玩家1移動計時(貪食蛇) 4:遊戲結束計時閃爍動動畫時間
timep time_system;//0:聲音頻率計時用 1:跑馬燈計時
timep time_contral;//0:0搖桿y停止時間 1:0搖桿x停止時間 2:0搖桿a停止時間 3:0搖桿b停止時間 4:1搖桿y停止時間 5:1搖桿x停止時間 6:1搖桿a停止時間 7:1搖桿b停止時間 

////螢幕變數////
Adafruit_NeoPixel ws2812(400, ws2812_data, NEO_GRB + NEO_KHZ800);
byte screen_out[3][3][20][20]={0};//[0:輸出畫面 1:備分 2:備分][0:R 1:G 2:B][y][x]畫面輸出
bool screen_change=0;//是否要改變畫面
TM1637Display tm1637_0(seg7[0][1],seg7[0][0]);  //    七 要顯示自訂:tm1637.display(ListDisp);顯示數字tm1637.displayNum(ListDisp);
int8_t ListDisp0[4]={0};                        //    段 顯示字串tm1637.displayStr("");顯示:分號tm1637.point(0、1);
TM1637Display tm1637_1(seg7[1][1],seg7[1][0]);  //    顯 不顯示數字display.setBrightness(7, true);
int8_t ListDisp1[4]={0};                        //    示
int Marquee_word_count=0;//跑馬燈字數
bool Marquee_word[12][24]={0};//跑馬燈字
bool Marquee_out[12][20]={0};//跑馬燈輸出
int Marquee_round=0;//跑馬燈輪迴
menu picture;//圖片資料庫
int screen_brightness=22;//螢幕亮度

////聲音////
sound music;

////多核心////
TaskHandle_t Task;
void task_core0();

/////////////////////////////函式///////////////////////////////////////////

////搖桿xy////    
int noise_removal(int);//消除雜訊(中位值平均濾波法)  
void input_zero(void);//按鈕和蘑菇頭設為未動作

////遊戲暫停////
void game_stop_menu();//遊戲中選單

////畫面管理////
void Marquee_set(bool ,bool*);//設定跑馬燈文字
void Marquee(byte,byte);//跑馬燈
void cutscenes(bool);//過場動畫
void paste(byte/*y*/,byte/*x*/,bool* /*輸入要貼的圖片*/,byte/*哪個地圖*/,byte/*陣列最大值y*/,byte/*陣列最大值x*/,int/*顏色*/,bool/*上下翻轉*/);//將圖片貼上
void paste(byte/*y*/,byte/*x*/,int* /*輸入要貼的圖片*/,byte/*哪個地圖*/,byte/*陣列最大值y*/,byte/*陣列最大值x*/,bool/*上下翻轉*/);//將圖片貼上(彩色)
void erase(byte/*y*/,byte/*x*/,bool/*0:單格 1:全部*/);//清除畫面


/////////////////////////////////////////////////////////////////////////////
void setup() 
{
  Serial.begin(115200);
  byte i=0,f=0,g=0,h=0;//迴圈用
  //pin腳設定
  pinMode(randomSeedpin,INPUT);
  for(i=0;i<2;i++)
  {
    for(f=0;f<5;f++)
    {
      pinMode(keyboard_pin[i][f],INPUT);
    }
  }
  //核心0設定
   xTaskCreatePinnedToCore(
    task_core0,/*任務實際對應的Function*/
    "Task",    /*任務名稱*/
    10000,     /*堆疊空間*/
    NULL,      /*無輸入值*/
    0,         /*優先序0*/
    &Task,     /*對應的任務變數位址*/
    0);        /*指定在核心0執行 */
   
  //隨機數種子設定
  randomSeed(analogRead(randomSeedpin));
}

void loop() 
{
  int i,f,g,h;
  //主選單
    input_zero();//輸入設為未動作
    tm1637_0.clear();// Turn off
    tm1637_1.clear();// Turn off
    cutscenes(1);//顯示過場動畫
    erase(0,0,1);//清除畫面
    paste(17,7,picture.ARROW[0],0,3,6,128,0);//貼上下箭頭
    paste(7,0,picture.G[0],0,5,5,128,0);//貼上G
    paste(7,5,picture.A[0],0,5,5,128,0);//貼上A
    paste(7,10,picture.M[0],0,5,5,128,0);//貼上M
    paste(7,15,picture.E[0],0,5,5,128,0);//貼上E
    cutscenes(0);//過場動畫消失
    screen_change=1;
    debouncetime=150;
    Joystick_wait=300;
    while(begin==0)//選單畫面
    {
      if(keyboard[0]==1||keyboard[1]==1||keyboard[2]==1||keyboard[3]==1||Joystick[0]!=0||Joystick[2]!=0)//任何搖桿有動作
      {
        if(keyboard[0]||keyboard[2])//確認鍵被按下
        {
          if((picture.menu_map[0]==1))//第一層選單進入第二層選單的當前選單設定
          {
            if((picture.menu_map[1]==1)||(picture.menu_map[1]==0))//選項play
            {
              picture.menu_map[1]=1;
            }
            else if((picture.menu_map[1]==2)||(picture.menu_map[1]==3))//選項set
            {
              picture.menu_map[1]=6;
            }
          }
          picture.menu_map[0]++;//進入下一層
        }
        else if(!(picture.menu_map[0]==1)&&(keyboard[1]||keyboard[3]))//返回鍵被按下
        {
          picture.menu_map[1]=1;
          picture.menu_map[0]--;
        }
        if((Joystick[0]==2||Joystick[2]==2))//搖桿上且當前地圖>1
        {
          picture.menu_map[1]--;          
        }
        else if((Joystick[0]==1||Joystick[2]==1))//搖桿下搖且當前地圖<3
        {
          picture.menu_map[1]++;          
        }
        switch(picture.menu_map[0])//遊戲選單地圖
        {
          case 0://沒有選單上    
            picture.menu_map[0]++;
            break;
          case 1://第一層選單
            switch(picture.menu_map[1])//遊戲選單地圖
            {
              case 0://沒有選單上    
                picture.menu_map[1]++;
                break;
              case 1://game
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                paste(17,7,picture.ARROW[0],0,3,6,128,0);//貼上下箭頭
                paste(7,0,picture.G[0],0,5,5,128,0);//貼上g
                paste(7,5,picture.A[0],0,5,5,128,0);//貼上a
                paste(7,10,picture.M[0],0,5,5,128,0);//貼上m
                paste(7,15,picture.E[0],0,5,5,128,0);//貼上e
                cutscenes(0);//過場動畫消失
                break;
              case 2://setting
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                paste(0,7,picture.ARROW[0],0,3,6,128,1);//貼上上箭頭
                paste(7,0,picture.S[0],0,5,5,128,0);//貼上g
                paste(7,5,picture.E[0],0,5,5,128,0);//貼上a
                paste(7,10,picture.T[0],0,5,5,128,0);//貼上m
                cutscenes(0);//過場動畫消失
                break;
              case 3://沒有選單下
                picture.menu_map[1]--;
                break;
            }
            break;
          case 2://第二層選單
            switch(picture.menu_map[1])//遊戲選單地圖              
            {
              case 0://沒有選單上    
                picture.menu_map[1]++;
                break;
              case 1://俄羅斯方塊
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                paste(17,7,picture.ARROW[0],0,3,6,128,0);//貼上下箭頭
                paste(4,4,picture.m1[0],0,12,12,128,0);//貼上1
                cutscenes(0);//過場動畫消失
                break;
              case 2://彈跳球
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                paste(0,7,picture.ARROW[0],0,3,6,128,1);//貼上上箭頭
                paste(17,7,picture.ARROW[0],0,3,6,128,0);//貼上下箭頭
                paste(4,4,picture.m2[0],0,12,12,128,0);//貼上2  
                cutscenes(0);//過場動畫消失                 
                break;
              case 3://貪食蛇
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                paste(0,7,picture.ARROW[0],0,3,6,128,1);//貼上上箭頭
                paste(4,4,picture.m3[0],0,12,12,128,0);//貼上3 
                cutscenes(0);//過場動畫消失
                break; 
              case 4://沒有選單下
                picture.menu_map[1]--;
                break; 
              case 5://沒有選單上    
                picture.menu_map[1]++;
                break; 
              case 6://亮度
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                paste(17,7,picture.ARROW[0],0,3,6,128,0);//貼上下箭頭
                paste(4,4,picture.brightness[0],0,12,12,128,0);//貼上亮度
                cutscenes(0);//過場動畫消失
                break;
              /*case 7://靈敏度
                erase(0,0,1);//清除畫面
                paste(0,7,picture.ARROW[0],0,3,6,128,1);//貼上上箭頭
                paste(17,7,picture.ARROW[0],0,3,6,128,0);//貼上下箭頭
                paste(4,4,picture.m2[0],0,12,12,128,0);//貼上2 
                break;*/
              case 7://聲音
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                paste(0,7,picture.ARROW[0],0,3,6,128,1);//貼上上箭頭
                paste(4,4,picture.volume[0],0,12,12,128,0);//貼上聲音
                cutscenes(0);//過場動畫消失
                break;
              case 8://沒有選單下
                picture.menu_map[1]--;
                break;    
            }
            break;  
          case 3://第三層選單(動作)
            switch(picture.menu_map[1])//遊戲選單地圖              
            {
              case 0://沒有選單上(俄羅斯方塊) 
                picture.menu_map[1]++;
              case 1://俄羅斯方塊
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                begin=1;
                break;
              case 2://彈跳球
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                begin=1;               
                break;
              case 3://貪食蛇
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                begin=1; 
                break; 
              case 4://沒有選單下(貪食蛇)
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                picture.menu_map[1]--;
                begin=1;
                break; 
              case 5://沒有選單上(亮度)
                picture.menu_map[1]++;
              case 6://亮度
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面   
                begin=2;
                break;
              /*case 7://靈敏度
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                begin=2;
                break;*/
              case 7://聲音大小
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                begin=2;
                break;
              case 8://沒有選單下(聲音大小)
                cutscenes(1);//顯示過場動畫
                erase(0,0,1);//清除畫面
                picture.menu_map[1]--;
                begin=2;
                break;  
            }
            break;  
        }
        screen_change=1;
        input_zero();//輸入設為未動作
      }
    }
  //遊戲開始
  if(begin==1)
  {
    tm1637_0.setBrightness(7,true);//7段不顯示
    tm1637_1.setBrightness(7,true);//7段不顯示
    ////////俄羅斯方塊////////
    if(picture.menu_map[1]==1)
    {
      player_connect[3]=player_connect[2]+player_connect[1];//遊戲人數
      if(player_connect[3]==1)//單人模式
      {
        tm1637_0.showNumberDec(0,0);//顯示分數
        Tetris0.contral=player_connect[0];//設定搖桿
        Tetris0.contral*=2;
        paste(0,4,Tetris0.wall[0],0,20,1,256*256*85+256*85+85,0);//貼上牆壁圖片
        paste(0,15,Tetris0.wall[0],0,20,1,256*256*85+256*85+85,0);//貼上牆壁圖片
      }
      cutscenes(0);//過場動畫消失
      if(player_connect[3]==2)//雙人模式
      {
        tm1637_0.showNumberDec(0,0);//顯示分數
        tm1637_1.showNumberDec(0,0);//顯示分數
        Tetris0.contral=2;//設定搖桿
      }
      Joystick_wait=175;//搖桿除彈跳設為175毫秒
      debouncetime=225;//搖桿除彈跳設為225毫秒
      time_game.time_pass(1,0);//遊戲時間計時
      while(win0==0&&win1==0)//遊戲開始
      {
        //////生成新的方塊//////
        if(Tetris0.drop==0)
        {
          Tetris0.initialize();//玩家0方塊落下初始化
          Tetris0.move();//模擬移動方塊位置
          if(Tetris0.state[2]==1&&Tetris0.state[0]==2)//碰到方塊且超出上方邊界
          {
            win1=1;//玩家1贏
            break;//遊戲結束
          }
          Tetris0.log();//紀錄
          paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,Tetris0.color[Tetris0.block_choose],0);//貼上紀錄位置方塊圖片
          screen_change=1;//畫面改變
          time_game.time_pass(1,1);//玩家0落下時間開始計時
        }
        if(player_connect[3]==2)//(雙人模式)
        {
          if(Tetris1.drop==0)
          {
            Tetris1.initialize();//玩家1方塊落下初始化       
            Tetris1.move();//模擬移動方塊位置
            if(Tetris1.state[2]==1&&Tetris1.state[0]==2)//碰到方塊且超出上方邊界  
            {
              win0=1;//玩家0贏
              break;//遊戲結束
            }
            Tetris1.log();//紀錄
            paste(0,10,Tetris1.MAP[0][0],2,20,10,Tetris1.color[Tetris1.block_choose],0);//貼上紀錄位置方塊圖片
            screen_change=1;//畫面改變
            time_game.time_pass(1,2);//玩家1落下時間開始計時
          }
        }
        //////落下循環//////
        while(Tetris0.drop&&(Tetris1.drop||player_connect[3]==1)&&win0==0&&win1==0)
        {
          if(time_game.time_pass(0,1)>Tetris0.drop_time)//玩家0方塊落下時間到
          {
            Tetris0.drop_time=Tetris0.drop_time_r;
            Tetris0.location[0]++;//方塊位置下移
            Tetris0.move();//模擬移動方塊位置
            if(Tetris0.state[2]==1||Tetris0.state[0]==1)//碰到方塊或超出下方邊界  
            {
              Tetris0.drop=0;//方塊落下停止
              Tetris0.freeze();//方塊固定
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],0,20,10,0,0);//清除固定位置方塊圖片
              Tetris0.Combo=Tetris0.erase();//消除行
              fraction[0][0]+=(Tetris0.Combo*0.1+0.9)*Tetris0.Combo*10;//加分
              if(player_connect[3]==1)//單人模式落下速度
              {
                if(Tetris0.drop_time_r>150)//下降速度高於150毫秒
                {
                  Tetris0.drop_time_r-=Tetris0.Combo*20;//加快下降速度(每列減少20毫秒)
                }
                else if(Tetris0.drop_time_r<150)//下降速度不低於150毫秒
                {
                  Tetris0.drop_time_r=150;
                }
                Tetris0.drop_time=Tetris0.drop_time_r;
              }
              else if(player_connect[3]==2)//雙人模式落下速度
              {
                if(Tetris1.drop_time_r>150)//下降速度高於150毫秒
                {
                  Tetris1.drop_time_r-=(Tetris0.Combo*0.1+0.9)*Tetris0.Combo*20;//加快下降速度(每獲得1分減少對手2毫秒)
                }
                else if(Tetris1.drop_time_r<150)//下降速度不低於150毫秒
                {
                  Tetris1.drop_time_r=150;
                }
                Tetris1.drop_time=Tetris1.drop_time_r;
              }
              tm1637_0.showNumberDec(fraction[0][0],0);//顯示分數
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP_color[0],0,20,10,0);//貼上固定位置方塊圖片(彩色)
              break;//回到生成新方塊
            }
            else
            {
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,0,0);//清除上次紀錄位置方塊圖片
              Tetris0.log();//紀錄
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,Tetris0.color[Tetris0.block_choose],0);//貼上紀錄位置方塊圖片
              screen_change=1;//畫面改變
            }
            time_game.time_pass(1,1);//方塊落下重新計時                       
          }
          if((time_game.time_pass(0,2)>Tetris1.drop_time)&&player_connect[3]==2)//玩家1方塊落下時間到且為雙人模式
          {     
            Tetris1.drop_time=Tetris1.drop_time_r;
            Tetris1.location[0]++;//方塊位置下移
            Tetris1.move();//模擬移動方塊位置
            if(Tetris1.state[2]==1||Tetris1.state[0]==1)//碰到方塊或超出下方邊界  
            {
              Tetris1.drop=0;//方塊落下停止
              Tetris1.freeze();//方塊固定
              paste(0,10,Tetris1.MAP[0][0],0,20,10,0,0);//清除固定位置方塊圖片
              Tetris1.Combo=Tetris1.erase();//消除行
              fraction[1][0]+=(Tetris1.Combo*0.1+0.9)*Tetris1.Combo*10;//加分
              Tetris1.drop_time_r-=Tetris1.Combo*20;//加快下降速度(每列減少20毫秒)
              if(Tetris0.drop_time_r>150)//下降速度高於150毫秒
              {
                Tetris0.drop_time_r-=(Tetris1.Combo*0.1+0.9)*Tetris1.Combo*20;//加快下降速度(每獲得1分減少對手2毫秒)
              }
              else if(Tetris0.drop_time_r<150)//下降速度不低於150毫秒
              {
                Tetris0.drop_time_r=150;
              }
              Tetris0.drop_time=Tetris0.drop_time_r;
              tm1637_1.showNumberDec(fraction[1][0],0);//顯示分數
              paste(0,10,Tetris1.MAP_color[0],0,20,10,0);//貼上固定位置方塊圖片(彩色)
              break;//回到生成新方塊
            }
            else
            {
              paste(0,10,Tetris1.MAP[0][0],2,20,10,0,0);//清除上次紀錄位置方塊圖片
              Tetris1.log();//紀錄
              paste(0,10,Tetris1.MAP[0][0],2,20,10,Tetris1.color[Tetris1.block_choose],0);//貼上紀錄位置方塊圖片
              screen_change=1;//畫面改變
            }         
            time_game.time_pass(1,2);//方塊落下重新計時                       
          }
          if(keyboard[Tetris0.contral-2])//0玩家A鍵被按下
          {
            keyboard[Tetris0.contral-2]=0;
            Tetris0.direction++;//旋轉
            if(Tetris0.direction==4)
            {
              Tetris0.direction=0;
            }
            Tetris0.move();//旋轉
            if(Tetris0.state[0]==1||Tetris0.state[1]==1||Tetris0.state[1]==2||Tetris0.state[2]==1)//不合規
            {
              Tetris0.direction--;//回復旋轉
              if(Tetris0.direction==-1)
              {
                Tetris0.direction=3;
              }
            }
            else
            {
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,0,0);//清除紀錄位置方塊圖片
              Tetris0.log();//紀錄
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,Tetris0.color[Tetris0.block_choose],0);//貼上紀錄位置方塊圖片
              screen_change=1;//畫面改變
            }
          }
          if(keyboard[Tetris0.contral-1])//0玩家B鍵被按下
          {
            keyboard[Tetris0.contral-1]=0;
            for(i=0;i<3;i++)//計時暫停
            {
              time_game.time_pass(2,i);
            }
            game_stop_menu();//暫停選單
            for(i=0;i<3;i++)//計時繼續
            {
              time_game.time_pass(3,i);
            }
          }
          if(Joystick[Tetris0.contral-1]==1)//0玩家向右
          {
            Joystick[Tetris0.contral-1]=0;
            Tetris0.location[1]++;//方塊位置右移
            Tetris0.move();//模擬移動方塊位置
            if(Tetris0.state[2]==1||Tetris0.state[1]==1)//碰到方塊或超出右方邊界  
            {
              Tetris0.location[1]--;//方塊位置左移
              Tetris0.move();//模擬移動方塊位置
            }
            else
            {
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,0,0);//清除上次紀錄位置方塊圖片
              Tetris0.log();//紀錄
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,Tetris0.color[Tetris0.block_choose],0);//貼上紀錄位置方塊圖片
              screen_change=1;//畫面改變
            } 
          }
          else if(Joystick[Tetris0.contral-1]==2)//0玩家向左
          {
            Joystick[Tetris0.contral-1]=0;
            Tetris0.location[1]--;//方塊位置左移
            Tetris0.move();//模擬移動方塊位置
            if(Tetris0.state[2]==1||Tetris0.state[1]==2)//碰到方塊或超出左方邊界  
            {
              Tetris0.location[1]++;//方塊位置右移
              Tetris0.move();//模擬移動方塊位置
            }
            else
            {
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,0,0);//清除上次紀錄位置方塊圖片
              Tetris0.log();//紀錄
              paste(0,(player_connect[3]%2)*5,Tetris0.MAP[0][0],2,20,10,Tetris0.color[Tetris0.block_choose],0);//貼上紀錄位置方塊圖片
              screen_change=1;//畫面改變
            }
          }
          if(Joystick[Tetris0.contral-2]==1)//0玩家向下
          {
            Joystick[Tetris0.contral-2]=0;
            Tetris0.drop_time=0;
          }
          if(player_connect[3]==2)//雙人模式
          {
            if(keyboard[2])//1搖桿A鍵被按下
            {
              keyboard[2]=0;
              Tetris1.direction++;//旋轉
              if(Tetris1.direction==4)
              {
                Tetris1.direction=0;
              }
              Tetris1.move();//旋轉
              if(Tetris1.state[0]==1||Tetris1.state[1]==1||Tetris1.state[1]==2||Tetris1.state[2]==1)//不合規
              {
                Tetris1.direction--;//回復旋轉
                if(Tetris1.direction==-1)
                {
                  Tetris1.direction=3;
                }
              }
              else
              {
                paste(0,10,Tetris1.MAP[0][0],2,20,10,0,0);//清除紀錄位置方塊圖片
                Tetris1.log();//紀錄
                paste(0,10,Tetris1.MAP[0][0],2,20,10,Tetris1.color[Tetris1.block_choose],0);//貼上紀錄位置方塊圖片
                screen_change=1;//畫面改變
              }
            }
            if(keyboard[3])//1搖桿B鍵被按下
            {
              keyboard[3]=0;
              for(i=0;i<3;i++)//計時暫停
              {
                time_game.time_pass(2,i);
              }
              game_stop_menu();//暫停選單
              for(i=0;i<3;i++)//計時繼續
              {
                time_game.time_pass(3,i);
              }
            }
            if(Joystick[3]==1)//1搖桿向右
            {
              Joystick[3]=0;
              Tetris1.location[1]++;//方塊位置右移
              Tetris1.move();//模擬移動方塊位置
              if(Tetris1.state[2]==1||Tetris1.state[1]==1)//碰到方塊或超出右方邊界  
              {
                Tetris1.location[1]--;//方塊位置左移
                Tetris1.move();//模擬移動方塊位置
              }
              else
              {
                paste(0,10,Tetris1.MAP[0][0],2,20,10,0,0);//清除上次紀錄位置方塊圖片
                Tetris1.log();//紀錄
                paste(0,10,Tetris1.MAP[0][0],2,20,10,Tetris1.color[Tetris1.block_choose],0);//貼上紀錄位置方塊圖片
                screen_change=1;//畫面改變
              } 
            }
            else if(Joystick[3]==2)//1搖桿向左
            {
              Joystick[3]=0;
              Tetris1.location[1]--;//方塊位置左移
              Tetris1.move();//模擬移動方塊位置
              if(Tetris1.state[2]==1||Tetris1.state[1]==2)//碰到方塊或超出左方邊界  
              {
                Tetris1.location[1]++;//方塊位置右移
                Tetris1.move();//模擬移動方塊位置
              }
              else
              {
                paste(0,10,Tetris1.MAP[0][0],2,20,10,0,0);//清除上次紀錄位置方塊圖片
                Tetris1.log();//紀錄
                paste(0,10,Tetris1.MAP[0][0],2,20,10,Tetris1.color[Tetris1.block_choose],0);//貼上紀錄位置方塊圖片
                screen_change=1;//畫面改變
              }
            }
            if(Joystick[2]==1)//1搖桿向下
            {
              Joystick[2]=0;
              Tetris1.drop_time=0;
            }
          } 
        }
      }    
      ////俄羅斯方塊結束////
      Tetris0.end();
      Tetris1.end();
      if(fraction[0][0]>fraction[0][1])
      {
        fraction[0][1]=fraction[0][0];
      }
      if(fraction[1][0]>fraction[1][1])
      {
        fraction[1][1]=fraction[1][0];
      }
      fraction[0][0]=0;
      fraction[1][0]=0;
    }
    ////////圈圈叉叉////////
    else if(picture.menu_map[1]==2)
    {
      player_connect[3]=player_connect[2]+player_connect[1];//遊戲人數
      Tic_tac_toe.initialize();//遊戲初始化
      time_game.time_pass(1,0);//遊戲時間計時    
      paste(0,0,Tic_tac_toe.map[0],0,20,20,256*256*32+256*32+32,0);//貼上地圖圖片
      cutscenes(0);//過場動畫消失
      for(i=0;i<6;i++)//游標
      {
        for(f=0;f<6;f++)
        {
          screen_out[0][1][i+7*Tic_tac_toe.cursor[0]][f+7*Tic_tac_toe.cursor[1]]=32; 
        }
      }
      screen_change=1;//畫面改變
      if(player_connect[3]==1)//單人模式
      {
        Tic_tac_toe.contral=player_connect[0];//設定搖桿
        Tic_tac_toe.contral=Tic_tac_toe.contral*2;
      }
      while(win0==0&&win1==0)
      {
        if(player_connect[3]==1)//單人模式
        {
          win0=2;
          win1=2;
        }
        else if(player_connect[3]==2)//雙人模式
        {
          if(Joystick[Tic_tac_toe.round*2]!=0||Joystick[Tic_tac_toe.round*2+1]!=0)//回合玩家搖桿有動作
          {
            for(i=0;i<6;i++)
            {
              for(f=0;f<6;f++)
              {
                erase(i+7*Tic_tac_toe.cursor[0],f+7*Tic_tac_toe.cursor[1],0);
              }
            }
            if(Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]!=0)
            {
              paste(Tic_tac_toe.cursor[0]*7,Tic_tac_toe.cursor[1]*7,Tic_tac_toe.flag[0][0],Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]-1,6,6,Tic_tac_toe.color[Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]-1],0);//貼上原位置旗幟圖片
            }
            if(Joystick[Tic_tac_toe.round*2+1]==1)//回合玩家向右
            {
              Joystick[Tic_tac_toe.round*2+1]=0;
              if(Tic_tac_toe.cursor[1]<2)//允許向右
              {
                Tic_tac_toe.cursor[1]++;//游標向右
              }
            }
            else if(Joystick[Tic_tac_toe.round*2+1]==2)//回合玩家向左
            {
              Joystick[Tic_tac_toe.round*2+1]=0;
              if(Tic_tac_toe.cursor[1]>0)//允許向左
              {
                Tic_tac_toe.cursor[1]--;//游標向左
              }
            }
            if(Joystick[Tic_tac_toe.round*2]==1)//回合玩家向下
            {
              Joystick[Tic_tac_toe.round*2]=0;
              if(Tic_tac_toe.cursor[0]<2)//允許向下
              {
                Tic_tac_toe.cursor[0]++;//游標向下
              }
            }
            else if(Joystick[Tic_tac_toe.round*2]==2)//回合玩家向上
            {
              Joystick[Tic_tac_toe.round*2]=0;
              if(Tic_tac_toe.cursor[0]>0)//允許向上
              {
                Tic_tac_toe.cursor[0]--;//游標向上
              }
            }
            for(i=0;i<6;i++)
            {
              for(f=0;f<6;f++)
              {
                screen_out[0][1][i+7*Tic_tac_toe.cursor[0]][f+7*Tic_tac_toe.cursor[1]]=32;//放上游標(反綠)
              }
            }
            if(Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]!=0)
            {
              paste(Tic_tac_toe.cursor[0]*7,Tic_tac_toe.cursor[1]*7,Tic_tac_toe.flag[0][0],Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]-1,6,6,Tic_tac_toe.color[Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]-1],0);//貼上現在位置旗幟圖片
            }
            screen_change=1;//畫面改變
          }
          if(keyboard[Tic_tac_toe.round*2])//回合玩家A鍵被按下
          {
            keyboard[Tic_tac_toe.round*2]=0;
            Tic_tac_toe.choose(Tic_tac_toe.cursor[0],Tic_tac_toe.cursor[1]);//進行選擇位置
            if(Tic_tac_toe.stat_all==0)//選擇成功
            {
              paste(Tic_tac_toe.cursor[0]*7,Tic_tac_toe.cursor[1]*7,Tic_tac_toe.flag[0][0],Tic_tac_toe.round,6,6,Tic_tac_toe.color[Tic_tac_toe.round],0);//貼上回合玩家旗幟圖片
              screen_change=1;//畫面改變
              Tic_tac_toe.round=!Tic_tac_toe.round;//換回合
            }
            else if(Tic_tac_toe.stat_all==1)//選擇失敗
            {
              for(i=0;i<6;i++)
              {
                for(f=0;f<6;f++)
                {
                  screen_out[0][0][i+7*Tic_tac_toe.cursor[0]][f+7*Tic_tac_toe.cursor[1]]=32;//放上游標(反紅)
                  screen_out[0][1][i+7*Tic_tac_toe.cursor[0]][f+7*Tic_tac_toe.cursor[1]]=0;//放上游標(刪綠)
                }
              }
              if(Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]!=0)
              {
                paste(Tic_tac_toe.cursor[0]*7,Tic_tac_toe.cursor[1]*7,Tic_tac_toe.flag[0][0],Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]-1,6,6,Tic_tac_toe.color[Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]-1],0);//貼上現在位置旗幟圖片
              }
              screen_change=1;//畫面改變
              delay(300);
              for(i=0;i<6;i++)
              {
                for(f=0;f<6;f++)
                {
                  screen_out[0][1][i+7*Tic_tac_toe.cursor[0]][f+7*Tic_tac_toe.cursor[1]]=32;//放上游標(反綠)
                  screen_out[0][0][i+7*Tic_tac_toe.cursor[0]][f+7*Tic_tac_toe.cursor[1]]=0;//放上游標(刪紅)
                }
              }
              if(Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]!=0)
              {
                paste(Tic_tac_toe.cursor[0]*7,Tic_tac_toe.cursor[1]*7,Tic_tac_toe.flag[0][0],Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]-1,6,6,Tic_tac_toe.color[Tic_tac_toe.choose_log[Tic_tac_toe.cursor[0]][Tic_tac_toe.cursor[1]]-1],0);//貼上現在位置旗幟圖片
              }
              screen_change=1;//畫面改變
            }
            else if(Tic_tac_toe.stat_all==2)//回合玩家獲勝
            {
              paste(Tic_tac_toe.cursor[0]*7,Tic_tac_toe.cursor[1]*7,Tic_tac_toe.flag[0][0],Tic_tac_toe.round,6,6,Tic_tac_toe.color[Tic_tac_toe.round],0);//貼上回合玩家旗幟圖片
              screen_change=1;//畫面改變
              if(Tic_tac_toe.round==0)//玩家0獲勝
              {
                win0=1;
                break;
              }
              else if(Tic_tac_toe.round==1)//玩家1獲勝
              {
                win1=1;
                break;
              }
            }
            else if(Tic_tac_toe.stat_all==3)//平手
            {
              win0=1;
              win1=1;
              break;
            }
            input_zero();
          }
          if(keyboard[1]||keyboard[3])//B鍵被按下
          {
            keyboard[1]=0;
            keyboard[3]=0;
            time_game.time_pass(2,0);
            game_stop_menu();//暫停選單
            time_game.time_pass(3,0);
          }
        }
      }
      //圈圈叉叉結束
    }
    ////////貪食蛇////////
    else if(picture.menu_map[1]==3)
    {
      Joystick_wait=12;
      player_connect[3]=player_connect[2]+player_connect[1];//遊戲人數
      snake0.initialize(0,player_connect[3]-1);//玩家0初始化
      time_game.time_pass(1,0);//遊戲時間計時
      time_game.time_pass(1,3);//玩家0移動計時
      ////初始化////
      if(player_connect[3]==1)//單人模式
      {
        snake0.contral=player_connect[0];//設定搖桿
        snake0.contral=snake0.contral*2;
        snake0.new_fruit();
        erase(snake0.fruit[0],snake0.fruit[1],0);//清除果實位置圖片
        screen_out[0][0][snake0.fruit[0]][snake0.fruit[1]]=63;//貼上果實位置圖片(紅色)
      } 
      else if(player_connect[3]==2)//雙人模式
      {
        time_game.time_pass(1,4);//玩家1移動計時
        snake1.initialize(1,1);//玩家0初始化
        snake0.contral=2;//設定搖桿
        snake1.contral=4;//設定搖桿
        snake0.new_fruit(snake1.Map[0],snake1.head_location,snake1.head_location);
        erase(snake0.fruit[0],snake0.fruit[1],0);//清除果實位置圖片
        screen_out[0][0][snake0.fruit[0]][snake0.fruit[1]]=63;//貼上果實位置圖片(紅色)
        screen_out[0][0][snake1.head_location[0]][snake1.head_location[1]]=63;//貼上玩家1蛇身體位置圖片(橘色)
        screen_out[0][1][snake1.head_location[0]][snake1.head_location[1]]=32;//貼上玩家1蛇身體位置圖片(橘色)
        screen_out[0][0][snake1.tail_location[0]][snake1.tail_location[1]]=63;//貼上玩家1蛇尾位置圖片(黃色)
        screen_out[0][1][snake1.tail_location[0]][snake1.tail_location[1]]=63;//貼上玩家1蛇尾體位置圖片(黃色)
        paste(0,0,snake1.Map[0],0,20,20,256*63+63,0);//貼上玩家0蛇身體位置圖片(黃色)
      }
      screen_out[0][1][snake0.head_location[0]][snake0.head_location[1]]=63;//貼上玩家0蛇頭位置圖片(青藍色)
      screen_out[0][2][snake0.head_location[0]][snake0.head_location[1]]=63;//貼上玩家0蛇頭位置圖片(青藍色)
      screen_out[0][1][snake0.tail_location[0]][snake0.tail_location[1]]=63;//貼上玩家0蛇尾位置圖片(綠色)
      paste(0,0,snake0.Map[0],0,20,20,256*63,0);//貼上玩家0蛇身體位置圖片(綠色)
      cutscenes(0);//過場動畫消失
      screen_change=1;//畫面改變
      ////遊戲開始////
      while(win0==0&&win1==0)
      {
        if(player_connect[3]==1)//單人模式
        {
          if(time_game.time_pass(0,3)>snake0.move_time)//移動時間到
          {
            snake0.head_move();//移動玩家0蛇頭
            snake0.judge();//判斷狀態
            if((snake0.state[2]==1)||(snake0.state[1]==1))//碰到牆壁或碰到身體蛇尾
            {
              win1=1;
              break;
            }
            if(snake0.state[0]==1)//吃到果實
            {
              snake0.new_fruit();
              erase(snake0.fruit[0],snake0.fruit[1],0);//清除果實位置圖片
              screen_out[0][0][snake0.fruit[0]][snake0.fruit[1]]=63;//貼上果實位置圖片(紅色)
            }
            else//未吃到
            {
              erase(snake0.tail_location[0],snake0.tail_location[1],0);//清除玩家0蛇尾位置圖片
              snake0.tail_move();//移動玩家0蛇尾
            }
            snake0.direction_last=snake0.direction;//紀錄方向
            erase(snake0.head_location[0],snake0.head_location[1],0);//清除玩家0蛇頭位置圖片
            erase(snake0.tail_location[0],snake0.tail_location[1],0);//清除玩家0蛇尾位置圖片
            screen_out[0][1][snake0.head_location[0]][snake0.head_location[1]]=63;//貼上玩家0蛇頭位置圖片(青藍色)
            screen_out[0][2][snake0.head_location[0]][snake0.head_location[1]]=63;//貼上玩家0蛇頭位置圖片(青藍色)
            screen_out[0][1][snake0.tail_location[0]][snake0.tail_location[1]]=63;//貼上玩家0蛇尾位置圖片(綠色)
            paste(0,0,snake0.Map[0],0,20,20,256*63,0);//貼上玩家0蛇身體位置圖片(綠色)
            screen_change=1;//畫面改變
            time_game.time_pass(1,3);//重新計時
          }
          if(Joystick[snake0.contral-1]==1)//0玩家向右
          {
            Joystick[snake0.contral-1]=0;
            if(snake0.direction_last!=2)//允許方向
            {
              snake0.direction=3;//方向向右
            }
          }
          else if(Joystick[snake0.contral-1]==2)//0玩家向左
          {
            Joystick[snake0.contral-1]=0;
            if(snake0.direction_last!=3)//允許方向
            {
              snake0.direction=2;//方向向左
            }
          }
          if(Joystick[snake0.contral-2]==1)//0玩家向下
          {
            Joystick[snake0.contral-2]=0;
            if(snake0.direction_last!=0)//允許方向
            {
              snake0.direction=1;//方向向下
            }
          }
          else if(Joystick[snake0.contral-2]==2)//0玩家向上
          {
            Joystick[snake0.contral-2]=0;
            if(snake0.direction_last!=1)//允許方向
            {
              snake0.direction=0;//方向向上
            }
          }
          if(keyboard[snake0.contral-1])//0玩家B鍵被按下
          {
            keyboard[snake0.contral-1]=0;
            time_game.time_pass(2,0);
            for(i=3;i<5;i++)//計時暫停
            {
              time_game.time_pass(2,i);
            }
            game_stop_menu();//暫停選單
            time_game.time_pass(3,0);
            for(i=3;i<5;i++)//計時繼續
            {
              time_game.time_pass(3,i);
            }
          }
        }
        else if(player_connect[3]==2)//雙人模式
        {
          if(time_game.time_pass(0,3)>snake0.move_time)//玩家0移動時間到
          {
            snake0.head_move();//移動玩家0蛇頭
            snake1.head_move();//移動玩家1蛇頭
            snake0.judge(snake1.Map[0],snake1.head_location[0],snake1.head_location[1],snake0.fruit[0],snake0.fruit[1]);//判斷狀態
            snake1.judge(snake0.Map[0],snake0.head_location[0],snake0.head_location[1],snake0.fruit[0],snake0.fruit[1]);//判斷狀態
            //玩家0反應//
            if((snake0.state[2]==1)||(snake0.state[1]==1)||(snake0.state[1]==2)||(snake0.state[3]==1))//碰到牆壁或碰到身體蛇尾
            {
              win1=1;
            }
            if(snake0.state[0]==1)//吃到果實
            {
              snake0.new_fruit(snake1.Map[0],snake1.head_location,snake1.head_location);
              erase(snake0.fruit[0],snake0.fruit[1],0);//清除果實位置圖片
              screen_out[0][0][snake0.fruit[0]][snake0.fruit[1]]=63;//貼上果實位置圖片(紅色)
            }
            else//未吃到
            {
              erase(snake0.tail_location[0],snake0.tail_location[1],0);//清除玩家0蛇尾位置圖片
              snake0.tail_move();//移動玩家0蛇尾
            }
            snake0.direction_last=snake0.direction;//紀錄方向
            erase(snake0.head_location[0],snake0.head_location[1],0);//清除玩家0蛇頭位置圖片
            erase(snake0.tail_location[0],snake0.tail_location[1],0);//清除玩家0蛇尾位置圖片
            screen_out[0][1][snake0.head_location[0]][snake0.head_location[1]]=63;//貼上玩家0蛇頭位置圖片(青藍色)
            screen_out[0][2][snake0.head_location[0]][snake0.head_location[1]]=63;//貼上玩家0蛇頭位置圖片(青藍色)
            screen_out[0][1][snake0.tail_location[0]][snake0.tail_location[1]]=63;//貼上玩家0蛇尾位置圖片(綠色)
            paste(0,0,snake0.Map[0],0,20,20,256*63,0);//貼上玩家0蛇身體位置圖片(綠色)
            //玩家1反應//
            if((snake1.state[2]==1)||(snake1.state[1]==1)||(snake1.state[1]==2)||(snake1.state[3]==1))//碰到牆壁或碰到身體蛇尾
            {
              win0=1;
            }
            if(snake1.state[0]==1)//吃到果實
            {
              snake0.new_fruit(snake1.Map[0],snake1.head_location,snake1.head_location);
              erase(snake0.fruit[0],snake0.fruit[1],0);//清除果實位置圖片
              screen_out[0][0][snake0.fruit[0]][snake0.fruit[1]]=63;//貼上果實位置圖片(紅色)
            }
            else//未吃到
            {
              erase(snake1.tail_location[0],snake1.tail_location[1],0);//清除玩家1蛇尾位置圖片
              snake1.tail_move();//移動玩家1蛇尾
            }
            snake1.direction_last=snake1.direction;//紀錄方向
            erase(snake1.head_location[0],snake1.head_location[1],0);//清除玩家1蛇頭位置圖片
            erase(snake1.tail_location[0],snake1.tail_location[1],0);//清除玩家1蛇尾位置圖片
            screen_out[0][0][snake1.head_location[0]][snake1.head_location[1]]=63;//貼上玩家1蛇身體位置圖片(橘色)
            screen_out[0][1][snake1.head_location[0]][snake1.head_location[1]]=32;//貼上玩家1蛇身體位置圖片(橘色)
            screen_out[0][0][snake1.tail_location[0]][snake1.tail_location[1]]=63;//貼上玩家1蛇尾位置圖片(黃色)
            screen_out[0][1][snake1.tail_location[0]][snake1.tail_location[1]]=63;//貼上玩家1蛇尾位置圖片(黃色)
            paste(0,0,snake1.Map[0],0,20,20,256*63+63,0);//貼上玩家0蛇身體位置圖片(黃色)
            screen_change=1;//畫面改變
            time_game.time_pass(1,3);//重新計時
          }
          if(Joystick[snake0.contral-1]==1)//0玩家向右
          {
            Joystick[snake0.contral-1]=0;
            if(snake0.direction_last!=2)//允許方向
            {
              snake0.direction=3;//方向向右
            }
          }
          else if(Joystick[snake0.contral-1]==2)//0玩家向左
          {
            Joystick[snake0.contral-1]=0;
            if(snake0.direction_last!=3)//允許方向
            {
              snake0.direction=2;//方向向右
            }
          }
          if(Joystick[snake0.contral-2]==1)//0玩家向下
          {
            Joystick[snake0.contral-2]=0;
            if(snake0.direction_last!=0)//允許方向
            {
              snake0.direction=1;//方向向右
            }
          }
          else if(Joystick[snake0.contral-2]==2)//0玩家向上
          {
            Joystick[snake0.contral-2]=0;
            if(snake0.direction_last!=1)//允許方向
            {
              snake0.direction=0;//方向向右
            }
          }
          if(keyboard[snake0.contral-1])//0玩家B鍵被按下
          {
            keyboard[snake0.contral-1]=0;
            time_game.time_pass(2,0);
            for(i=3;i<5;i++)//計時暫停
            {
              time_game.time_pass(2,i);
            }
            game_stop_menu();//暫停選單
            time_game.time_pass(3,0);
            for(i=3;i<5;i++)//計時繼續
            {
              time_game.time_pass(3,i);
            }
          }
          if(Joystick[3]==1)//1玩家向右
          {
            Joystick[3]=0;
            if(snake1.direction_last!=2)//允許方向
            {
              snake1.direction=3;//方向向右
            }
          }
          else if(Joystick[3]==2)//1玩家向左
          {
            Joystick[3]=0;
            if(snake1.direction_last!=3)//允許方向
            {
              snake1.direction=2;//方向向右
            }
          }
          if(Joystick[2]==1)//1玩家向下
          {
            Joystick[2]=0;
            if(snake1.direction_last!=0)//允許方向
            {
              snake1.direction=1;//方向向右
            }
          }
          else if(Joystick[2]==2)//1玩家向上
          {
            Joystick[2]=0;
            if(snake1.direction_last!=1)//允許方向
            {
              snake1.direction=0;//方向向右
            }
          }
          if(keyboard[3])//1玩家B鍵被按下
          {
            keyboard[3]=0;
            time_game.time_pass(2,0);
            for(i=3;i<5;i++)//計時暫停
            {
              time_game.time_pass(2,i);
            }
            game_stop_menu();//暫停選單
            time_game.time_pass(3,0);
            for(i=3;i<5;i++)//計時繼續
            {
              time_game.time_pass(3,i);
            }
          }
        }
        else//有bug
        {
          break;//跳出遊戲迴圈
        }
      }
      ////貪食蛇結束結束////
      snake0.end();
      snake1.end();
    }
    /////遊戲結束(全部遊戲)/////
    end_skip=0;
    for(i=0;i<20;i++)
    {
      for(f=0;f<20;f++)
      {
        for(g=0;g<3;g++)
        {
          screen_out[1][g][i][f]=screen_out[0][g][i][f];//備分現在畫面
        }
      }
    }
    if(player_connect[3]==1)//單人模式結束遊戲
    {
      if(win0==2||win1==2)//暫停選單中退出(直接回主選單)
      {
      }
      else if(win1==1)//遊戲結束畫面
      {
        for(h=0;h<5;h++)
        {
          for(i=5;i<14;i++)
          {
            for(f=1;f<19;f++)
            {
              erase(i,f,0);
            }
          }
          paste(5,1,picture.frame[0],0,9,18,256*256*63+256*63+63,0);//貼上方框圖片(白色)
          paste(5,1,picture.end[0],0,9,18,63,0);//貼上end圖片(紅色)
          screen_change=1;//畫面改變
          time_game.time_pass(1,4);//開始計時閃爍時間
          while(time_game.time_pass(0,4)<1000)//等待1秒
          {
            if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任意按鍵跳過結束畫面
            {
              input_zero();
              end_skip=1;
              break;
            }
          }
          if(end_skip)
          {
            break;
          }
          for(i=0;i<20;i++)
          {
            for(f=0;f<20;f++)
            {
              for(g=0;g<3;g++)
              {
                screen_out[0][g][i][f]=screen_out[1][g][i][f];
              }
            }
          }
          screen_change=1;//畫面改變
          time_game.time_pass(1,4);//開始計時閃爍時間
          while(time_game.time_pass(0,4)<1000)//等待1秒
          {
            if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任意按鍵跳過結束畫面
            {
              input_zero();
              end_skip=1;
              break;
            }
          }
          if(end_skip)
          {
            break;
          }
        }
      }
    }
    else if(player_connect[3]==2)//雙人模式結束遊戲
    {
      if(win0==2||win1==2)//暫停選單中退出(直接回主選單)
      {
      }
      else if(win0==1&&win1==0)//玩家0贏畫面
      {
        for(h=0;h<5;h++)
        {
          for(i=2;i<17;i++)
          {
            for(f=0;f<9;f++)
            {
              erase(i,f,0);
            }
          }
          paste(2,0,picture.end_frame[0],0,15,9,256*256*63+256*63+63,0);//貼上方框圖片(白色)
          paste(2,0,picture.win[0],0,15,9,63,0);//貼上win圖片(紅色)
          screen_change=1;//畫面改變
          time_game.time_pass(1,4);//開始計時閃爍時間
          while(time_game.time_pass(0,4)<1000)//等待1秒
          {
            if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任意按鍵跳過結束畫面
            {
              input_zero();
              end_skip=1;
              break;
            }
          }
          if(end_skip)
          {
            break;
          }
          for(i=0;i<20;i++)
          {
            for(f=0;f<20;f++)
            {
              for(g=0;g<3;g++)
              {
                screen_out[0][g][i][f]=screen_out[1][g][i][f];
              }
            }
          }
          screen_change=1;//畫面改變
          time_game.time_pass(1,4);//開始計時閃爍時間
          while(time_game.time_pass(0,4)<1000)//等待1秒
          {
            if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任意按鍵跳過結束畫面
            {
              input_zero();
              end_skip=1;
              break;
            }
          }
          if(end_skip)
          {
            break;
          }
        }
      }
      else if(win0==0&&win1==1)//玩家1贏畫面
      {
        for(h=0;h<5;h++)
        {
          for(i=2;i<17;i++)
          {
            for(f=11;f<20;f++)
            {
              erase(i,f,0);
            }
          }
          paste(2,11,picture.end_frame[0],0,15,9,256*256*63+256*63+63,0);//貼上方框圖片(白色)
          paste(2,11,picture.win[0],0,15,9,63,0);//貼上win圖片(紅色)
          screen_change=1;//畫面改變
          time_game.time_pass(1,4);//開始計時閃爍時間
          while(time_game.time_pass(0,4)<1000)//等待1秒
          {
            if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任意按鍵跳過結束畫面
            {
              input_zero();
              end_skip=1;
              break;
            }
          }
          if(end_skip)
          {
            break;
          }
          for(i=0;i<20;i++)
          {
            for(f=0;f<20;f++)
            {
              for(g=0;g<3;g++)
              {
                screen_out[0][g][i][f]=screen_out[1][g][i][f];
              }
            }
          }
          screen_change=1;//畫面改變
          time_game.time_pass(1,4);//開始計時閃爍時間
          while(time_game.time_pass(0,4)<1000)//等待1秒
          {
            if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任意按鍵跳過結束畫面
            {
              input_zero();
              end_skip=1;
              break;
            }
          }
          if(end_skip)
          {
            break;
          }
        } 
      }
      else if(win0==1&&win1==1)//平手畫面
      {
        for(h=0;h<5;h++)
        {
          for(i=2;i<17;i++)
          {
            for(f=11;f<20;f++)
            {
              erase(i,f,0);
            }
          }
          paste(2,0,picture.end_frame[0],0,15,9,256*256*63+256*63+63,0);//貼上方框圖片(白色)
          paste(2,0,picture.tie[0],0,15,9,63,0);//貼上tie圖片(紅色)
          screen_change=1;//畫面改變
          time_game.time_pass(1,4);//開始計時閃爍時間
          while(time_game.time_pass(0,4)<1000)//等待1秒
          {
            if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任意按鍵跳過結束畫面
            {
              input_zero();
              end_skip=1;
              break;
            }
          }
          if(end_skip)
          {
            break;
          }
          for(i=0;i<20;i++)
          {
            for(f=0;f<20;f++)
            {
              for(g=0;g<3;g++)
              {
                screen_out[0][g][i][f]=screen_out[1][g][i][f];
              }
            }
          }
          screen_change=1;//畫面改變
          time_game.time_pass(1,4);//開始計時閃爍時間
          while(time_game.time_pass(0,4)<1000)//等待1秒
          {
            if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任意按鍵跳過結束畫面
            {
              input_zero();
              end_skip=1;
              break;
            }
          }
          if(end_skip)
          {
            break;
          }
        } 
      }
    }
  }
  if(begin==2)//進入設定
  {
    if(picture.menu_map[1]==6)//亮度設定
    {
      for(i=0;i<20;i++)
      {
        for(f=0;f<20;f++)
        {
          if(!(i<12&&i>7&&f<19&&f>0))
          {
            screen_out[0][0][i][f]=41;
            screen_out[0][1][i][f]=41;
            screen_out[0][2][i][f]=41;
          }
        }
      }
      f=0;
      while((f+3)*2<screen_brightness)
      {
        screen_out[0][0][i+9][f+2]=125;
        screen_out[0][0][i+10][f+2]=125;
        f++;
      }
      cutscenes(0);//過場動畫消失
      while(1)
      {
        if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//按鍵被按下
        {
          break;
        }
        if(Joystick[1]!=0||Joystick[3]!=0)//搖桿有動作
        {
          if(f<16&&(Joystick[1]==1||Joystick[3]==1))//搖桿右
          {
            Joystick[1]=0;
            Joystick[3]=0;
            screen_out[0][0][i+9][f+2]=125;
            screen_out[0][0][i+10][f+2]=125;
            f++;
            screen_brightness+=2;
            ws2812.setBrightness(screen_brightness);//限制亮度
          }
          else if(f>1&&(Joystick[1]==2||Joystick[3]==2))//搖桿左
          {
            Joystick[1]=0;
            Joystick[3]=0;
            f--;
            screen_out[0][0][i+9][f+2]=0;
            screen_out[0][0][i+10][f+2]=0;
            screen_brightness-=2;
            ws2812.setBrightness(screen_brightness);//限制亮度
          }
          screen_change=1;//畫面改變
          Joystick[1]=0;
          Joystick[3]=0;
        }
      }
    }
    /*else if(picture.menu_map[1]==7)//靈敏度設定
    {

    }*/
    else if(picture.menu_map[1]==7)//音量設定
    {
      if(music.muted==0)//目前為有聲
      {
        paste(4,4,picture.volume[0],0,12,12,128,0);//貼上聲音
      }
      else//目前為無聲
      {
        paste(4,4,picture.volume_mute[0],0,12,12,128,0);//貼上無聲音
      }
      cutscenes(0);//過場動畫消失
      while(1)
      {
        if(Joystick[0]!=0||Joystick[1]!=0||Joystick[2]!=0||Joystick[3]!=0)//搖桿y軸動作
        {
          Joystick[0]=0;
          Joystick[1]=0;
          Joystick[2]=0;
          Joystick[3]=0;
          music.muted=!music.muted;//設置為下一個模式(有聲/無聲)
          erase(0,0,1);//清除畫面
          if(music.muted==0)//目前為有聲
          {
            paste(4,4,picture.volume[0],0,12,12,128,0);//貼上聲音
          }
          else//目前為無聲
          {
            paste(4,4,picture.volume_mute[0],0,12,12,128,0);//貼上無聲音
          }
          screen_change=1;//畫面改變
        }
        else if(keyboard[0]||keyboard[1]||keyboard[2]||keyboard[3])//任何按鍵被按下
        {
          keyboard[0]=0;
          keyboard[1]=0;
          keyboard[2]=0;
          keyboard[3]=0;
          break;//跳出設定
        }
      }
      
    }
  }
  /////恢復設定(全部)/////
  picture.menu_map[0]=1;
  picture.menu_map[1]=1;
  begin=0;
  win0=0;
  win1=0;
}

//////////////////////////////////////////////////////////////////////////////////////
void game_stop_menu(void)//暫停選單
{
  byte i,f,g;
  menu_choose=1;
  input_zero();//搖桿設為未動作
  for(i=0;i<20;i++)//備分畫面
  {
    for(f=0;f<20;f++)
    {
      for(g=0;g<3;g++)
      {
        screen_out[1][g][i][f]=screen_out[0][g][i][f];
      }
    } 
  }
  for(i=5;i<14;i++)//清除顯示選單區域畫面
  {
    for(f=1;f<19;f++)
    {
      erase(i,f,0);
    }
  }
  paste(5,1,picture.frame[0],0,9,18,256*256*63+256*63+63,0);//貼上方框圖片(白色)
  paste(7,10,picture.ARROW2[0],0,5,6,256*63,0);//貼上繼續箭頭圖片(綠色)
  paste(7,3,picture.back[0],0,5,6,63,0);//貼上返回選單圖片(紅色)
  screen_change=1;//畫面改變
  while(1)//選單
  {
    if(Joystick[1]!=0||Joystick[3]!=0)//搖桿有動作
    {
      if(menu_choose==1&&(Joystick[1]==2||Joystick[3]==2))//搖桿向左
      {
        menu_choose=0;
        paste(7,10,picture.ARROW2[0],0,5,6,63,0);//貼上繼續箭頭圖片(紅色)
        paste(7,3,picture.back[0],0,5,6,256*63,0);//貼上返回選單圖片(綠色)
        screen_change=1;//畫面改變
      }
      else if(menu_choose==0&&(Joystick[1]==1||Joystick[3]==1))//搖桿向右
      {
        menu_choose=1;
        paste(7,10,picture.ARROW2[0],0,5,6,256*63,0);//貼上繼續箭頭圖片(綠色)
        paste(7,3,picture.back[0],0,5,6,63,0);//貼上返回選單圖片(紅色)
        screen_change=1;//畫面改變
      }
      Joystick[1]=0;
      Joystick[3]=0;
    }
    if(keyboard[0]||keyboard[2])//搖桿A鍵被按下
    {
      keyboard[0]=0;
      keyboard[2]=0;
      if(menu_choose==1)//繼續遊戲
      {
        break;
      }
      else if(menu_choose==0)//返回主選單
      {
        win0=2;
        win1=2;
        break;
      }
    }
  }
  for(i=0;i<20;i++)//恢復原本畫面
  {
    for(f=0;f<20;f++)
    {
      for(g=0;g<3;g++)
      {
        screen_out[0][g][i][f]=screen_out[1][g][i][f];
      }
    } 
  }
  input_zero();//搖桿設為未動作
  screen_change=1;//畫面改變
}  
void task_core0(void*)
{
  byte i0,f0,g0,h0;//迴圈用
  ////顯示器初始化////
  byte row_next=0;//下次刷新第幾行 從第0列開始
  ws2812.clear();//確保歸零
  ws2812.show();
  ws2812.begin();//初始設定
  ws2812.setBrightness(screen_brightness);//限制亮度
  ////搖桿初始化////
  for(i0=0;i0<8;i0++)
  {
    time_contral.time_pass(1,i0);
  }
  ////聲音初始化////
  time_system.time_pass(1,0);
  for(;;)
  {
    ////螢幕輸出處理////
    for(g0=0;g0<6;g0++)
    {
      if(screen_change==1)//更改畫面
      {
        screen_change=0;
        for(i0=0;i0<20;i0++)
        {
          for(row_next=0;row_next<20;row_next++)
          {
            ws2812.setPixelColor((row_next%4*4)+(row_next/4*80)+(i0%4+(row_next%2)*(3-i0%4*2))+(i0/4*16),screen_out[0][0][row_next][i0],screen_out[0][1][row_next][i0],screen_out[0][2][row_next][i0]);
          }
        }
        ws2812.show();
      }
      ////搖桿輸入處理////
      if(begin==0)//遊戲未開始時
      {
        player_connect[1]=digitalRead(keyboard_pin[0][4]);//讀取搖桿0是否連接
        player_connect[2]=digitalRead(keyboard_pin[1][4]);//讀取搖桿1是否連接
        if(player_connect[0]==0)//上次未有搖桿連接
        {
          if(player_connect[1])//搖桿0先連接
          {
            player_connect[0]=1;
          }
          else if(player_connect[2])//搖桿1先連接
          {
            player_connect[0]=2;
          }
        }
        else//上次有搖桿連接
        {
          if((player_connect[1]==1)&&(player_connect[2]==0))//設置為未有搖桿連接
          {
            player_connect[0]=1;
          }
          else if((player_connect[1]==0)&&(player_connect[2]==1))//設置為未有搖桿連接
          {
            player_connect[0]=2;
          }
          else if((player_connect[1]==0)&&(player_connect[2]==0))//設置為未有搖桿連接
          {
            player_connect[0]=0;
          }
        }
      }
      if(player_connect[1])//搖桿0連接
      {
        for(i0=0;i0<2;i0++)
        {
          Serial.println(Joystick_log[0][0][0]);
          Joystick_log[0][i0][g0]=analogRead(keyboard_pin[0][i0]);//讀取搖桿數y、x值
          if(time_contral.time_pass(0,i0)>Joystick_wait)//判斷搖桿y、x狀態
          {
            if(noise_removal(i0)>voltage_level[0])//判斷為下或右
            {
              time_contral.time_pass(1,i0);
              Joystick[i0]=1;
            }
            if(noise_removal(i0)<voltage_level[1])//判斷為上或左
            {
              time_contral.time_pass(1,i0);
              Joystick[i0]=2;
            }
          }
          if(time_contral.time_pass(0,i0+2)>debouncetime&&digitalRead(keyboard_pin[0][2+i0]))
          {
            time_contral.time_pass(1,i0+2);
            keyboard[i0]=1;
          }
        } 
      }
      if(player_connect[2])//搖桿1連接
      {
        for(i0=0;i0<2;i0++)//讀取搖桿數x、y值
        {
          Joystick_log[1][i0][g0]=analogRead(keyboard_pin[1][i0]);   
          if(time_contral.time_pass(0,i0+4)>Joystick_wait)//判斷搖桿y、x狀態
          {
            if(noise_removal(i0+2)>voltage_level[0])//判斷為下或右
            { 
              time_contral.time_pass(1,i0+4);
              Joystick[i0+2]=1;
            }
            if(noise_removal(i0+2)<voltage_level[1])//判斷為上或左
            {
              time_contral.time_pass(1,i0+4);
              Joystick[i0+2]=2;
            }
          }
          if(time_contral.time_pass(0,i0+6)>debouncetime&&digitalRead(keyboard_pin[1][2+i0]))
          {
            time_contral.time_pass(1,i0+6);
            keyboard[2+i0]=1;
          }     
        } 
      }
    }   
    ////聲音輸出處理////
    if((time_system.time_pass(0,0)>music.BGM_duration[music.BGM_index]*TEMPO/1000+TEMPO/1000)&&!(music.muted))
    {
      music.BGM_index++;
      if(music.BGM_index==music.BGM_length)
      {
        music.BGM_index=0;
      }
      tone(buzzer_pin,music.BGM[music.BGM_index],music.BGM_duration[music.BGM_index]*TEMPO/1000);
      time_system.time_pass(1,0);
    }
  }
}
void cutscenes(bool mode/*0:消失過場動畫 1:出現過場動畫*/)//過場動畫
{
  int i,f,g,count=0;
  if(mode)//出現過場動畫
  {
    int location[4][2]={{0,0},{0,15},{15,15},{15,0}};//[0:左上 1:右上 2:右下 3:左下][0:y 1:x]
    for(i=0;i<4;i++)
    {
      paste(location[i][0],location[i][1],picture.cutscene[0],0,5,5,128,0);//貼上過場動畫
    }
    screen_change=1;//畫面改變
    delay(20);
    for(count=0;count<10;count++)
    {
      location[0][0]++;
      location[1][1]--;
      location[2][0]--;
      location[3][1]++;
      for(i=0;i<4;i++)
      {
        paste(location[i][0],location[i][1],picture.cutscene[0],0,5,5,128,0);//貼上過場動畫
      }
      screen_change=1;//畫面改變
      delay(20);
    }
    for(count=0;count<5;count++)
    {
      location[0][1]++;
      location[1][0]++;
      location[2][1]--;
      location[3][0]--;
      for(i=0;i<4;i++)
      {
        paste(location[i][0],location[i][1],picture.cutscene[0],0,5,5,128,0);//貼上過場動畫
      }
      screen_change=1;//畫面改變
      delay(20);
    }
  }
  else//消失過場動畫
  {
    for(i=0;i<20;i++)
    {
      for(f=0;f<20;f++)
      {
        for(g=0;g<3;g++)
        {
          screen_out[1][g][i][f]=screen_out[0][g][i][f];//備分畫面
        }
      }
    }
    for(count=0;count<10;count++)
    {
      for(i=0;i<20;i++)
      {
        for(f=0;f<20;f++)
        {
          screen_out[0][0][i][f]=128;//全畫面變紅
          screen_out[0][1][i][f]=0;//全畫面變紅
          screen_out[0][2][i][f]=0;//全畫面變紅
        }
      }
      for(i=9-count;i<11+count;i++)
      {
        for(f=9-count;f<11+count;f++)
        {
          for(g=0;g<3;g++)
          {
            screen_out[0][g][i][f]=screen_out[1][g][i][f];//備分畫面
          }
        }  
      }
      screen_change=1;//畫面改變
      delay(20);
    }
  }
}
int noise_removal(int pin/*選擇哪個搖桿的y或x(0、1、2、3)*/)
{
  int i,f,g;//迴圈用
  int max=0,min=0;
  for(i=1;i<6;i++)
  {
    if(Joystick_log[pin/2][pin%2][max]<Joystick_log[pin/2][pin%2][i])//取最大值
    {
      max=i;
    }
    else if(Joystick_log[pin/2][pin%2][min]>Joystick_log[pin/2][pin%2][i])//取最小值
    {
      min=i;
    }
  }
  if(Joystick_log[pin/2][pin%2][max]==Joystick_log[pin/2][pin%2][min])//最大和最小都一樣就直接輸出
  {
    return Joystick_log[pin/2][pin%2][max];
  }
  else//取平均值
  {
    return (Joystick_log[pin/2][pin%2][0]+Joystick_log[pin/2][pin%2][1]+Joystick_log[pin/2][pin%2][2]+Joystick_log[pin/2][pin%2][3]+Joystick_log[pin/2][pin%2][4]+Joystick_log[pin/2][pin%2][5]-Joystick_log[pin/2][pin%2][min]-Joystick_log[pin/2][pin%2][max])/4;
  }
}
void Marquee_set(bool set,bool *MAP)//設定跑馬燈字
{
  int i,f;
  if(set==0)//set=0:刪除字串
  {
    Marquee_word_count=0; 
    Marquee_round=0;        
    for(i=0;i<12;i++)
    {
      for(f=0;f<24;f++)
      {
        Marquee_word[i][f]=0;
      } 
    }
  }
  else if(set==1)//set=1:往後放入字
  {
    Marquee_word_count++;
    for(i=0;i<12;i++)
    {
      for(f=0;f<12;f++)
      {
        Marquee_word[i][f+(Marquee_word_count-1)*12]=MAP[i*12+f];
      }
    }
  }
}
void Marquee(byte y,byte x)//跑馬燈  
{//          (   起始點   )
  int i,f;
  if(Marquee_word_count*12>20)
  {
    for(i=0;i<12;i++)
    {
      for(f=0;f<20;f++)
      {
        Marquee_out[i][f]=Marquee_word[i][f+Marquee_round];
      } 
    }
    for(i=0;i<12;i++)
    {
      for(f=0;f<20;f++)
      {
        erase(i+y,f,0);//清除跑馬燈範圍
      }  
    }
    paste(y,x,Marquee_out[0],0,12,20,16,0);//紅色字體
    Marquee_round+=2;
    if(Marquee_round+20>Marquee_word_count*12)
    {
      Marquee_round=0;
    }
  }
}  
void paste(byte y,byte x,bool *MAP,byte MAP_Z,byte MAP_max_y,byte MAP_max_x,int color,bool flip)
{//        (   起始點   )( 選擇變數 )( 選擇地圖 )(          陣列最大值         )(  顏色  )(上下翻轉)
  byte i,f,g,end=0;
  int color_r=color;
    for(i=0;i<MAP_max_y;i++)
    {
      for(f=0;f<MAP_max_x;f++)
      {
        if(((y+i)<20)&&((y+i)>-1)&&((x+f)<20)&&((x+f)>-1))//在螢幕裡的點
        {
          if(*(MAP+(MAP_Z*MAP_max_y*MAP_max_x)+(i*MAP_max_x)+f))
          {
            for(g=0;g<3;g++)
            {
              if(flip)
              {
                i=MAP_max_y-i-1;
              }
              screen_out[0][g][i+y][f+x]=color%256;
              color=color/256;
              if(flip)
              {
                i=MAP_max_y-i-1;
              }
            }
          }             
        }
        else if(((y+i)<0)||((y+i)>19))
        {
          end=1;
          break;
        }
        else if(((x+f)<0)||((x+f)>19))
        {
          break;
        }
        color=color_r;
      }
      if(end)
      {
        break;
      }
    }
     
}
void paste(byte y,byte x,int *MAP,byte MAP_Z,byte MAP_max_y,byte MAP_max_x,bool flip)
{//        (   起始點   )( 選擇變數 )( 選擇地圖 )(          陣列最大值      )(上下翻轉)
  byte i,f,g,end=0;
  int color=0;
    for(i=0;i<MAP_max_y;i++)
    {
      for(f=0;f<MAP_max_x;f++)
      {
        if(((y+i)<20)&&((y+i)>-1)&&((x+f)<20)&&((x+f)>-1))//在螢幕裡的點
        {
          if(*(MAP+(MAP_Z*MAP_max_y*MAP_max_x)+(i*MAP_max_x)+f)>0)
          {
            color=*(MAP+(MAP_Z*MAP_max_y*MAP_max_x)+(i*MAP_max_x)+f);
            for(g=0;g<3;g++)
            {
              if(flip)
              {
                i=MAP_max_y-i-1;
              }
              screen_out[0][g][i+y][f+x]=color%256;
              color=color/256;
              if(flip)
              {
                i=MAP_max_y-i-1;
              }
            }
          }             
        }
        else if(((y+i)<0)||((y+i)>19))
        {
          end=1;
          break;
        }
        else if(((x+f)<0)||((x+f)>19))
        {
          break;
        }
      }
      if(end)
      {
        break;
      }
    }
     
}
void erase(byte y,byte x,bool mode/*0:單格 1:全部*/)
{
  byte g;//迴圈用
  if(mode)//清除全部畫面
  {
    byte i,f,h;//迴圈用
    for(i=0;i<20;i++)
    {
      for(f=0;f<20;f++)
      {    
        for(g=0;g<3;g++)
        {
          screen_out[0][g][i][f]=0;
        }        
      }
    }
  }
  else//清除指定位置
  {
    for(g=0;g<3;g++)
    {
      screen_out[0][g][y][x]=0;
    }
  }
}
void input_zero(void)//按鈕和蘑菇頭設為未動作
{ 
  byte i;
  for(i=0;i<4;i++)
  {
    keyboard[i]=0;
    Joystick[i]=0;
  }
}
void serial(void)
{
  int i,f;
  /*for(i=0;i<20;i++)
  {
    for(f=0;f<20;f++)
    {
      
    }
    Serial.print("\n");
  }*/
  Serial.print(Joystick_log[0][0][0]);

  Serial.print("\n");
  /*Serial.print("  win1=");
  Serial.print(win1);
  Serial.print("  location[0]=");
  Serial.print(Tetris0.location[0]);
  Serial.print("  location[1]=");
  Serial.print(Tetris0.location[1]);
  Serial.print("  block_choose=");
  Serial.print(Tetris0.block_choose);
  Serial.print("  state[0]=");
  Serial.print(Tetris0.state[0]);
  Serial.print("  state[1]=");
  Serial.print(Tetris0.state[1]);
  Serial.print("  state[2]=");
  Serial.print(Tetris0.state[2]);
  Serial.print("  state[3]=");
  Serial.println(Tetris0.state[3]);
  for(i=0;i<20;i++)
  {
    for(f=0;f<10;f++)
    {
      Serial.print(Tetris0.MAP[0][i][f]);
      Serial.print(Tetris0.MAP[2][i][f]);
    }
    Serial.print("\n");
  }
  Serial.print("\n\n");*/
}