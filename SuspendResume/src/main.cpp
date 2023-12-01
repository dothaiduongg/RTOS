#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Arduino_FreeRTOS.h>
#include <SimpleDHT.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>
#include <semphr.h>
// #include <timers.h> //Thu vien cho software timer

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif



TaskHandle_t task2_t;
SemaphoreHandle_t countingSema;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int pinDHT11 = 12;
SimpleDHT11 dht11;
/* Địa chỉ của DS1307 */
const byte DS1307 = 0x68;
/* Số byte dữ liệu sẽ đọc từ DS1307 */
const byte NumberOfFields = 7;
/* khai báo các biến thời gian */
int second, minute, hour, day, wday, month, year;
#define pin 11  // dinh nghia 1 hang so, o day pin la chan so 6
#define num_led 8 
#define BRIGHTNESS 50 // Set BRIGHTNESS to about 1/5 (max = 255)
Adafruit_NeoPixel strip(num_led, pin, NEO_GRBW + NEO_KHZ800);
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5


#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // Đọc giá trị các nút nhấn

 if (adc_key_in > 1000) return btnNONE; // Giá trị lớn hơn 1000 là không có nút nào được nhấn
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT; 
 

 return btnNONE;  // Nếu không nằm trong khoảng trên thì cũng có nghĩa là không có nút nào được nhấn
}

void DHT11(){
  byte temperature = 0;
  byte humidity = 0;
  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    Serial.print("Read DHT11 failed.");
    return;
  }
  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, "); 
  Serial.print((int)humidity); Serial.println(" %");
  
}

/* Chuyển từ format BCD (Binary-Coded Decimal) sang Decimal */
int bcd2dec(byte num)
{
        return ((num/16 * 10) + (num % 16));
}
/* Chuyển từ Decimal sang BCD */
int dec2bcd(byte num)
{
        return ((num/10 * 16) + (num % 10));
}

/*ColorWipe...........................................................................................................................................................*/
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}
 
void setTime(byte hr, byte min, byte sec, byte wd, byte d, byte mth, byte yr)
{
        Wire.beginTransmission(DS1307);
        Wire.write(byte(0x00)); // đặt lại pointer
        Wire.write(dec2bcd(sec));
        Wire.write(dec2bcd(min));
        Wire.write(dec2bcd(hr));
        Wire.write(dec2bcd(wd)); // day of week: Sunday = 1, Saturday = 7
        Wire.write(dec2bcd(d)); 
        Wire.write(dec2bcd(mth));
        Wire.write(dec2bcd(yr));
        Wire.endTransmission();
} 
void readDS1307()
{
        Wire.beginTransmission(DS1307);
        Wire.write((byte)0x00);
        Wire.endTransmission();
        Wire.requestFrom(DS1307, NumberOfFields);
        
        second = bcd2dec(Wire.read() & 0x7f);
        minute = bcd2dec(Wire.read() );
        hour   = bcd2dec(Wire.read() & 0x3f); // chế độ 24h.
        wday   = bcd2dec(Wire.read() );
        day    = bcd2dec(Wire.read() );
        month  = bcd2dec(Wire.read() );
        year   = bcd2dec(Wire.read() );
        year += 2000;    
}

void initial(){
  Wire.begin();
  pinMode(15,OUTPUT); 
  digitalWrite(15,HIGH);
  pinMode(16,OUTPUT); 
  digitalWrite(16,HIGH);
   
  
  pinMode(5,OUTPUT); 
  
  
  /* cài đặt thời gian cho module */
  setTime(9, 0, 45, 6, 28, 10, 23); // 9:0:45 t7 28-10-2023
}


void LCD(){
 
   lcd.setCursor(0,1);            //Thiết lập vị trí con trỏ
 lcd.print("Phim An :"); // In ra cái mình muốn ghi
Serial.println("lcd on");
 lcd.setCursor(11,1);       
 lcd_key = read_LCD_buttons();  // Đọc nút nhấn

 switch (lcd_key)               // Phần lập trình hiển thị nút được nhấn
 {
   case btnRIGHT:
     {
     lcd.print("RIGHT ");
     vTaskResume(task2_t);
     break;
     }
   case btnLEFT:
     {
     lcd.print("LEFT   ");
     vTaskSuspend(task2_t);
     break;
     }
   case btnUP:
     {
     lcd.print("UP    ");
     break;
     }
   case btnDOWN:
     {
     lcd.print("DOWN  ");
     break;
     }
   case btnSELECT:
     {
     lcd.print("SELECT");
      delay(10);
     break;
     }
     case btnNONE:
     {
     lcd.print("NONE  ");
     break;

     }
     delay(20);
 }

}
void printDigits(int digits){
    // các thành phần thời gian được ngăn chách bằng dấu :
    Serial.print(":");
        
    if(digits < 10)
        Serial.print('0');
    Serial.print(digits);
}

void digitalClockDisplay(){
    // digital clock display of the time
    Serial.print(hour);
    printDigits(minute);
    printDigits(second);
    Serial.print(" ");
    Serial.print(day);
    Serial.print(" ");
    Serial.print(month);
    Serial.print(" ");
    Serial.print(year); 
    Serial.println(); 
}
 
void NEO(){
  colorWipe(strip.Color(255,   0,   0)     , 50); // Red
  colorWipe(strip.Color(  0, 255,   0)     , 50); // Green
  colorWipe(strip.Color(  0,   0, 255)     , 50); // Blue
  colorWipe(strip.Color(  0,   0,   0, 255), 50); // True white (not RGB white)
  Serial.println("Neo on");
}

void task1(void *pvPara){
  while(1){
    LCD();
    vTaskDelay(10);
  }
}

void task2(void *pvPara){
  while(1){
    NEO();
}
}
void task3(void *pvPara){
  while(1){
  readDS1307();
  digitalClockDisplay();
  DHT11();
  }
}
void task3_ver2(void *pvPara){
  while(1){
  delay(1000);
  readDS1307();
  digitalClockDisplay();
  DHT11();
  }
  
}   

void setup()
{
  initial();
  Serial.begin(9600);
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
  #endif
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);
  lcd.begin(16, 2);              // Dùng LCD 16, 2
  lcd.setCursor(0,0);            //Thiết lập vị trí con trỏ
  lcd.print("DIEN TU 3M !"); // In ra cái mình muốn ghi
  xTaskCreate(&task1,"LCD",64,NULL,1,NULL);
  xTaskCreate(&task3_ver2,"RTC + DHT", 150, NULL,1,NULL);
  xTaskCreate(&task2,"NEO LED",150,NULL,1,&task2_t);
}
 
void loop()
{}
