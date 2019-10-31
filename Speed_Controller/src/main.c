#include "stm32f10x.h"

#include "ADC/ADC.h"
#include "Encoder/encoder.h"
#include "CAN.h"
#include "virtual_com.h"
#include "Timer/Timer.h"
#include "virtual_com.h"

#define DRIVE_CONTROL_ID 0x400
#define BATTERY_FULL_MSG 0x622

union {
	float float_var;
	uint8_t chars[4];
} u;

union {
	float float_var;
	uint8_t chars[4];
} v;

int main(void){
	
	CANInit(CAN_500KBPS);
	EncoderInit();
	TimerInit(200);
	VirtualComInit();
	CAN_msg_t CAN_drive;     
	
	CAN_drive.len = 8;
	
	CAN_drive.id = DRIVE_CONTROL_ID + 1;
	
	v.float_var = -100.0;
	u.float_var = 0.0;
	
	//set velocity to 100 for drive messages
	CAN_drive.data[0] = v.chars[0];
	CAN_drive.data[1] = v.chars[1];
	CAN_drive.data[2] = v.chars[2];
	CAN_drive.data[3] = v.chars[3];
	
	//set initial 
	CAN_drive.data[4] = u.chars[0];
	CAN_drive.data[5] = u.chars[1];
	CAN_drive.data[6] = u.chars[2];
	CAN_drive.data[7] = u.chars[3];
	
	volatile uint16_t encoder_reading;
	uint16_t old_encoder_reading = 0x0000;
	
	RCC->APB2ENR |= 0x1 << 2;
	GPIOA->CRL &= ~(0xF << 20);
	GPIOA->CRL |= (0x3 << 20);
	
	while(1) 
	{
		
		encoder_reading = EncoderRead();
		
		if (timeoutFlag == TRUE)
		{
			u.float_var = (float) ((float) encoder_reading/(PEDAL_MAX- PEDAL_MIN));
				
			CAN_drive.data[4] = u.chars[0];
			CAN_drive.data[5] = u.chars[1];
			CAN_drive.data[6] = u.chars[2];
			CAN_drive.data[7] = u.chars[3];
			
			CANSend (&CAN_drive);
			
			timeoutFlag = FALSE;
			RestartTimer();
			
		}
		
		/*
		//If the encoder count changed, send new drive CAN message and restart timer
		if(old_encoder_reading != encoder_reading)
		{

			
			//Use a parabolic scaling
			//u.float_var = (float)(2*((float) encoder_reading/PEDAL_MAX) - ((float) encoder_reading/PEDAL_MAX)*((float) encoder_reading/PEDAL_MAX));		
			u.float_var = (float) ((float) encoder_reading/(PEDAL_MAX- PEDAL_MIN));
				
			CAN_drive.data[4] = u.chars[0];
			CAN_drive.data[5] = u.chars[1];
			CAN_drive.data[6] = u.chars[2];
			CAN_drive.data[7] = u.chars[3];
			
			CANSend(&CAN_drive);
		
			RestartTimer();
		
		}		
		//If a timeout occured, send the previously sent CAN drive message
		else if(timeoutFlag == TRUE)
		{	
			
			CANSend (&CAN_drive);
			
			timeoutFlag = FALSE;
			RestartTimer();
		}
		
		old_encoder_reading = encoder_reading;		
		
		if ((CAN1->ESR >> 3) & 0x1)
		{
			GPIOA->BSRR = 0x1 << 5;
		}
		else
		{
			GPIOA->BRR = 0x1 << 5;
		}
		*/
		
	}
	
}
