#include "w1.h"
#include "stm8s.h"
#include <string.h>
#include "atom.h"

static uint8_t div=2;
#pragma optimize=none
void w1_delay(int udelay){
//  int8_t tmp=mul;
//  do{
    for(int i=0;i<udelay;i++){
      asm("nop");
    }
//  }while(tmp-->0);
}
static uint32_t test_bit(uint32_t nr, const volatile void * addr)
{
  return ((*(uint32_t*)addr)&(((uint32_t)0x00000001)<<nr))!=0;
}
static void set_bit(uint32_t nr, const volatile void * addr)
{
  *(uint32_t*)addr|=((uint32_t)0x00000001)<<nr;
}
//int8_t w1_reset_bus(struct w1_master *dev,int8_t delay)
//{
//  int tm=1000;
//  int dm;
//  //reset state
//  dev->bus_master->write_bit(1);
//  dev->bus_master->set_pullup(1);
//  w1_delay(1000); 
//  while(tm-->0){
//    dm=1000;
//    dev->bus_master->write_bit(0);
//    dev->bus_master->set_pullup(1);
//    w1_delay(tm);
//    dev->bus_master->write_bit(1);
//    dev->bus_master->set_pullup(0);
//    do{
//      if(dev->bus_master->read_bit()==0){
//        break;
//      }
//      w1_delay(1);
//    }while(dm-->0);
//    if(dm<=0)break;
//    
//    w1_delay(3000);
//  }
//  if(tm>0)return -1;
//  dev->bus_master->write_bit(1);
//  dev->bus_master->set_pullup(1);
//  w1_delay(48);
//  if(delay>0)return 0;
//  else return -1;
//}
int8_t w1_reset_bus(struct w1_master *dev,int8_t delay)
{
  //reset state
  disableInterrupts();
  dev->bus_master->write_bit(1);
  dev->bus_master->set_pullup(1);
  w1_delay(600/div);
  dev->bus_master->write_bit(0);
  dev->bus_master->set_pullup(1);
  w1_delay(480/div);
  dev->bus_master->write_bit(1);
  dev->bus_master->set_pullup(0);
  do{
    if(dev->bus_master->read_bit()==0){
      break;
    }
    w1_delay(1);
  }while(delay-->0);
  dev->bus_master->write_bit(1);
  dev->bus_master->set_pullup(1);
  w1_delay(480/div);
  enableInterrupts();
  if(delay>0)return 0;
  else return -1;
}

static uint8_t w1_read_bit(struct w1_master *dev)
{
  uint8_t ret;
  disableInterrupts();
  dev->bus_master->write_bit(0);
  w1_delay(0);
  dev->bus_master->write_bit(1);
  dev->bus_master->set_pullup(0);
//  w1_delay(1);
  asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
  ret=dev->bus_master->read_bit();
  dev->bus_master->set_pullup(1);
  w1_delay(30);
  enableInterrupts();
  return ret;
}
int ttt=25;
static void w1_write_bit(struct w1_master *dev, uint8_t bit)
{
  disableInterrupts();
  dev->bus_master->write_bit(0);w1_delay(0);
  dev->bus_master->write_bit(bit);
  w1_delay(90/div);
  dev->bus_master->write_bit(1);
  enableInterrupts();
}
static uint8_t w1_touch_bit(struct w1_master *dev, int bit)
{
  disableInterrupts();
  if(bit)
  {
    return w1_read_bit(dev);
  }
  else
    w1_write_bit(dev, 0);		
  enableInterrupts();
  return 0;
}
static void w1_pre_write(struct w1_master *dev)
{
  disableInterrupts();
  if (dev->pullup_duration &&dev->enable_pullup && dev->bus_master->set_pullup)
  {
    dev->bus_master->set_pullup(dev->pullup_duration);
  }
    enableInterrupts();
}
static void w1_post_write(struct w1_master *dev)
{
  disableInterrupts();
  if (dev->pullup_duration) {
    if (dev->enable_pullup && dev->bus_master->set_pullup)
      dev->bus_master->set_pullup(0);
    else
      atomTimerDelay(dev->pullup_duration);
    dev->pullup_duration = 0;
  }
  enableInterrupts();
}
void w1_write_8(struct w1_master *dev, uint8_t byte)
{
  for(int i=0;i<8;++i)
  {
    if(i==7)
    {
      w1_pre_write(dev);
    }
    w1_touch_bit(dev,(byte>>i)&0x01);
  }
  w1_post_write(dev);
}

