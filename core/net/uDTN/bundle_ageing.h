/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup bundle Bundle Ageing Mechanisms
 *
 * @{
 */

/**
 * \file
 * \brief This file is responsible for bundle ageing mechanisms
 *
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef __BUNDLE_AGEING_H__
#define __BUNDLE_AGEING_H__

#include <stdint.h>

#include "contiki.h"
#include "mmem.h"

uint8_t bundle_ageing_parse_age_extension_block(struct mmem *bundlemem, uint8_t type, uint32_t flags, uint8_t * buffer, int length);
uint8_t bundle_ageing_encode_age_extension_block(struct mmem *bundlemem, uint8_t *buffer, int max_len);
uint32_t bundle_ageing_get_age(struct mmem * bundlemem);
uint8_t bundle_ageing_is_expired(struct mmem * bundlemem);

#endif /* __BUNDLE_AGEING_H__ */
/** @} */
/** @} */
