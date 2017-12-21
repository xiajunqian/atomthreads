#ifndef __W1_H__
#define __W1_H__
#include "stm8s.h"

#define W1_MAXNAMELEN		32

#define W1_SEARCH		0xF0
#define W1_ALARM_SEARCH		0xEC
#define W1_CONVERT_TEMP		0x44
#define W1_SKIP_ROM		0xCC
#define W1_COPY_SCRATCHPAD	0x48
#define W1_WRITE_SCRATCHPAD	0x4E
#define W1_READ_SCRATCHPAD	0xBE
#define W1_READ_ROM		0x33
#define W1_READ_PSUPPLY		0xB4
#define W1_MATCH_ROM		0x55
#define W1_RESUME_CMD		0xA5

#define W1_SLAVE_ACTIVE		0
#define W1_SLAVE_DETACH		1

#define CFG_W1_ROM_SIZE	        8
#define W1_LINE_MAX_DEVICE_COUNT        32

typedef enum
{
  W1_ERR_NONE=0,
  W1_ERR_NACK,
  W1_ERR_CMD,
  W1_ERR_UNDEFINED
}w1_err_t;
typedef struct
{
  void (*Set)(uint8_t state);
  uint8_t (*Get)(void);
  void (*SetDirection)(uint8_t out);
  void (*DelayUs)(int tm);
}w1_t;

struct bus_master
{
  void (*write_bit)(uint8_t bit);
  uint8_t (*read_bit)(void);
  void (*set_pullup)(uint8_t enable);
};
struct w1_reg_num
{
  uint8_t	family;
  uint8_t	id[6];
  uint8_t	crc;
};
enum w1_master_flags {
  W1_ABORT_SEARCH = 0,
  W1_WARN_MAX_COUNT = 1,
};
struct w1_master
{
  int enable_pullup;
  int pullup_duration;
  uint8_t search_id[CFG_W1_ROM_SIZE];
  int max_slave_count;
  struct w1_reg_num reg_num;
  uint32_t flags;
  struct bus_master *bus_master;
};

typedef void (*w1_slave_found_callback)(struct w1_master *, uint8_t rn[]);

int8_t w1_reset_bus(struct w1_master *dev,int8_t delay);
uint8_t w1_read_8(struct w1_master *dev);
void w1_write_8(struct w1_master *dev, uint8_t byte);
uint8_t w1_triplet(struct w1_master *dev, int bdir);
void w1_write_block(struct w1_master *dev, const uint8_t *buf, int len);
uint8_t w1_read_block(struct w1_master *dev, uint8_t *buf, int len);
void w1_search(struct w1_master *dev, uint8_t search_type, w1_slave_found_callback cb);

int w1_therm_seq_show(struct w1_master *dev);
uint16_t ds18b20_get(w1_err_t *err);
uint16_t w1_therm_match_read(struct w1_master *dev);
int w1_init(struct w1_master *dev,uint8_t rom[],int len,w1_slave_found_callback cb);

#endif
