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
	#define TXBUFFERSIZE  120
#define RXBUFFERSIZE   0x20

static __IO u32 TimingDelay = 0;
uint8_t TxBuffer[] = "\n\rUSART Hyperterminal Interrupts Example: USART-Hyperterminal\
 communication using Interrupt\n\r";
uint8_t RxBuffer[RXBUFFERSIZE];
uint8_t NbrOfDataToTransfer = TXBUFFERSIZE;
uint8_t NbrOfDataToRead = RXBUFFERSIZE;
__IO uint8_t TxCounter = 0; 
__IO uint16_t RxCounter = 0; 	
	
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
	
void USARTx_IRQHANDLER(void)
{
  if(USART_GetITStatus( USART2, USART_IT_RXNE) != RESET)
  {		
    /* Read one byte from the receive data register */
    RxBuffer[RxCounter] = (USART_ReceiveData(USART2));
		USART_ClearITPendingBit( USART2,USART_IT_RXNE ); 
		 USART_SendData(USART2, RxBuffer[RxCounter]);
		RxCounter++;
    if(RxCounter == NbrOfDataToRead)
    {
      /* Disable the EVAL_COMX Receive interrupt */
      //USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    }
  }

  if(USART_GetITStatus(USART2, USART_IT_TC ) != RESET)
  {   
		USART_ClearITPendingBit( USART2,USART_IT_TC ); 
    /* Write one byte to the transmit data register */
    USART_SendData(USART2, TxBuffer[TxCounter++]);
		
    if(TxCounter == NbrOfDataToTransfer)
    {
      /* Disable the EVAL_COMX Transmit interrupt */
      USART_ITConfig(USART2, USART_IT_TC , DISABLE);
    }
  }
}
void NVIC_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the USARTx Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

static void uart2int( void )
{	
  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOA, ENABLE );
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE );
	
	 /* Connect PXx to USARTx_Tx */
  GPIO_PinAFConfig( GPIOA, GPIO_PinSource2, GPIO_AF_USART2 );

  /* Connect PXx to USARTx_Rx */
  GPIO_PinAFConfig( GPIOA, GPIO_PinSource3, GPIO_AF_USART2 );
		
  USART_InitStructure.USART_BaudRate = 57600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No ;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  // Configure USARTx_Tx as alternate function push-pull //
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  // Configure USARTx_Rx as input floating //
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStructure);  
  

	USART_Init(USART2,&USART_InitStructure); 

  USART_ITConfig( USART2, USART_IT_RXNE,ENABLE );  
  USART_ClearITPendingBit( USART2,USART_IT_RXNE ); 	
  
 	USART_Cmd( USART2, ENABLE ); 		
 	 		
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
	 
	NVIC_Config();
	uart2int();
	
	USART_ITConfig( USART2, USART_IT_TC , ENABLE);
  
  /* Wait until EVAL_COMX send the TxBuffer */
  while(TxCounter < NbrOfDataToTransfer)
  {}
	//USART_ITConfig( USART2, USART_IT_TXE, DISABLE);
	while( 1 )
	{
		Delay(500);
		GPIO_ToggleBits( GPIOB, GPIO_Pin_6 );
		GPIO_ToggleBits( GPIOB, GPIO_Pin_7 );
		
	}
	
	
	
	
	
}









