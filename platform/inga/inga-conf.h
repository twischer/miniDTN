#ifndef __INGA_CONF_H__
#define __INGA_CONF_H__

/**
 * \addtogroup inga
 * @{
 */

/** @defgroup inga-conf INGA Configuration options.
 *
 * This is a list of all configuration options avaiable exclusivle on the INGA platform.
 *
 * @{ */

/** Configures Baudrate of UART.
 * Default: 38400 */
#ifdef DOXYGEN
#define INGA_CONF_USART_BAUD 38400
#endif

#ifndef INGA_CONF_USART_BAUD
#define INGA_USART_BAUD USART_BAUD_38400
#else
#define INGA_USART_BAUD INGA_CONF_USART_BAUD
#endif

/** Use ADC for random number generation
 * Default: Enabled */
#ifdef DOXYGEN
#define INGA_CONF_ADC_RANDOM
#endif

#ifndef INGA_CONF_ADC_RANDOM
#define INGA_ADC_RANDOM 1
#else
#define INGA_ADC_RANDOM INGA_CONF_ADC_RANDOM
#endif /* INGA_CONF_ADC_RANDOM */


/** Enables writing of auto-generated EUI64 to EEPROM.
 * This has only an effect if the EUI64 is not stored in EEPROM or given as a define.
 * Default: Enabled */
#ifdef DOXYGEN
#define INGA_CONF_WRITE_EUI64 1
#endif

#ifndef INGA_CONF_WRITE_EUI64
#define INGA_WRITE_EUI64 1
#else /* INGA_CONF_WRITE_EUI64 */
#define INGA_WRITE_EUI64 INGA_CONF_WRITE_EUI64
#endif /* INGA_CONF_WRITE_EUI64 */

/* Assure NODE_ID is not set manually */
#ifdef NODE_ID
#undef NODE_ID
#warning Use NODE_CONF_ID to define your NodeId
#endif /* NODE_ID */

/** Set node ID.
 * @note This is for debugging and simulation purpose only!
 */
#ifdef DOXYGEN
#define NODE_CONF_ID
#endif

#ifndef NODE_CONF_ID
#define NODE_ID	0
#else /* NODE_CONF_ID */
#define NODE_ID	NODE_CONF_ID
#endif /* NODE_CONF_ID */


/** \name Periodic settings
 *
 * INGA provides the periodic output of informations such as runtime, free stack, or IP routes.
 * These can be configured with the defines noted below.
 * The interval with that the informations are printed can be set individually
 *
 * By default periodic prints are turned on for time stamps and free stack info showing up
 * every minute.
 * @{ */

/** Enables Periodic prints.
 * Specific prints can be enabled/disabled and configured seperately */
#ifdef DOXYGEN
#define INGA_CONF_PERIODIC
#endif

/// @cond
#ifndef INGA_CONF_PERIODIC
#define INGA_PERIODIC 1
#else
#define INGA_PERIODIC INGA_CONF_PERIODIC
#endif
/// @endcond

/** Enables route prints with given interval [seconds] 
 * Default: 0 */
#ifdef DOXYGEN
#define INGA_CONF_PERIODIC_ROUTES
#endif

/// @cond
#ifndef INGA_CONF_PERIODIC_ROUTES
#define INGA_PERIODIC_ROUTES 0
#else
#define INGA_PERIODIC_ROUTES INGA_CONF_PERIODIC_ROUTES
#if INGA_PERIODIC_ROUTES > 0 && !UIP_CONF_IPV6
#error Periodic routes only supported for IPv6
#endif
#endif
/// @endcond

/** Enables time stamps with given interval [seconds] */
#ifdef DOXYGEN
#define INGA_CONF_PERIODIC_STAMPS
#endif

/// @cond
#ifndef INGA_CONF_PERIODIC_STAMPS
#define INGA_PERIODIC_STAMPS 60
#else
#define INGA_PERIODIC_STAMPS INGA_CONF_PERIODIC_STAMPS
#endif
/// @endcond

