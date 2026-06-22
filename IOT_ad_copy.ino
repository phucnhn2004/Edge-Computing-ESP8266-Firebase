#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>  // Thư viện bảo mật tiêu chuẩn thay thế FirebaseArduino
#include <ThingSpeak.h> // Gui du lieu led web thingspeak
#include <WiFiUdp.h> // LIB use for realtime
#include <NTPClient.h> // LIB use for realtime
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h" 
#define DHTTYPE DHT11
LiquidCrystal_I2C lcd(0x27,16,2);
#define Led_test D0
#define BT1 D3
#define BT2 D5
#define BT3 D6
#define Pump_Machine D8
#define Rain_Sensor D7
#define Light_Sensor D1 
#define Led_Light D2    
#define dht_dpin 2 // D4
#define humi_pin A0

DHT dht(dht_dpin, DHTTYPE);

// ===================== ĐỒNG BỘ THÔNG TIN ĐƯỜNG TRUYỀN HOTSPOT CỦA BẠN =====================
#define WIFI_SSID "tp-link"        
#define WIFI_PASSWORD "anhvu6868"  

#define FIREBASE_HOST "doantotnghiep-e96e5-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "I3SZl8B5uqWY276ByhBDxcn5iCIpcH98aov3Hf4W" 
// ==================================================================================

const char *server = "api.thingspeak.com";
unsigned long myChannelNumber = 1236409;
const char * myWriteAPIKey = "NZLCGEGOCP6TD5AB";
float temp_c,old_temp = 0 ;
float humidity, old_humi = 0 ;
unsigned long time_check = 0,time_send_data = 0 ;
WiFiClient client;

//Declare
float temp_point = 0 ,humi_point = 0 ;
signed int hrs_point1,minu_point1;
signed int hrs_point2,minu_point2;
signed int hrs_point3,minu_point3;
signed int time_start =0, time_finish = 0;
signed int time_turn_on = 2 ;
signed int check_box1=0,check_box2=0,check_box3=0;

String real_time;
signed int sec,minu,hrs;
long time_process = 0;
unsigned long time_get_fb = 0;
int display_time=0,process_control_sendata= 0,process_control = 0 ;
boolean flag_turn_on_pump = 0;
boolean flag_send_humi = 0, flag_send_temp = 0, flag_send_data = 0,old_flag_pump = 0;
boolean flag_follow_condition = 0;
int flag_check_rain_ss = 2,old_flag_rain = 5, flag_check_time = 0 ;
String convert_send;
String data;
unsigned long count_average = 0;
float AverageHumi_Day = 0 ,AverageHumi_Month = 0 ,AverageTemp_Day = 0 ,AverageTemp_Month = 0 ;
unsigned long SumHumi_Day = 0, SumHumi_Month = 0, SumTemp_Day =0, SumTemp_Month = 0;
boolean flag_send_average = 0 ;
int process_control_send_average = 0;

int last_BT2_state = HIGH;
int last_BT3_state = HIGH;

WiFiUDP u;
NTPClient n(u,"3.vn.pool.ntp.org",7*3600); 

void blink_led(byte number);
void display_actual();
void display_setpoint();
int read_humi();
void convert_time_h_m(String get_time,int *_hour,int *_minute);
void get_real_time(int _timeout,int *_hour,int *_minute,int *_sec);
void get_firebase();
void control_pump();
void check_conditions_on_pump();
void check_conditions_off_pump();
void check_rain_sensor();
void calculator_average_data();
void ini_process();
void auto_process();
void manual_process();

String clean_quotes(String str) {
  str.replace("\"", "");
  str.replace("\\", ""); 
  str.trim();
  return str;
}

String get_json_value(String json, String key) {
  int keyIndex = json.indexOf("\"" + key + "\"");
  if (keyIndex == -1) return "";
  int colonIndex = json.indexOf(":", keyIndex);
  if (colonIndex == -1) return "";
  
  int startIndex = colonIndex + 1;
  while (startIndex < json.length() && (json.charAt(startIndex) == ' ' || json.charAt(startIndex) == '\r' || json.charAt(startIndex) == '\n')) {
    startIndex++;
  }
  
  int endIndex = json.indexOf(",", startIndex);
  if (endIndex == -1) endIndex = json.indexOf("}", startIndex);
  
  String val = json.substring(startIndex, endIndex);
  val.trim();
  return val;
}

