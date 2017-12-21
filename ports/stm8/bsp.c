#include "bsp.h"

#define LED0_PORT GPIOE
#define LED0_PIN  GPIO_PIN_5




void bsp_init(){
  //clk
  CLK_HSICmd(ENABLE);
  while(CLK_GetFlagStatus(CLK_FLAG_HSIRDY)==RESET);
  CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
 
  //gpio
  GPIO_Init(LED0_PORT,LED0_PIN,GPIO_MODE_OUT_OD_HIZ_SLOW);  
}



void bsp_led_toggle(uint8_t id){
  if(id==BSP_ID_LED0){
    GPIO_WriteReverse(LED0_PORT,LED0_PIN);
  }
}
void bsp_led_set(uint8_t id,uint8_t state){
  void (*fn)(GPIO_TypeDef*, GPIO_Pin_TypeDef)=state>0?GPIO_WriteLow:GPIO_WriteHigh;
  if(id==BSP_ID_LED0){
    fn(LED0_PORT,LED0_PIN);
  }
}