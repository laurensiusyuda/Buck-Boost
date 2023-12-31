#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//inisialisasi pin lcd
LiquidCrystal_I2C lcd(0x27, 20, 4);

//inisialisasi pin suhu
#define DHTPIN 25  
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

//inisialisasi pin sensor arus (pin 33,25,26)
#define arus_satu 32 
#define arus_dua 34
#define arus_tiga 36

//inisialisasi pin sensor tegangan (pin 34,35,32)
#define tegangan_satu 33
#define tegangan_dua 35 
#define tegangan_tiga 39

// inisialisasi pin relay
const int relayPin1 = 14;
const int relayPin2 = 27;

//inisialisasi pin kontrol buck boost 
const int pwmChannel18 = 0;
const int pwmChannel19 = 1;
const int pwmFreq = 40000;
const int pwmResolution = 8;

// membuat setpoint untuk menghitung arus dan tegangan 
float sensitivitas = 66;
float ACSoffset = 1650;
float R1 = 100000.0;
float R2 = 6800.0;
float R1_1 = 30000.0;
float R2_2 = 7500.0;


//membuat setpoit tegangan 
float setpointtegangan = 14.4;
float dutyCycle;
float pwmBoost = 0;
float pwmBuck;

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

bool flagEksekusiLCD = false;
int halaman = 1;
bool flaglcdrelay1 = false;
bool flaglcdrelay2 = false;
unsigned long waktuEksekusiLCD = 0;
int timecounter = 0;

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

  //mengatur pwm
  ledcSetup(pwmChannel18, pwmFreq, pwmResolution);
  ledcAttachPin(18, pwmChannel18);
  ledcSetup(pwmChannel19, pwmFreq, pwmResolution);
  ledcAttachPin(19, pwmChannel19);
}


//baca nilai adc 
float baca_nilai_adc(int pin){
  int nilaiADC = analogRead(pin);
  return nilaiADC;
}

//baca sensor arus 1
float baca_nilai_arus1(int pin) {
  int nilaiArus = analogRead(pin);
  float hasil = 0.0095 * nilaiArus - 28.235;
  return hasil;
}

//baca sensor arus 2 
float baca_nilai_arus2(int pin){
   int nilaiArus = analogRead(pin);
  float hasil = 0.0095 * nilaiArus - 28.235;
  return hasil;
}

//baca sensor arus 3 
float baca_nilai_arus3(int pin){
   int nilaiArus = analogRead(pin);
  float hasil = 0.0095 * nilaiArus - 28.235;
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
  float hasil = (1.01*(Vsensor / float (R2_2/(R2_2+R1_1))+0.6658566));
  return hasil;
}
//baca sensor tegangan 3 
float baca_nilai_tegangan3(int pin){
  int nilaiTegangan = analogRead(pin);
  float Vsensor = nilaiTegangan*(3.3 / 4095.0);
  float hasil = (1.01*(Vsensor / (R2_2/(R2_2+R1_1))+0.6628513));
  return hasil;
}

//baca sensor suhu
float baca_sensor_suhu(){
  float celcius = dht.readTemperature();
  return celcius;
}

// cutofff rbaterai
void cutoff_overcurrent(float tegangan){
  if (tegangan >= 15) // setting point overvoltage charging 
  {
    digitalWrite(relayPin1, HIGH);  // Mematikan relay untuk memutuskan aliran listrik
    flaglcdrelay1 = false;
  } else {
    digitalWrite(relayPin1, LOW);  // Mengaktifkan relay untuk mengalirkan listrik
    flaglcdrelay1 = true;
  }
}

