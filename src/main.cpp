#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//inisialisasi pin lcd
LiquidCrystal_I2C lcd(0x27, 20, 4);

//inisialisasi pin suhu
#define DHTPIN 15  
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

//inisialisasi pin sensor arus (pin 33,25,26)
#define arus_satu 33 
#define arus_dua 25
#define arus_tiga 26

//inisialisasi pin sensor tegangan (pin 34,35,32)
#define tegangan_satu 34
#define tegangan_dua 35 
#define tegangan_tiga 32

// inisialisasi pin relay
const int relayPin1 = 27;
const int relayPin2 = 14;
const int relayPin3 = 12;
const int relayPin4 = 13;

//inisialisasi pin kontrol buck boost 
#define pin_buck 23

// membuat setpoint untuk menghitung arus dan tegangan 
float sensitivitas = 66;
float ACSoffset = 1650;
float R1 = 30000.0;
float R2 = 7500.0;

//membuat setpoit tegangan 
float setpointtegangan = 14.4;
float previousError = 0;
float setpreviousVoltage = 0.0;

// fuzzifikasi error 
float Enegative_big;
float Enegative_middle;
float Enegative_small;
float E_zero;
float Epositif_small;
float Epositif_middle;
float Epositif_big;

// fuzzifikasi delta error 
float DEnegative_big;
float DEnegative_middle;
float DEnegative_small;
float DE_zero;
float DEpositif_small;
float DEpositif_middle;
float DEpositif_big;

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

  // pin mode relay 
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);
}

//baca nilai adc 
float baca_nilai_adc(int pin){
  int nilaiADC = analogRead(pin);
  return nilaiADC;
}

//baca sensor arus 1
float baca_nilai_arus1(int pin) {
  int nilaiArus = analogRead(pin);
  float Vsensor = (nilaiArus / 4096.00) * 3300.00;
  float hasil = (1.54 * ((((Vsensor - ACSoffset) / sensitivitas) + 2)) + 0.1837391);
  return hasil;
}

//baca sensor arus 2 
float baca_nilai_arus2(int pin){
  int nilaiArus = analogRead(pin);
  float Vsensor = (nilaiArus / 4096.00) * 3300.00;
  float hasil = (1.61 * ((((Vsensor - ACSoffset) / sensitivitas) + 2)) - 0.3927025);
  return hasil;
}

//baca sensor arus 3 
float baca_nilai_arus3(int pin){
  int nilaiArus = analogRead(pin);
  float Vsensor = (nilaiArus / 4096.00) * 3300.00;
  float hasil = (1.54 * ((((Vsensor - ACSoffset) / sensitivitas) + 2)) - 0.4091332);
  return hasil;
}

//baca sensor tegangan 1
float baca_nilai_tegangan1(int pin){
  int nilaiTegangan = analogRead(pin);
  float Vsensor = nilaiTegangan*(3.3 / 4095.0);
  float hasil = (1.01*(Vsensor / (R2/(R1+R2))+0.6555551));
  return hasil; 
}

//baca sensor tegangan 2 
float baca_nilai_tegangan2(int pin){
  int nilaiTegangan = analogRead(pin);
  float Vsensor = nilaiTegangan*(3.3 / 4095.0);
  float hasil = (1.01*(Vsensor / (R2/(R1+R2))+0.6658566));
  return hasil;
}

//baca sensor tegangan 3 
float baca_nilai_tegangan3(int pin){
  int nilaiTegangan = analogRead(pin);
  float Vsensor = nilaiTegangan*(3.3 / 4095.0);
  float hasil = (1.01*(Vsensor / (R2/(R1+R2))+0.6628513));
  return hasil;
}

//baca sensor suhu
float baca_sensor_suhu(){
  float celcius = dht.readTemperature();
  return celcius;
}

// fungsi mengatur relay cut ovv overcurrent buck boost 
void cutoff_overcurrent(float arus){
  if (arus >= 21) {
    digitalWrite(relayPin1, HIGH);  // Mengaktifkan relay untuk memutuskan aliran listrik
    delay(1000);                   // Durasi relay aktif (misal: 1 detik)
    digitalWrite(relayPin1, LOW); 
    lcd.setCursor(0, 0);
    lcd.print("Relay: OFF Overcurrent =");
    lcd.print(arus);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Relay: ON");
  }
}

// fungsi mengatur relay cut off overcurrent baterai
void cutoff_overcurrentbat(float arus){
  if (arus >= 21) {
    digitalWrite(relayPin1, HIGH);  // Mengaktifkan relay untuk memutuskan aliran listrik
    delay(1000);                   // Durasi relay aktif (misal: 1 detik)
    digitalWrite(relayPin1, LOW); 
    lcd.setCursor(0, 0);
    lcd.print("Relay: OFF Overcurrent =");
    lcd.print(arus);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Relay: ON");
  }
}

// fungsi mengantur relay cut off overvoltage  
void cutoff_overvoltage(float tegangan){
  if (tegangan > 12.0) {
    digitalWrite(relayPin3, HIGH);  // Mengaktifkan relay untuk memutuskan aliran listrik
    delay(1000);                   // Durasi relay aktif (misal: 1 detik)
    digitalWrite(relayPin3, LOW); 
    lcd.setCursor(0, 0);
    lcd.print("Relay: OFF Overvoltage =");
    lcd.print(tegangan);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Relay: ON");
  }
}

