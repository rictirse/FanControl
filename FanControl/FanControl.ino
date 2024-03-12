/*
 Name:		FanControl.ino
 Created:	3/4/2024 8:20:05 PM
 Author:	Dang
*/
#include "Timer.h" 
Timer TempTimer; 

#define FAN_SENSOR_PIN 2
#define FAN_PWM_PIN 3
#define PUMP_SENSOR_PIN 4
#define PUMP_PWM_PIN 5

#define ROOM_TEMP_PIN A3
#define WATER_IN_TEMP_PIN A4
#define WATER_OUT_TEMP_PIN A5

const float R1 = 10000.0; //參考電阻阻抗
const float ABSOLUTE_ZERO = 273.15; //絕對零度
String UartData = "";
float tRoom, tIn, tOut;

volatile unsigned long PwmCounter = 0;

void setup(void)
{
    pinMode(ROOM_TEMP_PIN, INPUT);
    pinMode(WATER_IN_TEMP_PIN, INPUT);
    pinMode(WATER_OUT_TEMP_PIN, INPUT);
	pinMode(FAN_PWM_PIN, OUTPUT);
	pinMode(PUMP_PWM_PIN, OUTPUT);
    //attachInterrupt(digitalPinToInterrupt(FAN_SENSOR_PIN), countPulses, FALLING);
	Serial.begin(9600);
    TempTimer.every(500, UpdateTemp);
    analogWrite(FAN_PWM_PIN, 30);
    analogWrite(PUMP_PWM_PIN, 0);
}

void loop(void)
{
    //讀Serial port data
    TempTimer.update();
    ReadSerial();

    ////pwm control
    //auto pwm = PercentageToHex(50);
    //analogWrite(FAN_PWM_PIN, pwm);

    ////fan RPM read
    //long rpm = PwmCounter * 60 / 2;
    //Serial.println(PwmCounter);
    //Serial.print("RPM:");
    //Serial.println(rpm);
    //PwmCounter = 0;
}

void UpdateTemp()
{
    ReadRoomTemp();
    ReadWaterInTemp();
    ReadWaterOutTemp();
}

/// <summary>
/// read serial port
/// </summary>
void ReadSerial()
{
    if (Serial.available() == 0) return;

    char serialBuffer = Serial.read();
    if (serialBuffer != '\n')
    {
        UartData += serialBuffer;
    }
    else
    {
        //Serial.print("UartData ");
        //Serial.println(UartData);
        while (true)
        {
            int pos = UartData.indexOf(',');
            if (pos == -1) break;

            String subStr = UartData.substring(0, pos);
            UartData = UartData.substring(pos + 1, UartData.length());
            //Serial.print("sub stirng ");
            //Serial.print(subStr);
            //Serial.print("sub stirng ");
            //Serial.print(UartData);
            //Serial.println("");
            SetPwm(subStr);
        }
        SetPwm(UartData);
        UartData = "";
    }
}

int SetPwm(String cmd)
{
    if (cmd.length() < 2) return;

    if (cmd[0] == 'f') //fan
    {
        //Serial.print("set fan ");
        //Serial.println(cmd);
        analogWrite(FAN_PWM_PIN, ParseFanValue(cmd));
    }
    else if (cmd[0] == 'p') //pump
    {
        //Serial.print("set pump ");
        //Serial.println(cmd);
        analogWrite(PUMP_PWM_PIN, ParseFanValue(cmd));
    }
}

/// <summary>
/// 讀Uart data 解析出風扇強度
/// </summary>
/// <param name="pos"></param>
/// <returns></returns>
int ParseFanValue(String cmd)
{
    int value = cmd.substring(1, UartData.length()).toInt();
    Serial.println(value);
    return PercentageToHex(value);
}


/// <summary>
/// 風扇轉速(百分比)轉PWM訊號(0-255)
/// </summary>
int PercentageToHex(float pwmPercentage)
{
    if (pwmPercentage > 100) return 255;
    else if (pwmPercentage < 0) return 0;
    return pwmPercentage * 2.55f;
}

void countPulses() 
{
    PwmCounter++;
}

/// <summary>
/// 讀室溫
/// </summary>
void ReadRoomTemp()
{
    const float Beta = 3974.0f; //B parameter
    const float RoomTemp = 294.45f; //室溫
    const float Ro = 11490.0f; //熱敏電阻室溫阻抗值

    int analog_output = analogRead(ROOM_TEMP_PIN);
    tRoom = B_parameter_equation(analog_output, Beta, RoomTemp, Ro);

    Serial.print("r");
    Serial.print(tRoom);
    Serial.print(",");
}

/// <summary>
/// 讀入水溫度
/// </summary>
void ReadWaterInTemp()
{
    const float Beta = 3974.0; //B parameter
    const float RoomTemp = 294.45; //室溫
    const float Ro = 11490.0; //熱敏電阻室溫阻抗值

    tIn = 35.0;
    Serial.print("i");
    Serial.print(tIn);
    Serial.print(",");
}

/// <summary>
/// 讀出水溫度
/// </summary>
void ReadWaterOutTemp()
{
    const float Beta = 3974.0; //B parameter
    const float RoomTemp = 294.45; //室溫
    const float Ro = 11490.0; //熱敏電阻室溫阻抗值

    tOut = 40.0;
    Serial.print("o");
    Serial.print(tOut);
    Serial.print(",");
}

/// <summary>
/// 計算溫度
/// </summary>
/// <param name="rawADC">analog訊號</param>
/// <param name="beta">溫度曲線</param>
/// <param name="roomTemp">室溫K值</param>
/// <param name="ro">室溫阻抗值</param>
/// <returns></returns>
float B_parameter_equation(int rawADC, float beta, float roomTemp, float ro)
{
    float R2 = R1 * (1023.0 / (float)rawADC - 1.0);
    float tKelvin = (beta * roomTemp) / (beta + (roomTemp * log(R2 / ro)));
    return tKelvin - ABSOLUTE_ZERO;
}