String firebase_get_data(String path) {
  WiFiClientSecure secure_client;
  secure_client.setInsecure(); 
  HTTPClient http;
  String url = "https://" + String(FIREBASE_HOST) + path + ".json?auth=" + String(FIREBASE_AUTH);
  if (http.begin(secure_client, url)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      http.end();
      return payload;
    }
    http.end();
  }
  return "";
}

void firebase_set_string(String path, String value) {
  WiFiClientSecure secure_client;
  secure_client.setInsecure();
  HTTPClient http;
  String url = "https://" + String(FIREBASE_HOST) + path + ".json?auth=" + String(FIREBASE_AUTH);
  if (http.begin(secure_client, url)) {
    http.addHeader("Content-Type", "application/json");
    String body = "\"" + value + "\"";
    http.PUT(body);
    http.end();
  }
}

void firebase_set_float(String path, float value) {
  WiFiClientSecure secure_client;
  secure_client.setInsecure();
  HTTPClient http;
  String url = "https://" + String(FIREBASE_HOST) + path + ".json?auth=" + String(FIREBASE_AUTH);
  if (http.begin(secure_client, url)) {
    http.addHeader("Content-Type", "application/json");
    http.PUT(String(value));
    http.end();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(Led_test, OUTPUT);
  pinMode(Pump_Machine, OUTPUT);
  pinMode(BT1, INPUT_PULLUP);
  pinMode(BT2, INPUT_PULLUP);
  pinMode(BT3, INPUT_PULLUP);
  pinMode(Light_Sensor, INPUT);   
  pinMode(Led_Light, OUTPUT);     

  lcd.begin(); 
  lcd.backlight();
  lcd.print("Hello world    ");
  dht.begin();
  if (digitalRead(BT3)== 0)
  {
    process_control = 5; 
    digitalWrite(Led_test,1);
  }
  else
  {
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED)
    {
      lcd.setCursor(0,1);
      lcd.print("Connecting Wifi       ");
      Serial.print(".");
      delay(500);
      process_control++;
      if (process_control == 15)
      {
        break;
      }
    }
  if ( process_control <15)
  {
    process_control = 0;
    lcd.setCursor(0,1);
    lcd.print("Connected           ");
    Serial.println();
    Serial.print("connected: ");
    Serial.println(WiFi.localIP());
    
    ThingSpeak.begin(client);
    n.begin();
    n.update();
    real_time= n.getFormattedTime();
    blink_led(4);
  }
  else
  {
  lcd.setCursor(0,0);
  lcd.print("Can not connect");
    lcd.setCursor(0,1);
  lcd.print("internet !!!        ");
  
  digitalWrite(Led_test,1);
  delay(2000);
  lcd.clear();
  process_control = 5;
  }   
  }
}

void loop() 
{
  switch (process_control)
  {
    case 0:
      ini_process();
      break;
    case 4:
      auto_process();
      break;
    case 5:
      manual_process();
      break;
  }
}

void blink_led(byte number)
{
  for (int i = 0 ; i<number;i++ )
  {
    digitalWrite(Led_test,1);
    delay(200);
    digitalWrite(Led_test,0);
    delay(200);
  }
}
void display_actual()
{
  lcd.setCursor(0,0);
  lcd.print("Actual:"); 
  lcd.print(hrs);
  lcd.print(":");
  lcd.print(minu);
  lcd.print("        ");

  lcd.setCursor(0,1);
  lcd.print("T=");
  lcd.print((int)temp_c);
  lcd.print("*C-");
  lcd.print("H=");
  lcd.print((int)humidity);
  lcd.print("%      ");
}