// fungsi mengatur relay cut off overheat 
void cutoff_overheat(float suhu){
  if (suhu >= 40) {
    digitalWrite(relayPin3, HIGH);  // Mengaktifkan relay untuk memutuskan aliran listrik
    delay(1000);                   // Durasi relay aktif (misal: 1 detik)
    digitalWrite(relayPin3, LOW); 
    lcd.setCursor(0, 0);
    lcd.print("Relay: OFF Overheat =");
    lcd.print(suhu);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Relay: ON");
  }
}

struct fuzzyresult_error{
  float Enegative_big;
  float Enegative_middle;
  float Enegative_small;
  float E_zero;
  float Epositif_small;
  float Epositif_middle;
  float Epositif_big;
};

struct fuzzyresult_derror{
  float DEnegative_big;
  float DEnegative_middle;
  float DEnegative_small;
  float DE_zero;
  float DEpositif_small;
  float DEpositif_middle;
  float DEpositif_big;
};

fuzzyresult_error fuzzy_error(float error){
  fuzzyresult_error result;
  if (error <= -3)
  {
    result.Enegative_big = 1;
    result.Enegative_middle = 0;
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  }
  else if (error > -3 && error < -2)
  {
    result.Enegative_big = (-2 - (error))/(-2 - (-3)); // turun
    result.Enegative_middle =((error) - (-3))/(-2 - (-3)); //naik
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  }
  else if (error > -2 && error < -1)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =(-1 - (error))/(-1 - (-2));
    result.Enegative_small = ((error) - (-2))/(-1 - (-2));
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  }
  else if (error > -1 && error < 0)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = (0 - (error))/(0 - (-1));
    result.E_zero = ((error) - (-1))/(0 - (-1));
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  }
  else if (error > 0 && error < 1)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = 0;
    result.E_zero = (1 - (error))/(1 - 0);
    result.Epositif_small = ((error) - 0)/(1 - 0);
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  } 
  else if (error > 1 && error < 2)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = (2 - (error))/(2 - 1);
    result.Epositif_middle = ((error) - 1)/(2 - 1);
    result.Epositif_big = 0;
  } 
  else if (error > 2 && error < 3)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = (3 - (error))/(3 - 2);
    result.Epositif_big = ((error) - 2)/(3 - 2);
  } 
  else if (error >= 3)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 1;
  }
  return result;
}

fuzzyresult_derror fuzzy_derror(float derror){
  fuzzyresult_derror result;
  if (derror <= -3) {
    result.DEnegative_big = 1;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > -3 && derror < -2) {
    result.DEnegative_big = (-2 - derror) / (-2 - (-3));
    result.DEnegative_middle = (derror - (-3)) / (-2 - (-3));
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > -2 && derror < -1) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = (-1 - derror) / (-1 - (-2));
    result.DEnegative_small = (derror - (-2)) / (-1 - (-2));
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > -1 && derror < 0) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = (0 - derror) / (0 - (-1));
    result.DE_zero = (derror - (-1)) / (0 - (-1));
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > 0 && derror < 1) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = (1 - derror) / (1 - 0);
    result.DEpositif_small = (derror - 0) / (1 - 0);
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > 1 && derror < 2) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = (2 - derror) / (2 - 1);
    result.DEpositif_middle = (derror - 1) / (2 - 1);
    result.DEpositif_big = 0;
  }
  else if (derror > 2 && derror < 3) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = (3 - derror) / (3 - 2);
    result.DEpositif_big = (derror - 2) / (3 - 2);
  }
  else if (derror >= 3) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 1;
  }
  return result;
}

void loop() {
  // pembacaan sensor arus dan adc 
  float arus1 = baca_nilai_arus1(arus_satu);
  float arus2 = baca_nilai_arus2(arus_dua);
  float arus3 = baca_nilai_arus3(arus_tiga);
  float nilaiarusADC1 = baca_nilai_adc(arus_satu);
  float nilaiarusADC2 = baca_nilai_adc(arus_dua);
  float nilaiarusADC3 = baca_nilai_adc(arus_tiga);

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

  // pembacaan sensor tegangan dan adc 
  float tegangan1 =  baca_nilai_tegangan1(tegangan_satu);
  float tegangan2 =  baca_nilai_tegangan2(tegangan_dua);
  float tegangan3 =  baca_nilai_tegangan3(tegangan_tiga);
  float nilaiteganganADC1 = baca_nilai_adc(tegangan_satu);
  float nilaiteganganADC2 = baca_nilai_adc(tegangan_dua);
  float nilaiteganganADC3 = baca_nilai_adc(tegangan_tiga);
  
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

  // pembacaan sensor suhu
  float suhu = baca_sensor_suhu();

  // tampilkan sensor suhu pada lcd 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");      
  lcd.print(tegangan1);
  lcd.print("C");
  delay(1000);

  // mengambil data errror
  float error = setpointtegangan - baca_nilai_tegangan1(tegangan_satu);

  // mengambil data delta error
  float deltaError = error - (setpointtegangan - setpreviousVoltage);
  setpreviousVoltage = baca_nilai_tegangan1(tegangan_satu);

  // relay cut-off jika overcurrent pada baterai 
  cutoff_overcurrentbat(arus2);
  // relay cut-off jika overcurrent pada buckboost
  cutoff_overcurrent(arus1);
  // relay cut-off jika overvoltage
  cutoff_overvoltage(tegangan2);
  // relay cut-off jika overheat
  cutoff_overheat(suhu);

  // proses mencari nilai keanggotaan
}