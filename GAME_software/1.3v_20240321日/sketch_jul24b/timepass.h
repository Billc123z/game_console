#include  <Arduino.h>
class timep
{ 
  int i;
  unsigned int timepass[8]={0};
  unsigned int stop[8]={0};
  public:    	    
    unsigned long time_pass(bool mode=0/*0:提取經過時間 1:開始計時* 2:暫停計時 3:繼續計時*/ ,int group=0/*選擇第幾個紀錄*/)
    {
      if(mode==1)
      {
        timepass[group]=millis();
        stop[group]=0;
      }
      else if(mode==0)
      {
        if(stop[group]==0)//如果此時沒有暫停
        {
          return millis()-timepass[group];
        }
        else if(stop[group]==1)//如果此時暫停
        {
          return timepass[group];
        }     
      }
      else if(mode==2&&stop[group]==0)//暫停
      {
        stop[group]=1;
        timepass[group]=millis()-timepass[group];
      }
      else if(mode==3&&stop[group]==1)//繼續
      {
        stop[group]=0;
        timepass[group]=millis()-timepass[group];
      }
    }
};