#include "stm32f10x.h"

#include "ADC/ADC.h"
#include "Encoder/encoder.h"
#include "CAN.h"
#include "virtual_com.h"
#include "Timer/Timer.h"
#include "virtual_com.h"

#define DRIVE_CONTROL_ID 0x400
#define BATTERY_FULL_MSG 0x622
#define BATT_BASE 0x620

#define ADC_MAX 0x300					// TODO: find out what the actual ADC_MAX is
#define ADC_MIN 0                   // TODO: find out what the actual ADC_MIN is

#define TRUE 1
#define FALSE 0

union {
	float float_var;
	uint8_t chars[4];
} u;

union {
	float float_var;
	uint8_t chars[4];
} v;

CAN_msg_t CAN_drive;

/**
 * When called it updates the CAN_drive and then sends the CAN_drive to the motor controller
 * Takes Current (0.000 - 1.000) you want the car to be at, and Velocity(any number in m/s) you want the car to be at
 */
void sendMotorCommand(float curr, float vel)
{
	v.float_var = vel;
	u.float_var = curr;
	
	//SendString("MOTOR COMMAND");
	
	//Set current
	CAN_drive.data[4] = u.chars[0];
	CAN_drive.data[5] = u.chars[1];
	CAN_drive.data[6] = u.chars[2];
	CAN_drive.data[7] = u.chars[3];
	
	//set velocity
	CAN_drive.data[0] = v.chars[0];
	CAN_drive.data[1] = v.chars[1];
	CAN_drive.data[2] = v.chars[2];
	CAN_drive.data[3] = v.chars[3];
	
	CANSend(&CAN_drive);
	
	//SendString(" CURR: ");
	//SendInt(curr * 100);
	//SendString(" VEL: ");
	//SendInt(vel);
	
	RestartTimer();
}

int main(void)
{
	
	CANInit(CAN_500KBPS);
	EncoderInit();
	ADCInit();
	TimerInit(200);
	VirtualComInit();
	
	//Setup of RegenToggle
	RCC->APB2ENR |= (0x1 << 4); //Clock C enable
	GPIOC->CRH &= ~(0xF); //Reset C8
	GPIOC->CRH |= (0x4); //Enable C8 as Input
	
	GPIOC->CRL &= ~(0xF << 24); //C6
	GPIOC->CRL |= (0x4 << 24); //C6
	
	//CAN recive setup
	CAN_msg_t CAN_rx_msg;
	CAN_msg_t CAN_tx_msg;
	
	//CAN motor command setup
	CAN_drive.len = 8;
	CAN_drive.id = DRIVE_CONTROL_ID + 1;
	
	sendMotorCommand(0, 0);
	
	volatile uint16_t encoder_reading;
	volatile uint16_t adc_reading;

	uint16_t old_encoder_reading = 0x0000;
	uint16_t old_regen_reading = 0x0000;
	
	uint8_t batteryPercent = 0x00;
	uint8_t regenToggle = 0x00;

	while(1) 
	{
		
		//Gets all new values
		encoder_reading = EncoderRead();
		adc_reading = (ReadADC() >> 4) << 4;
		regenToggle = ((GPIOC->IDR >> 8) & 0x1);
		
		if(CANMsgAvail())
		{
			CANReceive(&CAN_rx_msg);
			
			if(CAN_rx_msg.id == BATT_BASE + 6)
			{
				batteryPercent = (int8_t) CAN_rx_msg.data[7];
			}
		}

		//Checks if Regen is on and battery is beloew 98%
		if(regenToggle && batteryPercent < 98)
		{
			//Checks for regen knob not at 0
			if(adc_reading != 0 )//& old_regen_reading != adc_reading) // TODO: Check for change in adc knob so we dont flood CAN, TODO: Add min for adc knob
			{
				//sends knob percentage and velocity
				sendMotorCommand((float) adc_reading/ADC_MAX, 0.000);
			}
			else if(old_encoder_reading != encoder_reading) 
			{
				//Sends peddle percentage and velocity
				sendMotorCommand((float) encoder_reading/(PEDAL_MAX - PEDAL_MIN), 100.000);
			}
		
		} 
		else 
		{
			//If the encoder count changed, send new drive CAN message and restart timer
			if(old_encoder_reading != encoder_reading)
			{
				sendMotorCommand((float) encoder_reading/(PEDAL_MAX - PEDAL_MIN), 100.000);
			}
		 }
		
		//If a timeout occured, send the previously sent CAN drive message
		if(timeoutFlag == TRUE)
		{	
			CANSend(&CAN_drive);
			
			timeoutFlag = FALSE;
			RestartTimer();
		}
		
		old_regen_reading = adc_reading;
		old_encoder_reading = encoder_reading;		
		
	}
	
}