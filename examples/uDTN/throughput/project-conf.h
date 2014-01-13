#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

// Set our MMEM size
#undef MMEM_CONF_SIZE
#define MMEM_CONF_SIZE 2000

// Avoid deleting bundles in storage
// 3 = BUNDLE_STORAGE_BEHAVIOUR_DO_NOT_DELETE
#define BUNDLE_CONF_STORAGE_BEHAVIOUR 3

#endif /* __PROJECT_CONF_H__ */
