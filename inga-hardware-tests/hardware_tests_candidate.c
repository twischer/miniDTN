#include "contiki.h"
#include "net/rime.h"

#include <util/delay.h>
#include <stdio.h>
#include "test.h"
#include "test-params.h"
#include "lib/settings.h"

#include "dev/acc-sensor.h"
#include "dev/battery-sensor.h"
#include "dev/gyro-sensor.h"
#include "dev/pressure-sensor.h"
#include "dev/at45db.h"
#include "sensor-tests.h"
#include "clock.h"

/*--- Test parameters ---*/
#define ACC_TEST_CFG_MIN_ACC    800L
#define ACC_TEST_CFG_MAX_ACC    1200L
/*--- ---*/
/*--- Test parameters ---*/
#define SENSOR_NAME             "BATTERY"
#define BATTERY_TEST_CFG_MIN_V  3000
#define BATTERY_TEST_CFG_MAX_V  4500
#define BATTERY_TEST_CFG_MIN_I  30
#define BATTERY_TEST_CFG_MAX_I  100
/*--- ---*/

static struct unicast_conn uc;
static char buff_[30];
static uint8_t radio_done=0;
static uint8_t rec_count = 0;



TEST_SUITE("net_test_receiver");
/*---------------------------------------------------------------------------*/
PROCESS(rime_unicast_sender, "Example unicast");
AUTOSTART_PROCESSES(&rime_unicast_sender);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  char *datapntr;
  datapntr = packetbuf_dataptr();
  datapntr[NET_TEST_CFG_REQUEST_MSG_LEN] = '\0';

  printf("unicast message received from %x.%x: '%s'\n", from->u8[0], from->u8[1], datapntr);

  sprintf(buff_, NET_TEST_CFG_REQUEST_MSG, rec_count);
  printf("packet: %s\n buff: %s\n",(char *) packetbuf_dataptr(), buff_);
  if(strcmp((char *) datapntr, buff_)== 0){
	  rec_count++;
  }

  if (rec_count == 10) {
  	radio_done=1;
  }
}

