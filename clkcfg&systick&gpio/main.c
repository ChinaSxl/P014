////////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************//
//                                                                                                //
//                                    by SU @ CD.china			          			                      //
//                                                                                                //
//                                   2013-11-1 16:19:58                                           //
//                                                                                                //
//------------------------------------------------------------------------------------------------//
////////////////////////////////////////////////////////////////////////////////////////////////////

//作		者：SU
//编写时间：2013-11-1 16:54:41
//版	  本：V1.0
//说	明：原创作品版权所有，违者必纠，程序仅供学习；商用，转载，请联系作者su_tech@126.com
//修改时间：2013-11-1 16:54:44
//修改备注：

/*
整个系统根据
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











//系统寄存器或者状态备存
static void bksys( void )
{
	;
}
//系统寄存器或者状态恢复
static void restsys( void )
{
	SysTick->CTRL &= ~0x01; //关闭嘀哒定时器
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
	  }while( (key0 != key )&&( key0 != 0xffff) && ( tim < 200 ) ); //扫描200ms按键
	do{;}while( ( BOOT_KEY_RD() == 0 ) && ( tim < 200 ) ); 					//等释放
	
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
	  }while(  tim < 120000 ); //等通信口命令 约为2分钟
	return 0;
}
static void IAPprocess( void )
{
	//如果始能按键判断按键再执行IAP相关初始化
 	if( readkey() == 0 )	
 	{
 		#if defined( BOOTLEDEN ) && ( BOOTLEDEN == BENABLE )	
 		BOOT_LED_ON();
 		#endif
 		
	  bspInt(); //串行通信口初始化
	  
	 	__enable_irq();	//开总中断
		waitCmd();
		
		IAPint();
		IAPhdl();
	}	
	// Test if user code is programmed starting from address "APPLICATION_ADDRESS" 
  if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)  //判断用户代码堆栈区是否落在有效范围
  {
  	restsys();																									//恢复各项已经操作的寄存器.
  	__disable_irq(); 																						//关总中断
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
	  }while(  tim < WAITKEY_TIM ); //等通信口命令
	return 0;
}



static void IAPprocess( void )
{
	u32 tmp;
	
	#if defined( BOOTLEDEN ) && ( BOOTLEDEN == BENABLE )	
	BOOT_LED_ON();
	#endif
	
	bspInt(); 				//通信口初始化
	
	__enable_irq();		//开总中断
	
	tmp = waitCmd();	//执行一个命令等待周期3秒 如果命令正确则进入BOOT 否则跳出
	if( tmp == 1 ) 		//命令正确执行BOOT程序			
	{
		IAPint(); 
		IAPhdl();
	}	
	// Test if user code is programmed starting from address "APPLICATION_ADDRESS" 
  if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)  //判断用户代码堆栈区是否落在有效范围
  {
  	restsys();																									//恢复各项已经操作的寄存器.
  	__disable_irq(); 																						//关总中断
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
 	__disable_irq(); 		//关总中断  __enable_irq();开总中断

 	BSP_InitSysClk();   
#if ( BOOTPORT == BUSB ) 	
 	Set_USBClock();   	//开启USB时钟
#endif
	bspIntKeyAndLed();
 	
 	IAPprocess(); 
 	
	while(1);  
	  
	return (0);
	}
//////////////////////////////////////////////以下是测试脚////////////////////////
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
	SystemInit();//系统工作在2M模式
	SysTick_Config( SystemCoreClock / 1000 ); //1ms中断一次
	
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