/** Activates stack monitor with given interval [seconds] */
#ifdef DOXYGEN
#define INGA_CONF_PERIODIC_STACK
#endif

/// @cond
#ifndef INGA_CONF_PERIODIC_STACK
#define INGA_PERIODIC_STACK 60
#else
#define INGA_PERIODIC_STACK INGA_CONF_PERIODIC_STACK
#endif
/// @endcond

/** @} */


/** \name Bootscreen parameters
 *
 * INGA provides a useful bootscreen-like status information overview during initialization.
 * This is enabled by default but can be disabled and fine-tuned with the configuration
 * optiones noted below.
 *
 * @{ */

/** Enables INGA boot screen.
 * 0: Disable, 1: Enable*/
#ifdef DOXYGEN
#define INGA_CONF_BOOTSCREEN  1
#endif

/// @cond
#ifndef INGA_CONF_BOOTSCREEN
#define INGA_BOOTSCREEN 1
#else /* INGA_CONF_BOOTSCREEN */
#define INGA_BOOTSCREEN INGA_CONF_BOOTSCREEN
#endif /* INGA_CONF_BOOTSCREEN */
/// @endcond


/** Enables INGA boot screen section Net.
 * @note Does only have an effect if INGA_CONF_BOOTSCREEN is enabled */
#ifdef DOXYGEN
#define INGA_CONF_BOOTSCREEN_NET 1
#endif

/** Enables INGA boot screen section Netstack.
 * @note Does only have an effect if INGA_CONF_BOOTSCREEN is enabled */
#ifdef DOXYGEN
#define INGA_CONF_BOOTSCREEN_NETSTACK 1
#endif

/** Enables INGA boot screen section Radio.
 * @note Does only have an effect if INGA_CONF_BOOTSCREEN is enabled */
#ifdef DOXYGEN
#define INGA_CONF_BOOTSCREEN_RADIO  1
#endif

/** Enables INGA boot screen section Sensors.
 * @note Does only have an effect if INGA_CONF_BOOTSCREEN is enabled */
#ifdef DOXYGEN
#define INGA_CONF_BOOTSCREEN_SENSORS  1
#endif

/// @cond
#if INGA_BOOTSCREEN
#ifndef INGA_CONF_BOOTSCREEN_NET
#define INGA_BOOTSCREEN_NET 1
#else /* INGA_CONF_BOOTSCREEN_NET */
#define INGA_BOOTSCREEN_NET INGA_CONF_BOOTSCREEN_NET
#endif /* INGA_CONF_BOOTSCREEN_NET */

#ifndef INGA_CONF_BOOTSCREEN_NETSTACK
#define INGA_BOOTSCREEN_NETSTACK 1
#else /* INGA_CONF_BOOTSCREEN_NETSTACK */
#define INGA_BOOTSCREEN_NETSTACK INGA_CONF_BOOTSCREEN_NETSTACK
#endif /* INGA_CONF_BOOTSCREEN_NETSTACK */

#ifndef INGA_CONF_BOOTSCREEN_RADIO
#define INGA_BOOTSCREEN_RADIO 1
#else /* INGA_CONF_BOOTSCREEN_RADIO */
#define INGA_BOOTSCREEN_RADIO INGA_CONF_BOOTSCREEN_RADIO
#endif /* INGA_CONF_BOOTSCREEN_RADIO */

#ifndef INGA_CONF_BOOTSCREEN_SENSORS
#define INGA_BOOTSCREEN_SENSORS 1
#else /* INGA_CONF_BOOTSCREEN_SENSORS */
#define INGA_BOOTSCREEN_SENSORS INGA_CONF_BOOTSCREEN_SENSORS
#endif /* INGA_CONF_BOOTSCREEN_SENSORS */

#else /* INGA_BOOTSCREEN */
#define INGA_BOOTSCREEN_NET 0
#define INGA_BOOTSCREEN_NETSTACK 0
#define INGA_BOOTSCREEN_RADIO 0
#define INGA_BOOTSCREEN_SENSORS 0
#endif /* INGA_BOOTSCREEN */
/// @endcond

