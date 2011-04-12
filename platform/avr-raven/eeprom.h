#include <avr/eeprom.h>     
#define   EEPReadByte(addr)         eeprom_read_byte((uint8_t *)addr)     
#define   EEPWriteByte(addr, val)   eeprom_write_byte((uint8_t *)addr, val)  
