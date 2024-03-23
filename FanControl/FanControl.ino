/*
 Name:		FanControl.ino
 Created:	3/4/2024 8:20:05 PM
 Author:	Dang
*/
#include "Timer.h" 
#include <ArduinoJson.h>

StaticJsonDocument<200> json_doc;
char json_output[100];
Timer RefreshTimer; 

#define FAN_SENSOR_PIN 2
#define PUMP_SENSOR_PIN 3
#define FAN_PWM_PIN 4
#define PUMP_PWM_PIN 5
#define FAN_RELAY_PIN 7

#define ROOM_TEMP_PIN A3
#define WATER_IN_TEMP_PIN A4
#define WATER_OUT_TEMP_PIN A5

const float R1 = 10000.0; //參考電阻阻抗
const float ABSOLUTE_ZERO = 273.15; //絕對零度
String UartData = "";

volatile unsigned long FanPwmCounter = 0;
volatile unsigned long PumpPwmCounter = 0;

void setup(void)
{
    pinMode(ROOM_TEMP_PIN, INPUT);
    pinMode(WATER_IN_TEMP_PIN, INPUT);
    pinMode(WATER_OUT_TEMP_PIN, INPUT);
	pinMode(FAN_PWM_PIN, OUTPUT);
	pinMode(PUMP_PWM_PIN, OUTPUT);
	pinMode(FAN_RELAY_PIN, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(FAN_SENSOR_PIN), FanPwmCountPulses, FALLING);
    attachInterrupt(digitalPinToInterrupt(PUMP_SENSOR_PIN), PumpPwmCountPulses, FALLING);
	Serial.begin(115200);
    RefreshTimer.every(500, DataUpdateEvent);
    analogWrite(FAN_PWM_PIN, 30);
    analogWrite(PUMP_PWM_PIN, 0);
}

void loop(void)
{
    RefreshTimer.update();
    ReadSerial();
}

/// <summary>
/// 更新資料
/// </summary>
void DataUpdateEvent()
{
    ReadPwm();
    ReadRoomTemp();
    ReadWaterInTemp();
    ReadWaterOutTemp();
    serializeJson(json_doc, json_output);
    Serial.println(json_output);
}

/// <summary>
/// 讀PWM訊號換轉速
/// </summary>
void ReadPwm()
{ 
    json_doc["pumpRPM"] = FanPwmCounter * 30;
    json_doc["fanRPM"] = PumpPwmCounter * 30;

    FanPwmCounter = 0;
    PumpPwmCounter = 0;
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

    float pwmValue = 0;
    if (cmd[0] == 'f') //fan
    {
        pwmValue = ParseFanValue(cmd);
        digitalWrite(FAN_RELAY_PIN, pwmValue == 0 ? LOW : HIGH);
        analogWrite(FAN_PWM_PIN, pwmValue);
    }
    else if (cmd[0] == 'p') //pump
    {
        pwmValue = ParseFanValue(cmd);
        analogWrite(PUMP_PWM_PIN, pwmValue);
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
    //Serial.println(value);
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

void FanPwmCountPulses() 
{
    FanPwmCounter++;
}

void PumpPwmCountPulses()
{
    PumpPwmCounter++;
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

    json_doc["room"] = B_parameter_equation(analog_output, Beta, RoomTemp, Ro);
}

/// <summary>
/// 讀入水溫度
/// </summary>
void ReadWaterInTemp()
{
    const float Beta = 3974.0; //B parameter
    const float RoomTemp = 301.85f; //室溫
    const float Ro = 8560; //熱敏電阻室溫阻抗值

    int analog_output = analogRead(WATER_IN_TEMP_PIN);

    json_doc["waterIn"] = B_parameter_equation(analog_output, Beta, RoomTemp, Ro);
}

/// <summary>
/// 讀出水溫度
/// </summary>
void ReadWaterOutTemp()
{
    const float Beta = 3974.0; //B parameter
    const float RoomTemp = 301.85f; //室溫
    const float Ro = 8440; //熱敏電阻室溫阻抗值

    int analog_output = analogRead(WATER_OUT_TEMP_PIN);

    json_doc["waterOut"] = B_parameter_equation(analog_output, Beta, RoomTemp, Ro);
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