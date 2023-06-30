#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// inisialisasi pin lcd
LiquidCrystal_I2C lcd(0x27, 20, 4);

// inisialisasi pin suhu
#define DHTPIN 15  
#define DHTTYPE DHT11   

// inisialisasi pin sensor arus (pin 33,25,26)
#define arus_satu 33 
#define arus_dua 25
#define arus_tiga 26

// inisialisasi pin sensor tegangan (pin 34,35,32)
#define tegangan_satu 34
#define tegangan_dua 35 
#define tegangan_tiga 32

// inisialisasi pin kontrol buck boost 
#define pin_buck 23

DHT dht(DHTPIN, DHTTYPE);
float sensitivitas = 66;
float ACSoffset = 1650;

float R1 = 30000.0;
float R2 = 7500.0;

// membuat setpoit tegangan 
float setpointtegangan = 14.4;
float voltagetolerance = 3.0;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  // pin modes sensor arus 
  pinMode(arus_satu, INPUT);
  pinMode(arus_dua, INPUT);
  pinMode(arus_tiga, INPUT);
  // pin mode sensor tegangan 
  pinMode(tegangan_satu, INPUT);
  pinMode(tegangan_dua, INPUT);
  pinMode(tegangan_tiga, INPUT);
  // pin mode sensor suhu
  dht.begin();
}

// baca nilai adc 
float baca_nilai_adc(int pin){
  int nilaiADC = analogRead(pin);
  return nilaiADC;
}

// baca sensor arus 1
float baca_nilai_arus1(int pin) {
  int nilaiArus = analogRead(pin);
  float Vsensor = (nilaiArus / 4096.00) * 3300.00;
  float hasil = (1.54 * ((((Vsensor - ACSoffset) / sensitivitas) + 2)) + 0.1837391);
  return hasil;
}

// baca sensor arus 2 
float baca_nilai_arus2(int pin){
  int nilaiArus = analogRead(pin);
  float Vsensor = (nilaiArus / 4096.00) * 3300.00;
  float hasil = (1.61 * ((((Vsensor - ACSoffset) / sensitivitas) + 2)) - 0.3927025);
  return hasil;
}

// baca sensor arus 3 
float baca_nilai_arus3(int pin){
  int nilaiArus = analogRead(pin);
  float Vsensor = (nilaiArus / 4096.00) * 3300.00;
  float hasil = (1.54 * ((((Vsensor - ACSoffset) / sensitivitas) + 2)) - 0.4091332);
  return hasil;
}

// baca sensor tegangan 1
float baca_nilai_tegangan1(int pin){
  int nilaiTegangan = analogRead(pin);
  float Vsensor = nilaiTegangan*(3.3 / 4095.0);
  float hasil = (1.01*(Vsensor / (R2/(R1+R2))+0.6555551));
  return hasil; 
}

// baca sensor tegangan 2 
float baca_nilai_tegangan2(int pin){
  int nilaiTegangan = analogRead(pin);
  float Vsensor = nilaiTegangan*(3.3 / 4095.0);
  float hasil = (1.01*(Vsensor / (R2/(R1+R2))+0.6658566));
  return hasil;
}

// baca sensor tegangan 3 
float baca_nilai_tegangan3(int pin){
  int nilaiTegangan = analogRead(pin);
  float Vsensor = nilaiTegangan*(3.3 / 4095.0);
  float hasil = (1.01*(Vsensor / (R2/(R1+R2))+0.6628513));
  return hasil;
}

// float baca sensor suhu
float baca_sensor_suhu(){
  float celcius = dht.readTemperature();
  return celcius;
}

// float fuzzy 
// 1. Mencari fungsi keanggotaan 

void loop() {
  // pembacaan sensor arus dan adc 
  float arus1 = baca_nilai_arus1(arus_satu);
  float arus2 = baca_nilai_arus2(arus_dua);
  float arus3 = baca_nilai_arus3(arus_tiga);

  float nilaiarusADC1 = baca_nilai_adc(arus_satu);
  float nilaiarusADC2 = baca_nilai_adc(arus_dua);
  float nilaiarusADC3 = baca_nilai_adc(arus_tiga);

  // pembacaan sensor tegangan dan adc 
  float tegangan1 =  baca_nilai_tegangan1(tegangan_satu);
  float tegangan2 =  baca_nilai_tegangan2(tegangan_dua);
  float tegangan3 =  baca_nilai_tegangan3(tegangan_tiga);

  float nilaiteganganADC1 = baca_nilai_adc(tegangan_satu);
  float nilaiteganganADC2 = baca_nilai_adc(tegangan_dua);
  float nilaiteganganADC3 = baca_nilai_adc(tegangan_tiga);

  // pembacaan sensor suhu
  float suhu = baca_sensor_suhu();

  // tampilkan sensor arus pada lcd 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("I1: ");      
  lcd.print(arus1);
  lcd.print("A");
  lcd.print (" ADC1 ");
  lcd.print (nilaiarusADC1);
  lcd.setCursor(0, 1);
  lcd.print("I2: ");      
  lcd.print(arus2);
  lcd.print("A");
  lcd.print (" ADC2 ");
  lcd.print (nilaiarusADC2);
  lcd.setCursor(0, 2);
  lcd.print("I3: ");      
  lcd.print(arus3);
  lcd.print("A");
  lcd.print (" ADC3 ");
  lcd.print (nilaiarusADC3);
  delay(1000);

  // tampilkan sensor tegangan pada lcd
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V1: ");      
  lcd.print(tegangan1);
  lcd.print("V");
  lcd.print (" ADC1 ");
  lcd.print (nilaiteganganADC1);
  lcd.setCursor(0, 1);
  lcd.print("V2: ");      
  lcd.print(tegangan2);
  lcd.print("V");
  lcd.print (" ADC2 ");
  lcd.print (nilaiteganganADC2);
  lcd.setCursor(0, 2);
  lcd.print("V3: ");      
  lcd.print(tegangan3);
  lcd.print("V");
  lcd.print (" ADC3 ");
  lcd.print (nilaiteganganADC3);
  delay(1000);

  // tampilkan sensor suhu pada lcd 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");      
  lcd.print(tegangan1);
  lcd.print("suhu");
  delay(1000);
}

