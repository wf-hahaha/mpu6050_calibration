// I2Cdev і MPU6050 бібліотеки повинні бути встановленні
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"

/////////////////////////////////// КОНФІГУРАЦІЯ /////////////////////////////
int buffersize=1000; //Кількість зчитувань показників датчиків для виведення середнього значення (default:1000)
int acel_deadzone=8; //Доступна похибка акселерометра (default:8)
int giro_deadzone=1; //Доступна похибка гіроскопа (default:1)

// Адреса пристрою
// AD0 low = 0x68 (по-замовчуванню)
// AD0 high = 0x69
MPU6050 accelgyro(0x68); //Отримання об"єкта для подальшої роботи

int16_t ax, ay, az,gx, gy, gz;

int mean_ax,mean_ay,mean_az,mean_gx,mean_gy,mean_gz,state=0;
int ax_offset,ay_offset,az_offset,gx_offset,gy_offset,gz_offset;

/////////////////////////////////// УСТАНОВКИ ////////////////////////////////////
void setup() {
 // ініціалізація I2C шини
 Wire.begin();
 // Корекція частоти шини. Закоментувати для ARDUINO DUE
 TWBR = 24; // 400kHz I2C clock (200kHz if CPU is 8MHz). Leonardo measured 250kHz.

 // ініціалізація послідовного порта
 Serial.begin(115200);

 // ініціалізація MPU-6050. 
 // В загальному ця функція виконує 4 дії.
 accelgyro.initialize();

 // Стартове привітання
 Serial.println("\nMPU6050 Calibration Sketch");
 delay(2000);
 Serial.println("\nYour MPU6050 should be placed in horizontal position, with package letters facing up. \nDon't touch it until you see a finish message.\n");
 delay(3000);
 // Перевірка з"єднання із MPU-6050
 Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
 delay(1000);
 // Скидання регістрів похибок. 
 // Після проходження калібрування сюди заносяться дані, що видала програма та перевіряється допуск відхилення
 // Допуск повинен бути не більше, ніж  значення acel_deadzone та giro_deadzone. Інакше проводиться повторне калібрування.
 // -5556 -2433 153 -1553 1501  -1961
 accelgyro.setXAccelOffset(0);
 accelgyro.setYAccelOffset(0);
 accelgyro.setZAccelOffset(0);
 accelgyro.setXGyroOffset(0);
 accelgyro.setYGyroOffset(0);
 accelgyro.setZGyroOffset(0);
}

/////////////////////////////////// ЦИКЛ ////////////////////////////////////
void loop() {
 // Перший крок. Зчитування первинних показників датчиків.
 if (state==0){
 Serial.println("\nReading sensors for first time...");
 meansensors();
 state++;
 delay(1000);
 }

 // Другий крок. Калібрування. За початкові дані беруться показники із першого кроку. 
 // Калібрування відбуваєтся до тих пір, поки похибки всіх осей датчиків не будуть менше ніж значення acel_deadzone та giro_deadzone
 if (state==1) {
 Serial.println("\nCalculating offsets...");
 calibration();
 state++;
 delay(1000);
 }

 // Третій крок. Остаточне зчитування даних датчиків із застосуванням останніх показників калібрування.
 if (state==2) {
 meansensors();
 Serial.println("\nFINISHED!");
 Serial.print("\nSensor readings with offsets:\t");
 Serial.print(mean_ax); 
 Serial.print("\t");
 Serial.print(mean_ay); 
 Serial.print("\t");
 Serial.print(mean_az); 
 Serial.print("\t");
 Serial.print(mean_gx); 
 Serial.print("\t");
 Serial.print(mean_gy); 
 Serial.print("\t");
 Serial.println(mean_gz);
 Serial.print("Your offsets:\t");
 Serial.print(ax_offset); 
 Serial.print("\t");
 Serial.print(ay_offset); 
 Serial.print("\t");
 Serial.print(az_offset); 
 Serial.print("\t");
 Serial.print(gx_offset); 
 Serial.print("\t");
 Serial.print(gy_offset); 
 Serial.print("\t");
 Serial.println(gz_offset); 
 Serial.println("\nData is printed as: acelX acelY acelZ giroX giroY giroZ");
 Serial.println("Check that your sensor readings are close to 0 0 16384 0 0 0");
 Serial.println("If calibration was succesful write down your offsets so you can set them in your projects using something similar to mpu.setXAccelOffset(youroffset)");
 while (1);
 }
}

/////////////////////////////////// ФУНКЦІЇ////////////////////////////////////

// Функція отримує середні дані по всім осям. Кількість ітерацій залежить від buffersize
void meansensors(){
 long i=0,buff_ax=0,buff_ay=0,buff_az=0,buff_gx=0,buff_gy=0,buff_gz=0;

 while (i<(buffersize+101)){
 // read raw accel/gyro measurements from device
 accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
 
 if (i>100 && i<=(buffersize+100)){ //Перших 100 зчитувань ігнорується
 // Сумування вимірів
  buff_ax=buff_ax+ax;
  buff_ay=buff_ay+ay;
  buff_az=buff_az+az;
  buff_gx=buff_gx+gx;
  buff_gy=buff_gy+gy;
  buff_gz=buff_gz+gz;
 }
 if (i==(buffersize+100)){
 // Отримання середніх значень у глобальні змінні
  mean_ax=buff_ax/buffersize;
  mean_ay=buff_ay/buffersize;
  mean_az=buff_az/buffersize;
  mean_gx=buff_gx/buffersize;
  mean_gy=buff_gy/buffersize;
  mean_gz=buff_gz/buffersize;
 }
 i++;
 delay(2); //Затримка для виключення повторних вимірів
 }
}

// Функція калібрування
void calibration(){
 // Отримання похибки із допуском.
 ax_offset=-mean_ax/8;
 ay_offset=-mean_ay/8;
 az_offset=(16384-mean_az)/8;

 gx_offset=-mean_gx/4;
 gy_offset=-mean_gy/4;
 gz_offset=-mean_gz/4;
 // Основний цикл калібрування. Триває поки всі показники не будуть у допусках.
 while (1){
  int ready=0;
  // Підставка поточних похибок
  accelgyro.setXAccelOffset(ax_offset);
  accelgyro.setYAccelOffset(ay_offset);
  accelgyro.setZAccelOffset(az_offset);

  accelgyro.setXGyroOffset(gx_offset);
  accelgyro.setYGyroOffset(gy_offset);
  accelgyro.setZGyroOffset(gz_offset);

  // Отримання поточних результатів
  meansensors();
  Serial.println("...");

  if (abs(mean_ax)<=acel_deadzone) ready++;
  else ax_offset=ax_offset-mean_ax/acel_deadzone;

  if (abs(mean_ay)<=acel_deadzone) ready++;
  else ay_offset=ay_offset-mean_ay/acel_deadzone;

  if (abs(16384-mean_az)<=acel_deadzone) ready++;
  else az_offset=az_offset+(16384-mean_az)/acel_deadzone;

  if (abs(mean_gx)<=giro_deadzone) ready++;
  else gx_offset=gx_offset-mean_gx/(giro_deadzone+1);

  if (abs(mean_gy)<=giro_deadzone) ready++;
  else gy_offset=gy_offset-mean_gy/(giro_deadzone+1);

  if (abs(mean_gz)<=giro_deadzone) ready++;
  else gz_offset=gz_offset-mean_gz/(giro_deadzone+1);

  if (ready==6) break; //Якщо всі похибки в допусках - вихід із циклу калібрування. 
 }
}
