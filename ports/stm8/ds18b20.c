#include "w1.h"
#include "atom.h"
#define DQ_PORT GPIOC
#define DQ_PIN  GPIO_PIN_3

uint8_t rom[W1_LINE_MAX_DEVICE_COUNT][8],rom_index;

static void ds18b20_drv_set(uint8_t state)
{
  void (*fn)(GPIO_TypeDef*, GPIO_Pin_TypeDef)=state>0?GPIO_WriteHigh:GPIO_WriteLow;
  fn(DQ_PORT,DQ_PIN);
}
static uint8_t ds18b20_drv_get(void)
{
  return GPIO_ReadInputPin(DQ_PORT,DQ_PIN)!=RESET;
}

void ds18b20_drv_setdirection(uint8_t out)
{
  if(out){
    DQ_PORT->DDR|=DQ_PIN;
  }else{
    DQ_PORT->DDR&=~DQ_PIN;
  }
}

extern void w1_delay(int udelay);
uint8_t ds18b20_readByte(struct w1_master *dev)    //mcu读一个字节  
{  
    uint8_t i,value=0;  
    for(i=0;i<8;i++)  
    {  
        dev->bus_master->write_bit(0);        
        value>>=1;w1_delay(0);
        dev->bus_master->write_bit(1);
        dev->bus_master->set_pullup(0);
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        if(dev->bus_master->read_bit())            
        {  
            value|=0x80;//保存高电平数据,低电平的话不用保存,移位后默认是0  
        }
         dev->bus_master->set_pullup(1);
        w1_delay(30); //延时60.76us    
    }
    return value;
}  
void ds18b20_writeByte(struct w1_master *dev,uint8_t dat)  //mcu向ic写一个字节  
{  
    uint8_t i;  
    for(i=0;i<8;i++)  
    {  
        dev->bus_master->write_bit(0);            //产生读写时序的起始信号  
        w1_delay(0);
        dev->bus_master->write_bit(dat & 0x01); //对总线赋值,从最低位开始写起  
        w1_delay(30); //延时60.76us 
        dev->bus_master->write_bit(1);          //释放总线,为下一次mcu送数据做准备,         
        dat>>=1;     //有效数据移动到最低位,2次写数据间隙至少需1us  
    }
} 

uint16_t ds18b20_read2(struct w1_master *dev,w1_err_t *err){
  uint16_t temp=0;
  disableInterrupts();
  if(w1_reset_bus(dev,50))
  {
    *err=W1_ERR_NACK;
    goto end;
  }
  ds18b20_writeByte(dev,0xcc);
  ds18b20_writeByte(dev,0x44);
  atomTimerDelay(1);
  if(w1_reset_bus(dev,50))
  {
    *err=W1_ERR_NACK;
    goto end;
  }
  ds18b20_writeByte(dev,0xcc);
  ds18b20_writeByte(dev,0xbe);
  temp=ds18b20_readByte(dev);
  temp|=(uint16_t)(((uint16_t)ds18b20_readByte(dev))<<8);
  *err=W1_ERR_NONE;
end:
  enableInterrupts();
  return temp;  
}






void w1_slave_found_callback_0(struct w1_master *dev, uint8_t rn[])
{
  memcpy(rom[rom_index++],rn,8);
}

uint16_t ds18b20_read(struct w1_master *dev,w1_err_t *err){
  uint16_t temp=0;
  w1_search(dev,W1_SEARCH,w1_slave_found_callback_0);
  if(w1_reset_bus(dev,50))
  {
    *err=W1_ERR_NACK;
    goto end;
  }
  w1_write_8(dev,W1_SKIP_ROM);
  w1_write_8(dev,W1_CONVERT_TEMP);
  atomTimerDelay(10);
  if(w1_reset_bus(dev,50))
  {
    *err=W1_ERR_NACK;
    goto end;
  }
  w1_write_8(dev,W1_SKIP_ROM);
  w1_write_8(dev,W1_READ_SCRATCHPAD);
  temp=w1_read_8(dev);
  temp|=(uint16_t)(((uint16_t)w1_read_8(dev))<<8);
  *err=W1_ERR_NONE;
end:
  return temp;  
}

void ds18b20_init(){
  GPIO_Init(DQ_PORT,DQ_PIN,GPIO_MODE_OUT_PP_HIGH_SLOW);
}
uint16_t ds18b20_match_read(struct w1_master *dev,uint8_t rom[],w1_err_t *err){
  int temp=0xffff;
  if(w1_reset_bus(dev,127))
  {
    goto end;
  }
  w1_write_8(dev,W1_SKIP_ROM);
  w1_write_8(dev,W1_CONVERT_TEMP);
  if(w1_reset_bus(dev,127))
  {
    goto end;
  }
  w1_write_8(dev,W1_MATCH_ROM);
  w1_write_block(dev,rom,8);
  atomTimerDelay(10);  
  w1_write_8(dev,W1_READ_SCRATCHPAD);
  temp=w1_read_8(dev);
  temp|=((uint16_t)w1_read_8(dev))<<8;
end:
  return temp;
}

uint8_t *ds18b20_get_rom(uint8_t index){
  return rom[index];
}

struct bus_master w1_bus_master=
{
  ds18b20_drv_set,
  ds18b20_drv_get,
  ds18b20_drv_setdirection,
};
struct w1_master w1_therm_master=
{
  1,
  1,
  {0},
  W1_LINE_MAX_DEVICE_COUNT,
  {
    0,0,0
  },
  0,
  &w1_bus_master,
};

