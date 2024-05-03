#include  <Arduino.h>
class game2
{
  int allow_fruit[20][20]={0};//允許水果的位置
  int direction_log[400]={0};//移動紀錄
  int direction_log_in=0;//方向紀錄輸入
  int direction_log_out=0;//方向紀錄輸出
  bool player=0;//玩家幾
  bool player_count=0;//玩家數
  public:
  byte move_time=200;//移動時間
  int fruit[2]={0};//水果位置y,x
  int state[4]={0,0,0,0};//方塊移動時回傳狀態[0]:是否吃到果實 [1]:1:是否觸碰身體(加尾巴) 2:是否觸碰對手身體(加尾巴) [2]:此格是否在螢幕內 [3]:是否觸碰對手頭
  int contral=0;//哪隻搖桿控制
  bool Map[20][20]={0};//y,x 蛇身體的地圖(不包括頭尾)
  int head_location[2]={0};//蛇頭位置y,x
  int tail_location[2]={0};//蛇尾位置y,x
  int direction_last=0;//0~3:上下左右
  int direction=0;//0~3:上下左右
  void initialize(bool player_a,bool player_count_a)//0:玩家0 1:玩家1 ,玩家數0:一人 1:二人 初始化
  {
    head_location[0]=10;
    tail_location[0]=10;
    player=player_a;
    player_count=player_count_a;
    if(player==1)
    {
      head_location[1]=15;
      tail_location[1]=17;
      direction=2;
      direction_log[direction_log_in]=2;
      direction_log_in++;
      direction_log[direction_log_in]=2;
      direction_log_in++;
      Map[10][16]=1;
      direction_last=2;
    }
    else
    {
      head_location[1]=4;
      tail_location[1]=2;
      direction=3;
      direction_log[direction_log_in]=3;
      direction_log_in++;
      direction_log[direction_log_in]=3;
      direction_log_in++;
      Map[10][3]=1;
      direction_last=3;
    }
    direction_log_out=0;
  }
  void head_move()
  {
    direction_log[direction_log_in]=direction;//輸入移動紀錄
    direction_log_in++;//更改記錄輸入
    if(direction_log_in==400)
    {
      direction_log_in=0;
    }
    Map[head_location[0]][head_location[1]]=1;//原本頭位置設為身體
    switch(direction)//移動頭
    {
      case 0://上
      {
        head_location[0]--;
        break;
      }
      case 1://下
      {
        head_location[0]++;
        break;
      }
      case 2://左
      {
        head_location[1]--;
        break;
      }
      case 3://右
      {
        head_location[1]++;
        break;
      }
    }
  }
  void tail_move()
  {   
    switch(direction_log[direction_log_out])//移動尾巴
    {
      case 0://上
      {
        tail_location[0]--;
        break;
      }
      case 1://下
      {
        tail_location[0]++;
        break;
      }
      case 2://左
      {
        tail_location[1]--;
        break;
      }
      case 3://右
      {
        tail_location[1]++;
        break;
      }
    }
    Map[tail_location[0]][tail_location[1]]=0;//尾巴位置消除
    direction_log_out++;//更改記錄輸出
    if(direction_log_out==400)
    {
      direction_log_out=0;
    }
  }
  void new_fruit()//新增果實
  {
    int i,f,r;
    for(i=0;i<20;i++)
    {
      for(f=0;f<20;f++)
      {
        allow_fruit[i][f]=0;
        if(Map[i][f]==0&&!(head_location[0]==i&&head_location[1]==f)&&!(tail_location[0]==i&&tail_location[1]==f))
        {
          allow_fruit[i][f]=1;
        }
      }
    }   
    r=random(400);
    while(allow_fruit[r/20][r%20]==0)
    {
      r=random(400);
    }
    fruit[0]=r/20;
    fruit[1]=r%20;
  }
  void new_fruit(bool *Map1/*玩家1的身體地圖*/,int head_location1[2]/*玩家1蛇頭位置y,x*/,int tail_location1[2]/*玩家1蛇尾位置y,x*/)//新增果實(雙人模式)
  {
    int i,f,r;
    for(i=0;i<20;i++)
    {
      for(f=0;f<20;f++)
      {
        allow_fruit[i][f]=0;
        if(Map[i][f]==0&&Map1[i*20+f]==0&&!(head_location[0]==i&&head_location[1]==f)&&!(head_location1[0]==i&&head_location1[1]==f)&&!(tail_location[0]==i&&tail_location[1]==f)&&!(tail_location1[0]==i&&tail_location1[1]==f))
        {
          allow_fruit[i][f]=1;
        }
      }
    }   
    r=random(400);
    while(allow_fruit[r/20][r%20]==0)
    {
      r=random(400);
    }
    fruit[0]=r/20;
    fruit[1]=r%20;
  }
  void judge()//判斷狀態
  {
    int i,f;
    for(i=0;i<4;i++)//狀態歸零
    {
      state[i]=0;
    }
    if(head_location[0]>19)//碰到下邊界
    {
      state[2]=1;
    }
    else if(head_location[0]<0)//碰到上邊界
    {
      state[2]=1;
    }
    if(head_location[1]>19)//碰到右邊界
    {
      state[2]=1;
    }
    else if(head_location[1]<0)//碰到左邊界
    {
      state[2]=1;
    }
    if((state[2]==0)&&(Map[head_location[0]][head_location[1]]==1))//碰到自己的身體
    {
      state[1]=1;
    }
    if((state[2]==0)&&(head_location[0]==fruit[0])&&(head_location[1]==fruit[1]))//吃到果實
    {
      state[0]=1;
    }
  }
  void judge(bool *Map1,int head_y,int head_x,int fruit_y,int fruit_x)//判斷狀態(雙人模式)
  {
    int i,f;
    for(i=0;i<4;i++)//狀態歸零
    {
      state[i]=0;
    }
    if(head_location[0]>19)//碰到下邊界
    {
      state[2]=1;
    }
    else if(head_location[0]<0)//碰到上邊界
    {
      state[2]=1;
    }
    if(head_location[1]>19)//碰到右邊界
    {
      state[2]=1;
    }
    else if(head_location[1]<0)//碰到左邊界
    {
      state[2]=1;
    }
    if((state[2]==0)&&(Map[head_location[0]][head_location[1]]==1))//碰到自己的身體
    {
      state[1]=1;
    }
    else if((state[2]==0)&&(Map1[head_location[0]*20+head_location[1]]==1))//碰到對手的身體
    {
      state[1]=2;
    }
    if((state[2]==0)&&(head_location[0]==head_y)&&(head_location[1]==head_x))//碰到對手的頭
    {
      state[3]=1;
    }
    if((state[2]==0)&&(head_location[0]==fruit_y)&&(head_location[1]==fruit_x))//吃到果實
    {
      state[0]=1;
    }
  }
  void end()//遊戲結束數據刪除
  {
    int i,f;
    for(i=0;i<4;i++)
    {
      state[i]=0;
    }
    fruit[0]=0;
    fruit[1]=0;
    direction_log_in=0;
    direction_log_out=0;
    for(i=0;i<20;i++)
    {
      for(f=0;f<20;f++)
      {
        direction_log[i*20+f]=0;
        Map[i][f]=0;
      }
    }
  }
};