uint8_t w1_triplet(struct w1_master *dev, int bdir)
{
  uint8_t id_bit   = w1_touch_bit(dev, 1);
  uint8_t comp_bit = w1_touch_bit(dev, 1);
  uint8_t retval;
  
  if (id_bit && comp_bit)
    return 0x03;  /* error */
  
  if (!id_bit && !comp_bit) {
    /* Both bits are valid, take the direction given */
    retval = bdir ? 0x04 : 0;
  } else {
    /* Only one bit is valid, take that direction */
    bdir = id_bit;
    retval = id_bit ? 0x05 : 0x02;
  }
  w1_write_bit(dev, bdir);
  return retval;
}

uint8_t w1_read_8(struct w1_master *dev)
{
  int i;
  uint8_t res = 0;
  for (i = 0; i < 8; i++)
  {
    res |= (w1_touch_bit(dev,1) << i);
  }
  return res;
}
void w1_write_block(struct w1_master *dev, const uint8_t *buf, int len)
{
  for (int i = 0; i < 8; ++i)
  {
    w1_write_8(dev, buf[i]);
  }
  w1_post_write(dev);
}
uint8_t w1_read_block(struct w1_master *dev, uint8_t *buf, int len)
{
  int i;
  uint8_t ret;
  
  for (i = 0; i < len; ++i)
  {
    buf[i] = w1_read_8(dev);
  }
  ret = len;
  return ret;
}

void w1_search(struct w1_master *dev, uint8_t search_type, w1_slave_found_callback cb)
{
  uint8_t last_rn[CFG_W1_ROM_SIZE], rn[CFG_W1_ROM_SIZE], tmp;
  int slave_count = 0;
  int last_zero, last_device;
  int search_bit, desc_bit;
  uint8_t  triplet_ret = 0;
  
  
  search_bit = 0;
  memcpy(rn,dev->search_id,CFG_W1_ROM_SIZE);
  memset(last_rn,0,CFG_W1_ROM_SIZE);
  last_device = 0;
  last_zero = -1;
  
  desc_bit = 64;
  
  while ( !last_device && (slave_count++ < dev->max_slave_count) ) {
    memcpy(last_rn,rn,CFG_W1_ROM_SIZE);
    memset(rn,0,CFG_W1_ROM_SIZE);
    
    if(w1_reset_bus(dev,127))
    {
      goto end;
    }
    w1_write_8(dev, search_type);
    
    /* Do fast search on single slave bus */
    if (dev->max_slave_count == 1) {
      int rv,rn_flag=0;
      rv = w1_read_block(dev, rn, 8);
      for(int i=0;i<8;i++)
      {
        if(rn[i]!=0)
        {
          rn_flag=1;
          break;
        }
      }
      if (rv == 8 && rn_flag&&cb)
        cb(dev, rn);
      break;
    }
    
    for (int i=0; i<CFG_W1_ROM_SIZE; i++)
    {
      for (int j=0; j<8; j++)
      {
        /* Determine the direction/search bit */
        if (i == desc_bit)
          search_bit = 1;	  /* took the 0 path last time, so take the 1 path */
        else if (i > desc_bit)
          search_bit = 0;	  /* take the 0 path on the next branch */
        else
        {
          search_bit =  (last_rn[i]>>j) & 0x1;
        }
        /* Read two bits and write one bit */
        triplet_ret = w1_triplet(dev, search_bit);
        
        /* quit if no device responded */
        if ( (triplet_ret & 0x03) == 0x03 )
          break;
        
        /* If both directions were valid, and we took the 0 path... */
        if (triplet_ret == 0)
          last_zero = i;
        
        /* extract the direction taken & update the device number */
        tmp = (triplet_ret >> 2);
        rn[i] |= (tmp << j);
        if (test_bit(W1_ABORT_SEARCH, &dev->flags)) {
          return;
        }
      }
    }
    if ( (triplet_ret & 0x03) != 0x03 ) {
      if ((desc_bit == last_zero) || (last_zero < 0)) {
        last_device = 1;
        memset(dev->search_id,0,CFG_W1_ROM_SIZE);
      } else {
        memcpy(dev->search_id,rn,CFG_W1_ROM_SIZE);
      }
      desc_bit = last_zero;
      cb(dev, rn);
    }
    set_bit(W1_WARN_MAX_COUNT, &dev->flags);
  }
end:
  return;
}