void display_setpoint()
{
  if (check_box1 == 1)
  {
    lcd.setCursor(0,0);
    lcd.print("SetTime:"); 
    lcd.print(hrs_point1);
    lcd.print(":");
    lcd.print(minu_point1);
    lcd.print("     ");
  }
  else
  {
    if (check_box2 == 1)
    {
      lcd.setCursor(0,0);
      lcd.print("SetTime:"); 
      lcd.print(hrs_point2);
      lcd.print(":");
      lcd.print(minu_point2);
      lcd.print("     ");
    }
    else
    {
      if (check_box3 == 1)
      {
        lcd.setCursor(0,0);
        lcd.print("SetTime:"); 
        lcd.print(hrs_point3);
        lcd.print(":");
        lcd.print(minu_point3);
        lcd.print("     ");
      }
      else
      {
        lcd.setCursor(0,0);
        lcd.print("!! Not set Time       ");
      }
    }
  }
  
  lcd.setCursor(0,1);
  lcd.print("Set:T=");
  lcd.print((int)temp_point);
  lcd.print("*C-");
  lcd.print("H=");
  lcd.print((int)humi_point);
  lcd.print("%      ");  
}

void convert_time_h_m(String get_time,int *_hour,int *_minute)
{
  byte moc[3];byte count_moc=0;
  String chuoi1, chuoi2;
  get_time = clean_quotes(get_time); 
  for (int i = 0 ; i<get_time.length();i++)
  {
    if (get_time.charAt(i) == ':')
    {
      moc[count_moc] = i;
      count_moc++;
    }  
  }
  chuoi1 = get_time; 
  chuoi2 = get_time; 
  chuoi1.remove(moc[0]);
  chuoi2.remove(0,moc[0]+1);
  *_hour = chuoi1.toInt();
  *_minute = chuoi2.toInt();
}

void get_real_time(int _timeout,int *_hour,int *_minute,int *_sec)
{
   if (millis()- time_process > _timeout)
  {
  time_process = millis();
  n.update();
  real_time= n.getFormattedTime();
  byte moc[3];byte count_moc=0;
  String chuoi1, chuoi2,chuoi3;
  for (int i = 0 ; i< real_time.length();i++)
  {
    if (real_time.charAt(i) == ':')
    {
      moc[count_moc] = i;
      count_moc++;
    }  
  }
  chuoi1 = real_time;
  chuoi2 = real_time;
  chuoi3 = real_time;
  chuoi1.remove(moc[0]); 
  chuoi2.remove(0,moc[0]+1);
  chuoi3.remove(0,moc[1]+1);
  *_hour = chuoi1.toInt();
  *_minute = chuoi2.toInt();
  *_sec = chuoi3.toInt();
  }
}

void get_firebase() 
{ 
  if (millis() - time_get_fb > 1000) 
  {
    time_get_fb = millis();
    String payload = firebase_get_data("/GetData");
    if (payload == "") {
      Serial.println("Firebase pull failed");
      return;
    }
    
    String pump_data = clean_quotes(get_json_value(payload, "pump"));
    if (pump_data == "0") flag_turn_on_pump = 0 ;
    if (pump_data == "1") flag_turn_on_pump = 1;
    
    humi_point = clean_quotes(get_json_value(payload, "SetHumi")).toFloat();
    temp_point = clean_quotes(get_json_value(payload, "SetTemp")).toFloat();
    
    convert_time_h_m(get_json_value(payload, "SetTime1"), &hrs_point1, &minu_point1);
    convert_time_h_m(get_json_value(payload, "SetTime2"), &hrs_point2, &minu_point2);
    convert_time_h_m(get_json_value(payload, "SetTime3"), &hrs_point3, &minu_point3);
    
    check_box1 = clean_quotes(get_json_value(payload, "CheckBox1")).toInt();
    check_box2 = clean_quotes(get_json_value(payload, "CheckBox2")).toInt();
    check_box3 = clean_quotes(get_json_value(payload, "CheckBox3")).toInt();
  }
}

void control_pump()
{
  // SỬA ĐỔI TUYỆT ĐỐI: ĐƯA LÊN ĐẦU HÀM. Nếu có mưa, ghim chặt trạng thái về 0 ngay lập tức!
  if (flag_check_rain_ss == 1)
  {
    flag_turn_on_pump = 0;
  }

  if (old_flag_pump == flag_turn_on_pump)
  {}
  else
  {
    old_flag_pump = flag_turn_on_pump;
    convert_send = String(old_flag_pump);
    firebase_set_string("/PumpStatus", convert_send);
    delay(50);
    firebase_set_string("/PumpStatus", convert_send);
    if (old_flag_pump == 1)
    {
      digitalWrite(Pump_Machine, HIGH);
      digitalWrite(Led_test, HIGH);
    }
    if (old_flag_pump == 0)
    {
      digitalWrite(Pump_Machine, LOW);
      digitalWrite(Led_test, LOW);
    }   
  }
}

