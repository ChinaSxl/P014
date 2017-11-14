////////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************//
//                                                                                                //
//                                    by SU @ CD.china			          			                      //
//                                                                                                //
//                                   2013-11-1 16:19:58                                           //
//                                                                                                //
//------------------------------------------------------------------------------------------------//
////////////////////////////////////////////////////////////////////////////////////////////////////

//��		�ߣ�SU
//��дʱ�䣺2013-11-1 16:54:41
//��	  ����V1.0
//˵	����ԭ����Ʒ��Ȩ���У�Υ�߱ؾ����������ѧϰ�����ã�ת�أ�����ϵ����su_tech@126.com
//�޸�ʱ�䣺2013-11-1 16:54:44
//�޸ı�ע��

/*
����ϵͳ����
*/

#include <includes.h>
#include "stm32l1xx.h"

#include "Hardware.h"
#include "global.h"
#include "IAP.h"
#include "UART.h"
#include "flash.h"
#include "core_cm3.h"

#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "hw_config.h"


gSTRU_S gStrs;

typedef  void (*pFunction)(void);

u32 JumpAddress;
pFunction JumpToApplication;

static u8 const bootKey[] = "sboot";







u32 GetRxmsg( u8 *ptr, u32 len )
{
	u32 rt;
	#if ( BOOTPORT == BU1 )	||( BOOTPORT == BU2 )
	rt = uartGetRxmsg( ptr, len );
	#else
	rt = usbGetRxmsg( ptr, len );
	#endif
	return rt;
	}; 











//ϵͳ�Ĵ�������״̬����
static void bksys( void )
{
	;
}
//ϵͳ�Ĵ�������״̬�ָ�
static void restsys( void )
{
	SysTick->CTRL &= ~0x01; //�ر����ն�ʱ��
	#if ( BOOTPORT == BU1 )	||( BOOTPORT == BU2 )
	;
	#else
	PowerOff();
	#endif
	
	NvicRest();
}
#if( defined( BOOTKEYEN ) && ( BOOTKEYEN == BENABLE ) )
u32 readkey( void )
{
	u32 tim;
	u16 key0,key;
	key0 = 0xffff;
	key = 0;
	timClr();
	do{
			key = key0;
	   	key0 =  BOOT_KEY_RD();
	   	tim = timRd();
	  }while( (key0 != key )&&( key0 != 0xffff) && ( tim < 200 ) ); //ɨ��200ms����
	do{;}while( ( BOOT_KEY_RD() == 0 ) && ( tim < 200 ) ); 					//���ͷ�
	
	return key;
}
static u32 waitCmd( void )
{
	u8 ch[5];
	u32 ct = 0;
	u32 tim;
	timClr();
	do{
			if( gStrs.uflg == TRUE )
			{
				GetRxmsg( ch, 2 );
				if( ch[0] == bootKey[ct] )
				{
					ct++;
					if( ct == 5 )
						return 1;
				}
				else
				{
					ct = 0;
				}
			}
			tim = timRd();
	  }while(  tim < 120000 ); //��ͨ�ſ����� ԼΪ2����
	return 0;
}
static void IAPprocess( void )
{
	//���ʼ�ܰ����жϰ�����ִ��IAP��س�ʼ��
 	if( readkey() == 0 )	
 	{
 		#if defined( BOOTLEDEN ) && ( BOOTLEDEN == BENABLE )	
 		BOOT_LED_ON();
 		#endif
 		
	  bspInt(); //����ͨ�ſڳ�ʼ��
	  
	 	__enable_irq();	//�����ж�
		waitCmd();
		
		IAPint();
		IAPhdl();
	}	
	// Test if user code is programmed starting from address "APPLICATION_ADDRESS" 
  if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)  //�ж��û������ջ���Ƿ�������Ч��Χ
  {
  	restsys();																									//�ָ������Ѿ������ļĴ���.
  	__disable_irq(); 																						//�����ж�
    // Jump to user application 
    JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
    JumpToApplication = (pFunction) JumpAddress;
    // Initialize user application's Stack Pointer 
    __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
    JumpToApplication();
  }	
}
#else

static u32 waitCmd( void )
{
	u8 ch[5];
	u32 ct = 0;
	u32 tim;
	timClr();
	do{
			if( gStrs.uflg == TRUE )
			{
				GetRxmsg( ch, 2 );
				if( ch[0] == bootKey[ct] )
				{
					ct++;
					if( ct == 5 )
						return 1;
				}
				else
				{
					ct = 0;
				}
			}
			tim = timRd();
	  }while(  tim < WAITKEY_TIM ); //��ͨ�ſ�����
	return 0;
}



static void IAPprocess( void )
{
	u32 tmp;
	
	#if defined( BOOTLEDEN ) && ( BOOTLEDEN == BENABLE )	
	BOOT_LED_ON();
	#endif
	
	bspInt(); 				//ͨ�ſڳ�ʼ��
	
	__enable_irq();		//�����ж�
	
	tmp = waitCmd();	//ִ��һ������ȴ�����3�� ���������ȷ�����BOOT ��������
	if( tmp == 1 ) 		//������ȷִ��BOOT����			
	{
		IAPint(); 
		IAPhdl();
	}	
	// Test if user code is programmed starting from address "APPLICATION_ADDRESS" 
  if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)  //�ж��û������ջ���Ƿ�������Ч��Χ
  {
  	restsys();																									//�ָ������Ѿ������ļĴ���.
  	__disable_irq(); 																						//�����ж�
    // Jump to user application 
    JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
    JumpToApplication = (pFunction) JumpAddress;
    // Initialize user application's Stack Pointer 
    __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
    JumpToApplication();
  }	
}
#endif

int  main1(void)
{
	//u32 tmp;
	bksys();
 	SystemInit();
 	__disable_irq(); 		//�����ж�  __enable_irq();�����ж�

 	BSP_InitSysClk();   
#if ( BOOTPORT == BUSB ) 	
 	Set_USBClock();   	//����USBʱ��
#endif
	bspIntKeyAndLed();
 	
 	IAPprocess(); 
 	
	while(1);  
	  
	return (0);
	}
//////////////////////////////////////////////�����ǲ��Խ�////////////////////////
	static __IO u32 TimingDelay = 0;
void Delay(__IO uint32_t nTime)
{ 
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

void STickIsr( void )	
{
	 if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}
	
	
int main( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SystemInit();//ϵͳ������2Mģʽ
	SysTick_Config( SystemCoreClock / 1000 ); //1ms�ж�һ��
	
	RCC_AHBPeriphClockCmd(	RCC_AHBPeriph_GPIOB,	ENABLE	);	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init( GPIOB, &GPIO_InitStructure );  
	 
	
	while( 1 )
	{
		Delay(500);
		GPIO_ToggleBits( GPIOB, GPIO_Pin_6 );
		GPIO_ToggleBits( GPIOB, GPIO_Pin_7 );
	}
	
	
	
	
	
}









