/* 
 * File:   ini_parser.h
 * Author: enrico
 *
 * Created on 1. Februar 2013, 10:28
 */

#ifndef INI_PARSER_H
#define	INI_PARSER_H

/**
 * \defgroup ini_parser Ini file key-value parser.
 * 
 * @{
 */

/// Total number of config groups
#ifndef CONFIG_GROUPS
#define CONFIG_GROUPS 3
#warning CONFIG_GROUPS should be set manually
#endif

/// The maximum key size
#ifndef MAY_KEY_SIZE
#define MAX_KEY_SIZE 5
#warning MAX_KEY_SIZE should be set manually
#endif

/** Maps a groups key name to a value represented by a type and a pointer
 * to the desired storage location
 */
typedef struct cfg_entry_s {
  // key name
  const char* key;
  // pointer to processor function
  int (*handle)(char*, void*);
  // pointer to storage location
  void* value_p;
} cfg_entry;

/** Holds mappings for all key-value pairs of a config group (cfg_group)
 */
typedef struct cfg_group_s {
  const char* name; // name of this group
  int entries; // number of data sets
  cfg_entry entry[MAX_KEY_SIZE]; // keys of data sets
} cfg_group;

/** Maps a group key to a config group (cfg_group)
 */
typedef struct cfg_file_s {
  const char* name; // name of config file
  int entries; // number of groups
  const char* keys[CONFIG_GROUPS]; // keys of available groups
  const cfg_group * groups[CONFIG_GROUPS]; // availbale groups
  const void (*handle[CONFIG_GROUPS])(void);
} cfg_file;

#define FALSE 0
#define TRUE 1

/**
 * Parses the given char array. Must be terminated with '\n'
 * @param buf char cuffer to parse in
 * @param len size of char buffer
 * @return 0 if succeeded, otherwise 1
 */
int parse_ini(char* buf, const cfg_file* conf_file);

/** @} */

#endif	/* INI_PARSER_H */