void check_conditions_on_pump()
{
  if (check_box1 == 1 )
  {
    if (hrs == hrs_point1 && minu == minu_point1 && sec <10) flag_check_time = 1;
  }
  if (check_box2 == 1 )
  {
    if (hrs == hrs_point2 && minu == minu_point2 && sec <10) flag_check_time = 1;
  }
  if (check_box3 == 1 )
  {
    if (hrs == hrs_point3 && minu == minu_point3 && sec <10) flag_check_time = 1;
  }
  
  if (flag_check_time == 1)
  {
    if (temp_c >temp_point || humidity < humi_point)
    {
      firebase_set_string("/GetData/pump", "1");
      flag_turn_on_pump = 1;
      time_start = minu;
      time_finish = time_start + time_turn_on;
      if (time_finish >=60)time_finish = time_finish - 60;
      flag_follow_condition = 1;
    } 
    flag_check_time = 0;
  }
}

void check_conditions_off_pump()
{
  if (flag_follow_condition == 1)
  {
    if(minu == time_finish)
    {
      firebase_set_string("/GetData/pump", "0");
      flag_turn_on_pump = 0;
      flag_follow_condition = 0;
    }
  }
}

void check_rain_sensor()
{
  if(digitalRead(Rain_Sensor) == 0) 
  {
    flag_check_rain_ss = 1;
    flag_turn_on_pump = 0;
  }
  else
  {
    flag_check_rain_ss = 0;
  }
  if (flag_check_rain_ss != old_flag_rain)
  {
    old_flag_rain = flag_check_rain_ss;
    convert_send = String(old_flag_rain);
    firebase_set_string("/RainSensor1", convert_send);
  }
}

void calculator_average_data()
{
  SumTemp_Day = SumTemp_Day + temp_c;
  SumHumi_Day = SumHumi_Day + humidity;
  count_average ++;
  if (hrs == hrs_point3 && minu == minu_point3 && flag_send_average == 0 && sec < 30)
  {   
    flag_send_average = 1;
    AverageTemp_Day =(float) SumTemp_Day / count_average;
    AverageHumi_Day =(float) SumHumi_Day / count_average;
  }
  if (flag_send_average == 1)
  {
    process_control_send_average++;
    switch(process_control_send_average)
       {
        case 1:
          ThingSpeak.writeField(myChannelNumber, 3, AverageTemp_Day, myWriteAPIKey);
          break;
        case 2:
          firebase_set_float("/AverageTemp_Day", AverageTemp_Day);
          break;
        case 3: 
          ThingSpeak.writeField(myChannelNumber, 4, AverageHumi_Day, myWriteAPIKey);
           break;
        case 4: 
          firebase_set_float("/AverageHumi_Day", AverageHumi_Day);               
          break;
       }
    if(process_control_send_average == 4)
    {
      process_control_send_average=0;  
      flag_send_average = 0;
      SumTemp_Day = 0 ;
      SumHumi_Day = 0 ;
      count_average = 0 ;
    }   
  }
}