// fungsi mengatur relay cut off rbeban 
void cutoff_overheat(float suhu, float arus){
  if (suhu >= 40 || arus > 5) // setting point overheat dan overcurrent beban 
  {
    digitalWrite(relayPin2, HIGH);  // Mengaktifkan relay untuk memutuskan aliran listrik
    flaglcdrelay2 = false;
  } else {
    digitalWrite(relayPin1, LOW);  // Mengaktifkan relay untuk mengalirkan listrik
    flaglcdrelay2 = true;
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

struct fuzzyresult_control {
  float output[49];
  float outputmin[49];
};

fuzzyresult_error fuzzy_error(float error){
  fuzzyresult_error result;
  if (error <= -9)
  {
    result.Enegative_big = 1;
    result.Enegative_middle = 0;
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  }
  else if (error > -9 && error < -6)
  {
    result.Enegative_big = (-6 - (error))/(-6 - (-9));
    result.Enegative_middle =((error) - (-9))/(-6 - (-9));
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  }
  else if (error > -6 && error < -3)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =(-3 - (error))/(-3 - (-6));
    result.Enegative_small = ((error) - (-6))/(-3 - (-6));
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  }
  else if (error > -3 && error < 0)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = (0 - (error))/(0 - (-3));
    result.E_zero = ((error) - (-3))/(0 - (-3));
    result.Epositif_small = 0;
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  }
  else if (error > 0 && error < 3)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = 0;
    result.E_zero = (3 - (error))/(3 - 0);
    result.Epositif_small = ((error) - 0)/(3 - 0);
    result.Epositif_middle = 0;
    result.Epositif_big = 0;
  } 
  else if (error > 3 && error < 6)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = (6 - (error))/(6 - 3);
    result.Epositif_middle = ((error) - 3)/(6 - 3);
    result.Epositif_big = 0;
  } 
  else if (error > 6 && error < 9)
  {
    result.Enegative_big = 0;
    result.Enegative_middle =0;
    result.Enegative_small = 0;
    result.E_zero = 0;
    result.Epositif_small = 0;
    result.Epositif_middle = (9 - (error))/(9 - 6);
    result.Epositif_big = ((error) - 9)/(9 - 6);
  } 
  else if (error >= 9)
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
  if (derror <= -9) {
    result.DEnegative_big = 1;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > -9 && derror < -6) {
    result.DEnegative_big = (-6 - derror) / (-6 - (-9));
    result.DEnegative_middle = (derror - (-9)) / (-6 - (-9));
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > -6 && derror < -3) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = (-3 - derror) / (-3 - (-6));
    result.DEnegative_small = (derror - (-6)) / (-3 - (-6));
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > -3 && derror < 0) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = (0 - derror) / (0 - (-3));
    result.DE_zero = (derror - (-3)) / (0 - (-3));
    result.DEpositif_small = 0;
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > 0 && derror < 3) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = (3 - derror) / (3 - 0);
    result.DEpositif_small = (derror - 0) / (3 - 0);
    result.DEpositif_middle = 0;
    result.DEpositif_big = 0;
  }
  else if (derror > 3 && derror < 6) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = (6 - derror) / (6 - 3);
    result.DEpositif_middle = (derror - 3) / (6 - 3);
    result.DEpositif_big = 0;
  }
  else if (derror > 6 && derror < 9) {
    result.DEnegative_big = 0;
    result.DEnegative_middle = 0;
    result.DEnegative_small = 0;
    result.DE_zero = 0;
    result.DEpositif_small = 0;
    result.DEpositif_middle = (9 - derror) / (9 - 6);
    result.DEpositif_big = (derror - 6) / (9 - 6);
  }
  else if (derror >= 9) {
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

float nb(float alfa) {
  if (alfa > 0 && alfa < 1) {
    return (8 - alfa * 3);
  } else if (alfa == 1) {
    return 5;
  } else {
    return 8;
  }
}

float nm (float alfa){
  if (alfa > 0 && alfa < 1) {
    return (10 - alfa * 2);
  } else if (alfa == 1) {
    return 8;
  } else {
    return 10;
  }
}

float ns (float alfa){
  if (alfa > 0 && alfa < 1) {
    return (12 - alfa * 2);
  } else if (alfa == 1) {
    return 10;
  } else {
    return 12;
  }
}

float z (float alfa){
  if (alfa > 0 && alfa < 1) {
    return (14 - alfa * 2);
  } else if (alfa == 1) {
    return 12;
  } else {
    return 14;
  }
}

float ps (float alfa){
  if (alfa > 0 && alfa < 1) {
    return (16 - alfa * 2);
  } else if (alfa == 1) {
    return 14;
  } else {
    return 16;
  }
}

float pm (float alfa){
  if (alfa > 0 && alfa < 1) {
    return (18 - alfa * 2);
  } else if (alfa == 1) {
    return 16;
  } else {
    return 18;
  }
}

float pb (float alfa){
  if (alfa > 0 && alfa < 1) {
    return (20 - alfa * 2);
  } else if (alfa == 1) {
    return 18;
  } else {
    return 20;
  }
}

fuzzyresult_control fuzzy_inference(fuzzyresult_error error, fuzzyresult_derror derror) {
  fuzzyresult_control result;

  // Enegative_big
  result.output[0] = nb(min(error.Enegative_big, derror.DEnegative_big));  
  result.outputmin[0] = min(error.Enegative_big, derror.DEnegative_big);
  result.output[1] = nb(min(error.Enegative_big, derror.DEnegative_middle));
  result.outputmin[1] = min(error.Enegative_big, derror.DEnegative_middle);
  result.output[2] = nm(min(error.Enegative_big, derror.DEnegative_small));
  result.outputmin[2] = min(error.Enegative_big, derror.DEnegative_small);
  result.output[3] = nm(min(error.Enegative_big, derror.DE_zero));
  result.outputmin[3] = min(error.Enegative_big, derror.DE_zero);
  result.output[4] = nm(min(error.Enegative_big, derror.DEpositif_small));
  result.outputmin[4] = min(error.Enegative_big, derror.DEpositif_small);
  result.output[5] = ns(min(error.Enegative_big, derror.DEpositif_middle));
  result.outputmin[5] = min(error.Enegative_big, derror.DEpositif_middle);
  result.output[6] = z(min(error.Enegative_big, derror.DEpositif_big));
  result.outputmin[6] = min(error.Enegative_big, derror.DEpositif_big);

  // Enegative_middle
  result.output[7]  = nb(min(error.Enegative_middle, derror.DEnegative_big));
  result.outputmin[7] = min(error.Enegative_middle, derror.DEnegative_big);
  result.output[8]  = nm(min(error.Enegative_middle, derror.DEnegative_middle));
  result.outputmin[8] = min(error.Enegative_middle, derror.DEnegative_middle);
  result.output[9]  = nm(min(error.Enegative_middle, derror.DEnegative_small));
  result.outputmin[9] = min(error.Enegative_middle, derror.DEnegative_small);
  result.output[10] = nm(min(error.Enegative_middle, derror.DE_zero));
  result.outputmin[10] = min(error.Enegative_middle, derror.DE_zero);
  result.output[11] = ns(min(error.Enegative_middle, derror.DEpositif_small));
  result.outputmin[11] = min(error.Enegative_middle, derror.DEpositif_small);
  result.output[12] = z(min(error.Enegative_middle, derror.DEpositif_middle));
  result.outputmin[12] = min(error.Enegative_middle, derror.DEpositif_middle);
  result.output[13] = ps(min(error.Enegative_middle, derror.DEpositif_big));
  result.outputmin[13] = min(error.Enegative_middle, derror.DEpositif_big);

  // Enegative_small
  result.output[14] = nm(min(error.Enegative_small, derror.DEnegative_big));
  result.outputmin[14] = min(error.Enegative_small, derror.DEnegative_big);
  result.output[15] = nm(min(error.Enegative_small, derror.DEnegative_middle));
  result.outputmin[15] = min(error.Enegative_small, derror.DEnegative_middle);
  result.output[16] = nm(min(error.Enegative_small, derror.DEnegative_small));
  result.outputmin[16] = min(error.Enegative_small, derror.DEnegative_small);
  result.output[17] = ns(min(error.Enegative_small, derror.DE_zero));
  result.outputmin[17] = min(error.Enegative_small, derror.DE_zero);
  result.output[18] = z(min(error.Enegative_small, derror.DEpositif_small));
  result.outputmin[18] = min(error.Enegative_small, derror.DEpositif_small);
  result.output[19] = ps(min(error.Enegative_small, derror.DEpositif_middle));
  result.outputmin[19] = min(error.Enegative_small, derror.DEpositif_middle);
  result.output[20] = pm(min(error.Enegative_small, derror.DEpositif_big));
  result.outputmin[20] = min(error.Enegative_small, derror.DEpositif_big);

  // E_zero
  result.output[21] = nm(min(error.E_zero, derror.DEnegative_big));
  result.outputmin[21] = min(error.E_zero, derror.DEnegative_big);
  result.output[22] = nm(min(error.E_zero, derror.DEnegative_middle));
  result.outputmin[22] = min(error.E_zero, derror.DEnegative_middle);
  result.output[23] = ns(min(error.E_zero, derror.DEnegative_small));
  result.outputmin[23] = min(error.E_zero, derror.DEnegative_small);
  result.output[24] = z(min(error.E_zero, derror.DE_zero));
  result.outputmin[24] = min(error.E_zero, derror.DE_zero);
  result.output[25] = ps(min(error.E_zero, derror.DEpositif_small));
  result.outputmin[25] = min(error.E_zero, derror.DEpositif_small);
  result.output[26] = pm(min(error.E_zero, derror.DEpositif_middle));
  result.outputmin[26] = min(error.E_zero, derror.DEpositif_middle);
  result.output[27] = pm(min(error.E_zero, derror.DEpositif_big));
  result.outputmin[27] = min(error.E_zero, derror.DEpositif_big);

  // Epositif_small
  result.output[28] = nm(min(error.Epositif_small, derror.DEnegative_big));
  result.outputmin[28] = min(error.Epositif_small, derror.DEnegative_big);
  result.output[29] = ns(min(error.Epositif_small, derror.DEnegative_middle));
  result.outputmin[29] = min(error.Epositif_small, derror.DEnegative_middle);
  result.output[30] = z(min(error.Epositif_small, derror.DEnegative_small));
  result.outputmin[30] = min(error.Epositif_small, derror.DEnegative_small);
  result.output[31] = ps(min(error.Epositif_small, derror.DE_zero));
  result.outputmin[31] = min(error.Epositif_small, derror.DE_zero);
  result.output[32] = pm(min(error.Epositif_small, derror.DEpositif_small));
  result.outputmin[32] = min(error.Epositif_small, derror.DEpositif_small);
  result.output[33] = pm(min(error.Epositif_small, derror.DEpositif_middle));
  result.outputmin[33] = min(error.Epositif_small, derror.DEpositif_middle);
  result.output[34] = pm(min(error.Epositif_small, derror.DEpositif_big));
  result.outputmin[34] = min(error.Epositif_small, derror.DEpositif_big);

  // Epositif_middle
  result.output[35] = ns(min(error.Epositif_middle, derror.DEnegative_big));
  result.outputmin[35] = min(error.Epositif_middle, derror.DEnegative_big);
  result.output[36] = z(min(error.Epositif_middle, derror.DEnegative_middle));
  result.outputmin[36] = min(error.Epositif_middle, derror.DEnegative_middle);
  result.output[37] = ps(min(error.Epositif_middle, derror.DEnegative_small));
  result.outputmin[37] = min(error.Epositif_middle, derror.DEnegative_small);
  result.output[38] = pm(min(error.Epositif_middle, derror.DE_zero));
  result.outputmin[38] = min(error.Epositif_middle, derror.DE_zero);
  result.output[39] = pm(min(error.Epositif_middle, derror.DEpositif_small)); 
  result.outputmin[39] = min(error.Epositif_middle, derror.DEpositif_small);
  result.output[40] = pm(min(error.Epositif_middle, derror.DEpositif_middle));
  result.outputmin[40] = min(error.Epositif_middle, derror.DEpositif_middle);
  result.output[41] = pb(min(error.Epositif_middle, derror.DEpositif_big));      
  result.outputmin[41] = min(error.Epositif_middle, derror.DEpositif_big);
  
  // Epositif_big
  result.output[42] = z(min(error.Epositif_big, derror.DEnegative_big));
  result.outputmin[42] = min(error.Epositif_big, derror.DEnegative_big);
  result.output[43] = ps(min(error.Epositif_big, derror.DEnegative_middle));
  result.outputmin[43] = min(error.Epositif_big, derror.DEnegative_middle);
  result.output[44] = pm(min(error.Epositif_big, derror.DEnegative_small));
  result.outputmin[44] = min(error.Epositif_big, derror.DEnegative_small);
  result.output[45] = pm(min(error.Epositif_big, derror.DE_zero));
  result.outputmin[45] = min(error.Epositif_big, derror.DE_zero);
  result.output[46] = pm(min(error.Epositif_big, derror.DEpositif_small));
  result.outputmin[46] = min(error.Epositif_big, derror.DEpositif_small);
  result.output[47] = pb(min(error.Epositif_big, derror.DEpositif_middle));
  result.outputmin[47] = min(error.Epositif_big, derror.DEpositif_middle);
  result.output[48] = pb(min(error.Epositif_big, derror.DEpositif_big));
  result.outputmin[48] = min(error.Epositif_big, derror.DEpositif_big);
  return result;
}


float defuzzyfikasi(fuzzyresult_control result, int size) {
  float total1 = 0;
  float total2 = 0;

  for (int i = 0; i < size; i++) {
    if (result.output[i] > 0) {
      total1 += result.outputmin[i] * result.output[i];
      total2 += result.outputmin[i];
    }
  }
  float hasil = total1 / total2;
  return hasil;
}

// Fungsi untuk timer interrupt
void timerInterrupt() {
  if (millis() - waktuEksekusiLCD >= 1000) {
    waktuEksekusiLCD = millis();
    flagEksekusiLCD = true;
    timecounter ++;
    if (flaglcdrelay1 == false || flaglcdrelay2 == false)
    {
      halaman = 0;
      timecounter = 5;
    }
    else if (timecounter >= 5)
    {
      timecounter = 0;
      halaman ++;
      if (halaman == 4)
      {
        halaman = 1;
      }
    }
  }
}

void loop() {
  timerInterrupt();
  // pembacaan sensor arus dan adc 
  float arus1 = abs(baca_nilai_arus1(arus_satu));
  float arus2 = abs(baca_nilai_arus2(arus_dua));
  float arus3 = abs(baca_nilai_arus3(arus_tiga));
  int nilaiarusADC1 = baca_nilai_adc(arus_satu);
  int nilaiarusADC2 = baca_nilai_adc(arus_dua);
  int nilaiarusADC3 = baca_nilai_adc(arus_tiga);

  // pembacaan sensor tegangan dan adc 
  float tegangan1 =  baca_nilai_tegangan1(tegangan_satu);
  float tegangan2 =  baca_nilai_tegangan2(tegangan_dua);
  float tegangan3 =  baca_nilai_tegangan3(tegangan_tiga);
  int nilaiteganganADC1 = baca_nilai_adc(tegangan_satu);
  int nilaiteganganADC2 = baca_nilai_adc(tegangan_dua);
  int nilaiteganganADC3 = baca_nilai_adc(tegangan_tiga);

  // pembacaan sensor suhu
  float suhu = baca_sensor_suhu();

if (flagEksekusiLCD == true) {
  flagEksekusiLCD = false;
  // relay cut-off r baterai
  cutoff_overcurrent(tegangan2);
  // relay cut-off r beban
  cutoff_overheat(suhu,arus3);

  
  // mengambil data errror
  float error =  tegangan1 - setpointtegangan;
  float deltaError  = error;
 
  // Fuzzy Error
  fuzzyresult_error fuzzyError = fuzzy_error(error);
  // Fuzzy Delta Error
  fuzzyresult_derror fuzzyDeltaError = fuzzy_derror(deltaError);
  // Fuzzy Inference
  fuzzyresult_control fuzzyControl = fuzzy_inference(fuzzyError, fuzzyDeltaError);
  // Defuzzyfikasi
  float defuzzyResult = defuzzyfikasi(fuzzyControl, 49);

    if (tegangan1 >= 22) {
      dutyCycle = (14.4 / tegangan1);
      pwmBuck = dutyCycle * 255;
      pwmBoost = 0;
    } else {
      if (defuzzyResult <= 5) {
        pwmBoost = 0;
        pwmBuck = 0;
      } else if (defuzzyResult < 14.4) {
        dutyCycle = (14.4 - defuzzyResult) / 14.4;
        pwmBoost = dutyCycle * 255;
        pwmBuck = 255;
      } else {
        dutyCycle = 14.4 / defuzzyResult;
        pwmBuck = dutyCycle * 255;
        pwmBoost = 0;
      }
    }

  if (flaglcdrelay1 == false || flaglcdrelay2 == false )
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Relay: OFF");
  }  
  else if (halaman == 1)
  {
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
  }
  else if (halaman == 2)
  {
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
  }
  else if (halaman == 3 )
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Suhu: ");      
    lcd.print(suhu);
    lcd.print("C");
  }
  Serial.print("Nilai Tegangan1: ");
  Serial.println(tegangan1);
  Serial.print("Nilai Tegangan2: ");
  Serial.println(tegangan2);
  Serial.print("Nilai Tegangan3: ");
  Serial.println(tegangan3);
  Serial.print("Nilai Duty Cycle: ");
  Serial.println(dutyCycle);
  Serial.print("PWM (Buck): ");
  Serial.println(pwmBuck);
  Serial.print("PWM (boost): ");
  Serial.println(pwmBoost); 
  ledcWrite(pwmChannel18, pwmBuck);
  ledcWrite(pwmChannel19, pwmBoost);
  }
  Serial.println(flagEksekusiLCD);
}