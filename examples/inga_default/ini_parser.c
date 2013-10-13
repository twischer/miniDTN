#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ini_parser.h"
#include "logger.h"

static int getKeyID(cfg_group* grp, char* key);
static int processValue(cfg_group* grp, char* key, char* value);
static int processValueByID(cfg_group* grp, int key_id, char* value);

#ifndef DEBUG
#define DEBUG 1
#endif
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*----------------------------------------------------------------------------*/
static int
processValue(cfg_group* grp, char* key, char* value)
{
  return processValueByID(grp, getKeyID(grp, key), value);
}
/*----------------------------------------------------------------------------*/
/**
 * Sets the value of the pointer target associated with the given id
 * @param grp config group for which the value sould be set
 * @param key_id
 * @param value Value to set. The value must be a string convertable to the
 * type specified in the given cfg_group struct grp
 * @return 
 */
static int
processValueByID(cfg_group* grp, int key_id, char* value)
{
  return grp->entry[key_id].handle(value, grp->entry[key_id].value_p);
}
/*----------------------------------------------------------------------------*/
/**
 * Checks the given config group (cfg_group) struct for a key with name 'key'
 * @param grp config group file to search in
 * @param key key to search for
 * @return id of grp in the struct array
 */
static int
getKeyID(cfg_group* grp, char* key)
{
  // iterate over entry lists
  int idx;
  for (idx = 0; idx < grp->entries; idx++) {
    // if match, return corresponding ID
    if (strcmp(grp->entry[idx].key, key) == 0) {
      log_v("Key '%s' found at %d\n", key, idx);
      return idx;
    }
  }
  log_w("Key '%s' not found\n", key);
  return -1;
}
/*----------------------------------------------------------------------------*/
/**
 * Checks the given config file (cfg_file) struct for a key with name 'group'
 * @param file config file struct to search in
 * @param group key to search for
 * @return pointer to config group (cfg_group) struct
 */
static cfg_group*
getGroup(const cfg_file* file, char* group)
{
  // iterate over entry lists
  int idx;
  for (idx = 0; idx < file->entries; idx++) {
    // if match, return corresponding group
    if (strcmp(file->keys[idx], group) == 0) {
      log_v("Group '%s' found\n", group);
      // call handler if available
      if (file->handle[idx] != NULL) {
        file->handle[idx]();
      }
      return file->groups[idx];
    }
  }
  log_w("Group '%s' not found\n", group);
  return NULL;
}
/*----------------------------------------------------------------------------*/
#define START 0
#define READGROUP 1
#define READKEY 2
#define READVALUE 3
//
#define MAX_GROUP_KEY_VALUE_SIZE  30
/** * Parses ini file.
 * 
 * @param buf
 * @return 0 if succeeded, 1 if error occured
 */
int
parse_ini(char* buf, const cfg_file* conf_file)
{
  int count = 0;
  char rkv_buf[MAX_GROUP_KEY_VALUE_SIZE];
  char *rkv_pos;
  rkv_pos = &rkv_buf[0];
  int mode = START;
  cfg_group* current_group = NULL;
  int current_key_id = -1;

  // iterate over whole string
  while (*buf) {
    char c = *buf++;
    switch (c) {
        // newline
      case '\n':
        *rkv_pos = '\0'; // terminate string
        switch (mode) {
          case READGROUP:
            current_group = getGroup(conf_file, rkv_buf);
            break;
          case READVALUE:
            if ((current_group != NULL) && (current_key_id != -1)) {
              processValueByID(current_group, current_key_id, rkv_buf);
            }
            break;
          default:
            // if linebreak after non-empty non-mode data occured, it is an error
            if (rkv_pos > &rkv_buf[0]) {
              log_e("Parse error!\n");
              return -1;
            }
            break;
        }
        // execute...
        // reset pointer
        rkv_pos = &rkv_buf[0];
        mode = READKEY;
        break;
        // group begin
      case '[':
        mode = READGROUP;
        break;
        // group end
      case ']':
        // if mode is not READGROUP something went wrong!
        if (mode != READGROUP) {
          return 1;
        }
        break;
        // value start
      case '=':
        *rkv_pos = '\0';
        switch (mode) {
          case READKEY:
            if (current_group != NULL) {
              current_key_id = getKeyID(current_group, rkv_buf);
            }
            break;
          default:
            log_e("Parse error!\n");
            break;
        }
        rkv_pos = &rkv_buf[0];
        mode = READVALUE;
        break;
        // ignore blanks
      case ' ':
        break;
        // normal letter, add to current word
      default:
        *rkv_pos = c;
        rkv_pos++;
        break;
    }
    count++;
  };
  log_i("Parsing ini done\n");
  return 0;
}

