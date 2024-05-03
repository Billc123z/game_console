#include <SPIFFS.h>
#include  <Arduino.h>
class Spiffs
{
  public:
    String message;//輸入文字
    String file;//輸入檔案名
    void scanFile()//搜尋檔案
    {
      File root = SPIFFS.open("/");
      File file = root.openNextFile();
      while(file)
      {
        Serial.printf("file:%s\n",file.name());
        file = root.openNextFile();
        
      }
    }
    String readFile(String fileLocation_string, unsigned int count/*(0:全部資料 >0:第幾個數據*/)
    {
      const char* fileLocation = fileLocation_string.c_str();
      String readFsMessage="";

      File file = SPIFFS.open(fileLocation,"r"); //"r" open and read a file
      if(!file)
      {
        Serial.println("開啟檔案失敗");
        return readFsMessage;
      }
      Serial.printf("檔案大小:%u Bytes\n", file.size());
      Serial.printf("file.available():%d\n",file.available());
      if(count==0)//全部資料
      {
        readFsMessage = file.readStringUntil('\0');
      }
      else//讀取一筆資料
      {
        while(file.available()&&count!=0)
        {
          readFsMessage = file.readStringUntil('\n');
          count--;
        }
      }
      file.close();
      Serial.println(readFsMessage);
      return readFsMessage;
    }
    String readFile(String fileLocation_string, unsigned int count/*(0:全部資料 >0:第幾個數據*/, String* Front, String* back)
    {
      const char* fileLocation = fileLocation_string.c_str();
      String readFsMessage="";
      File file = SPIFFS.open(fileLocation,"r"); //"r" open and read a file
      if(!file)
      {
        Serial.println("開啟檔案失敗");
        return readFsMessage;
      }
      Serial.printf("檔案大小:%u Bytes\n", file.size());
      Serial.printf("file.available():%d\n",file.available());
      if(count==0)//全部資料
      {
        readFsMessage = file.readStringUntil('\0');
      }
      else//讀取一筆資料
      {
        while(file.available()&&count!=0)
        {
          readFsMessage = file.readStringUntil('\n');
          if(count>1)
          {
            *Front+=readFsMessage+String('\n');
          }
          count--;
        }
        while(file.available())
        {
          *back=file.readStringUntil('\n');
        }   
      }
      file.close();
      Serial.println(readFsMessage);
      return readFsMessage;
    }
    void writeFile(String fileLocation_string, String write_message_string, int count/*(0:全部資料重寫 >0:第幾個資料重寫*/)//寫入資料
    {
      const char* fileLocation = fileLocation_string.c_str();
      if(count==0)
      {
        const char* write_message = write_message_string.c_str();
        SPIFFS.remove(fileLocation);
        File file = SPIFFS.open(fileLocation, "w"); // open and write to file
        if(!file)
        {
          Serial.println("寫入失敗");
          return;
        }
        file.println(write_message);
        file.close();
        Serial.printf("write down \n%s\n",write_message);
      }
      else
      {
        String Front="",back="",amount="";
        readFile(fileLocation_string,count,&Front,&back);
        amount=Front+write_message_string+String('\n')+back;
        amount.trim();
        const char* write_message = amount.c_str();
        File file = SPIFFS.open(fileLocation, "w"); // open and write to file
        if(!file)
        {
          Serial.println("寫入失敗");
          return;
        }
        file.println(write_message);
        file.close();
        Serial.printf("write down \n%s\n",write_message);
      }
    }
    void init_SPIFFS()//初始化
    {
      if(!SPIFFS.begin(true))
      {
        Serial.println("SPIFFS 掛載失敗!");
      }
      Serial.println("SPIFFS 掛載成功!");
    }
};  
/*void setup() {


  writeFile(ssid_fs,ssid );

  //SPIFFS.remove(ssid_fs);
  Serial.print("讀取內容:");
  Serial.println(readFile(ssid_fs));
  scanFile();

}*/