void ini_process() 
{
  String payload = firebase_get_data("/GetData");
  lcd.setCursor(0,0);
  lcd.print("Waitting              "); 
  lcd.setCursor(0,1);
  lcd.print("Ini Process ....      "); 
  
  if (payload != "") {
    check_box1 = clean_quotes(get_json_value(payload, "CheckBox1")).toInt();  delay(50);
    check_box2 = clean_quotes(get_json_value(payload, "CheckBox2")).toInt();  delay(50);
    check_box3 = clean_quotes(get_json_value(payload, "CheckBox3")).toInt();  delay(50);
    
    humi_point = clean_quotes(get_json_value(payload, "SetHumi")).toFloat();  delay(50);
    temp_point = clean_quotes(get_json_value(payload, "SetTemp")).toFloat();  delay(50);
   
    data = clean_quotes(get_json_value(payload, "pump"));
    if (data == "0") flag_turn_on_pump = 0 ;
    if (data == "1") flag_turn_on_pump = 1;
    delay(50);  
    
    convert_time_h_m(get_json_value(payload, "SetTime1"), &hrs_point1, &minu_point1);   delay(50); 
    convert_time_h_m(get_json_value(payload, "SetTime2"), &hrs_point2, &minu_point2);   delay(50);
    convert_time_h_m(get_json_value(payload, "SetTime3"), &hrs_point3, &minu_point3);
  }
  
  process_control = 4; 
  temp_c = dht.readTemperature();
  if(isnan(temp_c))
  {
    delay(100);
    if(isnan(temp_c))
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Kiem tra cam        ");
      lcd.setCursor(0,1);
      lcd.print("bien nhiet do     ");
      delay(2000);
      process_control = 5;      
    }
  }
  old_temp = temp_c;
}

void auto_process() 
{
  get_real_time(10000,&hrs, &minu,&sec);
  
  // ĐÃ SỬA: Đọc cảm biến mưa liên tục mỗi chu kỳ loop để phản hồi ngay lập tức khi giọt nước chạm mạch
  check_rain_sensor();

  if (millis() - time_check > 8000)
  {
    time_check = millis();
    temp_c = dht.readTemperature();
    humidity = read_humi();
    if(isnan(temp_c)) temp_c = old_temp;
      
    if (digitalRead(Light_Sensor) == 0) {
        digitalWrite(Led_Light, HIGH);
    } else {
        digitalWrite(Led_Light, LOW);
    }
      
    if((temp_c<100) && (temp_c > -5) )
    {
      firebase_set_float("/ActualTemp1", temp_c);
      firebase_set_float("/ActualHumi1", humidity);
      
      ThingSpeak.setField(1, temp_c);
      ThingSpeak.setField(2, humidity);
      ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      
      old_temp = temp_c;
      old_humi = humidity;
      calculator_average_data();
    }
  }
  get_firebase();
  
  // ĐÃ SỬA (ĐỒNG BỘ ĐÁM MÂY): Nếu trời mưa mà nút trên App vẫn bật "1", ép trả về "0" ngay để tắt nút trên App điện thoại
  if (flag_check_rain_ss == 1 && flag_turn_on_pump == 1) 
  {
    flag_turn_on_pump = 0;
    firebase_set_string("/GetData/pump", "0");
  }
  
  display_time++;
  if (display_time<60) display_actual();
  if (display_time>=60)
  {
    display_setpoint();
    if (display_time>=120)display_time=0;
  }
  
  int current_BT2 = digitalRead(BT2);
  if (current_BT2 == 0 && last_BT2_state == 1) 
  {
    flag_turn_on_pump = 1;
    firebase_set_string("/GetData/pump", "1");
  }
  last_BT2_state = current_BT2;

  int current_BT3 = digitalRead(BT3);
  if (current_BT3 == 0 && last_BT3_state == 1) 
  { 
    flag_turn_on_pump = 0;
    firebase_set_string("/GetData/pump", "0");
  }
  last_BT3_state = current_BT3;
  
  check_conditions_on_pump();
  check_conditions_off_pump();
  control_pump(); 
}

void manual_process()
{
  lcd.setCursor(0,0);
  lcd.print("Manual Mode          ");
  if (millis() - time_check >2000)
  {
    time_check = millis(); 
    temp_c = dht.readTemperature();
    humidity = read_humi();
    lcd.setCursor(0,1);
    lcd.print("H:");
    lcd.print((int)humidity);
    lcd.print("%-");
    lcd.print((int)temp_c);
    lcd.print("*C          ");

    digitalWrite(Led_test,flag_turn_on_pump);
    flag_turn_on_pump = !flag_turn_on_pump;
  }
  if (digitalRead(BT2) == 0) digitalWrite(Pump_Machine,HIGH);
  if (digitalRead(BT3) == 0 ) digitalWrite(Pump_Machine,LOW);  
}

int read_humi()
{
  int kq;
  kq = analogRead(humi_pin);
  kq = map(kq,100,400,100,0);
  return kq;
}