assert_acc_value()
{
  int idx;
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;

  // wait for stable values
  _delay_ms(100);

  for (idx = 0; idx < 2; idx++) {
    x += acc_sensor.value(ACC_X);
    y += acc_sensor.value(ACC_Y);
    z += acc_sensor.value(ACC_Z);
  }
  x >>= 1;
  y >>= 1;
  z >>= 1;
  // calc ^2 of vec length
  int32_t static_g = x*x + y*y + z*z;
  // test for 1g +/- 10%
  //TEST_REPORT("x-axis", (int16_t) x, 1, "mg");
  //TEST_REPORT("y-axis", (int16_t) y, 1, "mg");
  //TEST_REPORT("z-axis", (int16_t) z, 1, "mg");

  if ((static_g > ACC_TEST_CFG_MAX_ACC*ACC_TEST_CFG_MAX_ACC) && 
  		  (static_g <  ACC_TEST_CFG_MIN_ACC*ACC_TEST_CFG_MIN_ACC)){
	return 0;	
  }
  return 1;
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc}; // List of Callbacks to be called if a message has been received.
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(rime_unicast_sender, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc));

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks); // Channel
  static struct etimer et;
  static rimeaddr_t addr;

  etimer_set(&et, 2*CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  addr.u8[0] = 1 & 0xFF;
  addr.u8[1] = 0 >> 8;
  static int8_t idx = 0;
  char buff_[30] = {'\0'};
  static uint8_t test_num =0;
//  TEST_REPORT("NODE_ID", (uint16_t)settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0), 1, "");
  printf("TEST:RESULT:NODE_ID:0x%04x\n",(uint16_t)settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0));
  while (1) {
    etimer_set(&et, CLOCK_SECOND/4);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

	if(test_num==0){
		//Radio
		if(!radio_done && idx < 15){
			sprintf(buff_, NET_TEST_CFG_REQUEST_MSG, rec_count);
			packetbuf_copyfrom(buff_ , NET_TEST_CFG_REQUEST_MSG_LEN); 
			unicast_send(&uc, &addr);
			printf("send: %s\n",buff_);
			idx++;
		}else{
			test_num++;
			char err_buf[100];
			sprintf(err_buf,"Radio received too few packages %d of %d",rec_count,idx);
			TEST_ASSERT(err_buf,radio_done);
			TESTS_PRINT_RESULT("Radio");
		}
	}else if(test_num==1){
		//Accelerometer
		uint8_t test_res =acc_sensor.status(SENSORS_READY) && !acc_sensor.status(SENSORS_ACTIVE) && SENSORS_ACTIVATE(acc_sensor) && acc_sensor.status(SENSORS_ACTIVE);
		TEST_ASSERT("Accelerometer failed to init (may not mounted?)",test_res);
		
		test_res = (acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_0HZ10) && assert_acc_value());
		TEST_ASSERT("Accelerometer vaule failed for 1/10HZ",test_res);
		test_res = (acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_6HZ25) && assert_acc_value()); 
		TEST_ASSERT("Accelerometer vaule failed for 6/25HZ",test_res);
		test_res = (acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_25HZ) && assert_acc_value()); 
		TEST_ASSERT("Accelerometer vaule failed for 25HZ",test_res);
		test_res = (acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_100HZ) && assert_acc_value());
		TEST_ASSERT("Accelerometer vaule failed for 100HZ",test_res);
		test_res = (acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_400HZ) && assert_acc_value());
		TEST_ASSERT("Accelerometer vaule failed for 400HZ",test_res);
		
		test_res = SENSORS_DEACTIVATE(acc_sensor) && /*!acc_sensor.status(SENSORS_ACTIVE) &&*/ acc_sensor.status(SENSORS_READY);
		TEST_ASSERT("Accelerometer failed deactivate",test_res);
		TESTS_PRINT_RESULT("Accelerometer");
		test_num++;


	}else if (test_num==2){
		uint8_t test_res =battery_sensor.status(SENSORS_READY) && !battery_sensor.status(SENSORS_ACTIVE) && SENSORS_ACTIVATE(battery_sensor) && battery_sensor.status(SENSORS_ACTIVE);
		TEST_ASSERT("Battery-sensor failed to init",test_res);

		int v = battery_sensor.value(BATTERY_VOLTAGE);
		int i = battery_sensor.value(BATTERY_CURRENT);

		test_res = v <= BATTERY_TEST_CFG_MAX_V && v >= BATTERY_TEST_CFG_MIN_V && i <= BATTERY_TEST_CFG_MAX_I && i >= BATTERY_TEST_CFG_MIN_I;
		TEST_ASSERT("Battery-sensor value failed",test_res);

		test_res = SENSORS_DEACTIVATE(battery_sensor) && /*!acc_sensor.status(SENSORS_ACTIVE) &&*/ battery_sensor.status(SENSORS_READY);
		TEST_ASSERT("Battery-sensor failed deactivate",test_res);
		TESTS_PRINT_RESULT("Battery-sensor");
		test_num++;

	}else if(test_num==3){
		uint8_t test_res = gyro_sensor.status(SENSORS_READY) && !gyro_sensor.status(SENSORS_ACTIVE) && SENSORS_ACTIVATE(gyro_sensor) && gyro_sensor.status(SENSORS_ACTIVE);
		TEST_ASSERT("Gyro sensor failed to init",test_res);

		//TODO: test values

		test_res = SENSORS_DEACTIVATE(gyro_sensor) && /*!acc_sensor.status(SENSORS_ACTIVE) &&*/ gyro_sensor.status(SENSORS_READY);
		TEST_ASSERT("Gyro failed deactivate",test_res);
		TESTS_PRINT_RESULT("Gyro");
		test_num++;

	}else if(test_num==4){
		uint8_t test_res = pressure_sensor.status(SENSORS_READY) && !pressure_sensor.status(SENSORS_ACTIVE) && SENSORS_ACTIVATE(pressure_sensor) && pressure_sensor.status(SENSORS_ACTIVE);
		TEST_ASSERT("Pressure sensor failed to init",test_res);

		//TODO: test values

		test_res = SENSORS_DEACTIVATE(pressure_sensor) && /*!acc_sensor.status(SENSORS_ACTIVE) &&*/ pressure_sensor.status(SENSORS_READY);
		TEST_ASSERT("Pressure failed deactivate",test_res);
		TESTS_PRINT_RESULT("Pressure-sensor");
		test_num++;
	//TODO Flash testen
	}else if(test_num==5){
		uint8_t test_res = !at45db_init();
		TEST_ASSERT("Flash failed to init",test_res);
		uint8_t buffer[512];
		uint8_t orig_data[512];
		memset(buffer,1,512);
		at45db_read_page_bypassed(0,0,orig_data,512);
		at45db_erase_page(0);
		at45db_write_page(0,0,buffer,512);
		memset(buffer,2,512);
		at45db_read_page_bypassed(0,0,buffer,512);
		uint16_t i=0;
		for (;i<512;i++){
			if (buffer[i]!=1){
				TEST_ASSERT("Flash failed to read or write",0);
				break;		
			}
		}
		at45db_erase_page(0);
		at45db_write_page(0,0,orig_data,512);
		TESTS_PRINT_RESULT("Flash");
		test_num++;

	}else{
		TESTS_DONE();
	}
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