/** Enables Power value conversion for Radio boot screen */
#ifdef DOXYGEN
#define INGA_CONF_CONVERTTXPOWER 1
#endif

/// @cond
#ifndef INGA_CONF_CONVERTTXPOWER
#define INGA_CONVERTTXPOWER 1
#else
#define INGA_CONVERTTXPOWER INGA_CONF_CONVERTTXPOWER
#endif
/// @endcond


/** Enables terminal escapes for colors in bootscreen etc. */
#ifdef DOXYGEN
#define INGA_CONF_TERMINAL_ESCAPES
#endif

/// @cond
#ifndef INGA_CONF_TERMINAL_ESCAPES
#define INGA_TERMINAL_ESCAPES 0
#else /* INGA_CONF_TERMINAL_ESCAPES */
#define INGA_TERMINAL_ESCAPES INGA_CONF_TERMINAL_ESCAPES
#endif /* INGA_CONF_TERMINAL_ESCAPES */
/// @endcond


/** @} */


/* ************************************************************************** */
/* Preset configurable pan Settings                                           */
/* ************************************************************************** */

/** \name EEPROM overwrite settings
 *
 * Note that this settings are normally stored in EEPROM and should be overriden
 * only for testing and debugging purpose!
 * @{ */

/** Set PAN ID.
 * Default: IEEE802154_PANID */
#ifdef DOXYGEN
#define INGA_CONF_PAN_ID 0xABCD
#endif

/// @cond
#ifndef INGA_CONF_PAN_ID
#define INGA_PAN_ID	IEEE802154_PANID
#else /* INGA_CONF_PAN_ID */
#define INGA_PAN_ID	INGA_CONF_PAN_ID
#endif /* INGA_CONF_PAN_ID */
/// @endcond

/** Set PAN address.
 * @note Should not be set as PAN IDs are meant to be distributed by PAN coordinators only.
 * no default. */
#ifdef DOXYGEN
#define INGA_CONF_PAN_ADDR 0x0000
#endif

/// @cond
#ifndef INGA_CONF_PAN_ADDR
#define INGA_PAN_ADDR	0x0000
#else /* INGA_CONF_PAN_ADDR */
#define INGA_PAN_ADDR	INGA_CONF_PAN_ADDR
#endif /* INGA_CONF_PAN_ADDR */
/// @endcond


/** Set EUI64.
 * Expects a coma-separated list of 8 bit values
 */
#ifdef DOXYGEN
#define INGA_CONF_EUI64 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
#endif

/// @cond
#ifndef INGA_CONF_EUI64
#define INGA_EUI64 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
#else
#define INGA_EUI64  INGA_CONF_EUI64
#endif
/// @endcond

/** Set radio channel.
 * Default: 26 */
#ifdef DOXYGEN
#define INGA_CONF_RADIO_CHANNEL 26
#endif

/// @cond
#ifndef INGA_CONF_RADIO_CHANNEL
#define INGA_RADIO_CHANNEL	26
#else /* INGA_CONF_RADIO_CHANNEL */
#define INGA_RADIO_CHANNEL	INGA_CONF_RADIO_CHANNEL
#endif /* INGA_CONF_RADIO_CHANNEL */
/// @endcond

/** Set radio transmission power. 
 * Default: 0 (+3.0dB) */
#ifdef DOXYGEN
#define INGA_CONF_RADIO_TX_POWER 0
#endif

/// @cond
#ifndef INGA_CONF_RADIO_TX_POWER
#define INGA_RADIO_TX_POWER	0
#else /* INGA_CONF_RADIO_TX_POWER */
#define INGA_RADIO_TX_POWER	INGA_CONF_RADIO_TX_POWER
#endif /* INGA_CONF_RADIO_TX_POWER */
/// @endcond


/** @} */

/** @} */

/** @} */

#endif
