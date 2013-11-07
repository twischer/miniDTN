#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

#include "../test.h"
#include "lib/settings.h"

TEST_SUITE("settings test");
/*---------------------------------------------------------------------------*/
PROCESS(settings_test_process, "UDP client process");
AUTOSTART_PROCESSES(&settings_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(settings_test_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

#if APP_SETTINGS_DELETE
  TEST_BEGIN("settings_delete");

  etimer_set(&et, CLOCK_SECOND);

  PROCESS_YIELD();

  TEST_EQUALS(settings_check(SETTINGS_KEY_PAN_ADDR, 0), 0);
  TEST_EQUALS(settings_check(SETTINGS_KEY_PAN_ID, 0), 0);
  TEST_EQUALS(settings_check(SETTINGS_KEY_CHANNEL, 0), 0);
  TEST_EQUALS(settings_check(SETTINGS_KEY_TXPOWER, 0), 0);
  TEST_EQUALS(settings_check(SETTINGS_KEY_EUI64, 0), 0);

#elif APP_SETTINGS_SET
  TEST_BEGIN("settings_set");

  etimer_set(&et, CLOCK_SECOND);

  PROCESS_YIELD();

  uint16_t settings_nodeid = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
  TEST_EQUALS(settings_nodeid, NODE_CONF_ID);

  uint16_t settings_panid = settings_get_uint16(SETTINGS_KEY_PAN_ID, 0);
  TEST_EQUALS(settings_panid, RADIO_CONF_PAN_ID);

  uint8_t settings_channel = settings_get_uint8(SETTINGS_KEY_CHANNEL, 0);
  TEST_EQUALS(settings_channel, RADIO_CONF_CHANNEL);

  uint8_t settings_txpower = settings_get_uint8(SETTINGS_KEY_TXPOWER, 0);
  TEST_EQUALS(settings_txpower, RADIO_CONF_TX_POWER);

  uint8_t settings_eui64[8];
  uint8_t compare_eui64[8] = {EUI64};
  settings_get(SETTINGS_KEY_EUI64, 0, settings_eui64, sizeof (settings_eui64));
  TEST_EQUALS(settings_eui64[0], compare_eui64[0]);
  TEST_EQUALS(settings_eui64[1], compare_eui64[1]);
  TEST_EQUALS(settings_eui64[2], compare_eui64[2]);
  TEST_EQUALS(settings_eui64[3], compare_eui64[3]);
  TEST_EQUALS(settings_eui64[4], compare_eui64[4]);
  TEST_EQUALS(settings_eui64[5], compare_eui64[5]);
  TEST_EQUALS(settings_eui64[6], compare_eui64[6]);
  TEST_EQUALS(settings_eui64[7], compare_eui64[7]);

#endif /* APP_SETTINGS_SET */

  TEST_END();

  TESTS_DONE();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
