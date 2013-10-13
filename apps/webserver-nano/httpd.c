/*
 * Copyright (c) 2013, Adam Dunkels, Enrico Joerns
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file Configurable Contiki Webserver with file system support.
 * 
 * \author
 *  Adam Dunkels
 *  Enrico Joerns <e.joerns@tu-bs.de>
 */

#include <string.h>
#include <stdio.h>

#include "contiki-net.h"
#include "httpd.h"

#include "webserver.h"
#include "httpd-fs.h"

#if WEBSERVER_CONF_CGI
#include "httpd-cgi.h"
#endif

#define DEBUG 1
#if DEBUG
#define PRINTD PRINTA
#else
#define PRINTD(...)
#endif

#define STATE_WAITING 0
#define STATE_OUTPUT  1
/* Allocate memory for the tcp connections */
MEMB(conns, struct httpd_state, WEBSERVER_CONF_CONNS);

#ifndef HTTPD_FILE_CACHESIZE
#define HTTPD_FILE_CACHESIZE  128
#endif

//--- Character table

#define ISO_tab     0x09
#define ISO_nl      0x0a
#define ISO_cr      0x0d
#define ISO_space   0x20
#define ISO_bang    0x21
#define ISO_percent 0x25
#define ISO_period  0x2e
#define ISO_slash   0x2f
#define ISO_colon   0x3a
#define ISO_qmark   0x3f

//--- String constants

const char httpd_mime_htm[]     HTTPD_STRING_ATTR = "text/html";
#if WEBSERVER_CONF_CSS
const char httpd_mime_css[]     HTTPD_STRING_ATTR = "text/css";
#endif
#if WEBSERVER_CONF_PNG
const char httpd_mime_png[]     HTTPD_STRING_ATTR = "image/png";
#endif
#if WEBSERVER_CONF_GIF
const char httpd_mime_gif[]     HTTPD_STRING_ATTR = "image/gif";
#endif
#if WEBSERVER_CONF_JPG
const char httpd_mime_jpg[]     HTTPD_STRING_ATTR = "image/jpeg";
#endif
#if WEBSERVER_CONF_TXT
const char httpd_mime_txt[]     HTTPD_STRING_ATTR = "text/plain";
#endif
#if WEBSERVER_CONF_BIN
const char httpd_mime_bin[]     HTTPD_STRING_ATTR = "application/octet-stream";
#endif
#if WEBSERVER_CONF_INCLUDE || WEBSERVER_CONF_CGI
const char httpd_str_shtml []   HTTPD_STRING_ATTR = ".shtml";
#endif
const char httpd_str_indexfn [] HTTPD_STRING_ATTR = "/index.html";
const char httpd_str_indexfn3 [] HTTPD_STRING_ATTR = "/index.htm";
#if WEBSERVER_CONF_INCLUDE || WEBSERVER_CONF_CGI
const char httpd_str_indexsfn [] HTTPD_STRING_ATTR = "/index.shtml";
#endif
const char httpd_str_404fn []   HTTPD_STRING_ATTR = "/404.html";
const char httpd_str_404notf [] HTTPD_STRING_ATTR = "404 Not found";
const char httpd_str_200ok []   HTTPD_STRING_ATTR = "200 OK";
const char httpd_str_get[]      HTTPD_STRING_ATTR = "GET ";
const char httpd_str_ref[]      HTTPD_STRING_ATTR = "Referer:";
const char httpd_str_http[]     HTTPD_STRING_ATTR = "HTTP/1.0 ";
const char httpd_str_server[]   HTTPD_STRING_ATTR = "\r\nServer: Contiki2.6\r\nConnection: close\r\n";
const char httpd_str_content[]  HTTPD_STRING_ATTR = "Content-type: ";
const char httpd_str_crlf[]     HTTPD_STRING_ATTR = "\r\n\r\n";


/* Holds cached file data */
char file_cache_array[HTTPD_FILE_CACHESIZE + 1] = {0}; // alawys 0-terminated
char* file_cache_ptr = file_cache_array;
static int cache_len;
/*---------------------------------------------------------------------------*/
static unsigned short
file_generate(void *state)
{
  struct httpd_state *s = (struct httpd_state *) state;

  if (s->sendlen > uip_mss()) {
    PRINTD("httpd: Send length too large to be sent.\n");
    s->sendlen = uip_mss();
  }

  /* Send data and seek back (needed for retransmissions) */
  s->sendlen = httpd_fs_read(s->fd, uip_appdata, s->sendlen);
  httpd_fs_seek(s->fd, -s->sendlen, HTTPD_SEEK_CUR);
  
  ((char*)uip_appdata)[s->sendlen] = '\0';

  return s->sendlen;
}
/*---------------------------------------------------------------------------*/
static unsigned short
generate(void *state)
{
  struct httpd_state *s = (struct httpd_state *) state;

  if (s->sendlen > uip_mss()) {
    PRINTD("httpd: Send length too large to be sent.\n");
  }

  memcpy(uip_appdata, file_cache_ptr, s->sendlen);

  return s->sendlen;
}
/*---------------------------------------------------------------------------*/
/** 
 * Sends file in blocks of max. uip_mss() size each
 */
static
PT_THREAD(send_file(struct httpd_state *s))
{
  PSOCK_BEGIN(&s->sout);

  s->sendlen = uip_mss();

  static int fend, sent;
  fend = httpd_fs_seek(s->fd, 0L, HTTPD_SEEK_END);
  httpd_fs_seek(s->fd, 0L, HTTPD_SEEK_SET);
  sent = 0;

  do {
    
    if (fend - sent < uip_mss()) {
      s->sendlen = fend - sent;
    }

    PSOCK_GENERATOR_SEND(&s->sout, file_generate, s);
    sent += httpd_fs_seek(s->fd, s->sendlen, HTTPD_SEEK_CUR);

  } while (sent < fend); // seek until EOF

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
#if WEBSERVER_CONF_INCLUDE || WEBSERVER_CONF_CGI
/* Sends s->sendlen bytes of file. */
static
PT_THREAD(send_part_of_cache(struct httpd_state *s))
{
  PSOCK_BEGIN(&s->sout);

  static int tosend;
  tosend = s->sendlen;

  do {

    /* determine size of next packat to send */
    s->sendlen = tosend > uip_mss() ? uip_mss() : tosend;

    PSOCK_GENERATOR_SEND(&s->sout, generate, s); // sends up to uip_mss() bytes

    file_cache_ptr += s->sendlen;

    if (tosend > s->sendlen) {
      tosend -= s->sendlen;
    } else {
      tosend = 0;
    }


  } while (tosend > 0);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
/**
 * Skips over any script parameters to the beginning of the next line.
 * Sets scritptr to next line.
 */
static void
skip_scriptline(struct httpd_state *s)
{
  char *p;
  
  if ((p = (char *) strchr(s->scriptptr, ISO_nl)) != NULL) {
    p += 1;
    s->scriptlen -= (unsigned short) (p - s->scriptptr);
    s->scriptptr = p;
  } else {
    s->scriptlen = 0;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * Extracts a file or cgi name, trim leading spaces and replace termination with zero.
 * 
 * @param destination the extracted scriptname is written to
 * @param input buffer the name is extracted from
 * @return pointer to end of scriptname (incl. trailing spaces),
 *         i.e. start of parameters
 */
char *
get_scriptname(char *dest, char *readbuf)
{
  uint8_t i = 0, skip = 1;
  do {
    dest[i] = *(readbuf++);
    //allow leading colons
    if (dest[i] == ISO_colon) {
      if (!skip) break;
    }
    //skip leading tabs
    else if (dest[i] == ISO_tab) {
      if (skip) continue;
      else break;
    }
    //skip leading spaces
    else if (dest[i] == ISO_space) {
      if (skip) continue;
      else break;
    }
    else if (dest[i] == ISO_nl) break; //nl is preferred delimiter
    else if (dest[i] == ISO_cr) break; //some editors insert cr
    else if (dest[i] == '\0') break; //files are terminated with null
    else skip = 0;
    i++;
  } while (i < (MAX_SCRIPT_NAME_LENGTH + 1));
  readbuf--;
  /* Skip trailing spaces/tabs */
  while ((dest[i] == ISO_space) || (dest[i] == ISO_tab)) dest[i] = *(++readbuf);
  dest[i] = '\0';
  return (readbuf);
}
/*----------------------------------------------------------------------------*/
/** 
 * Loads data from file to cache starting at current file position.
 * @note Position in file will not be changed!
 * 
 * @param httpd_state with fd of file to read from
 * @return -1 if file is done
 */
static int
load_cache(struct httpd_state *s)
{ 
  // set cache pointer to start of cache and load data
  file_cache_ptr = file_cache_array;
  cache_len = httpd_fs_read(s->fd, file_cache_ptr, HTTPD_FILE_CACHESIZE);
  httpd_fs_seek(s->fd, -cache_len, HTTPD_SEEK_CUR);
  *(file_cache_ptr + cache_len) = '\0';
  
  if (cache_len < HTTPD_FILE_CACHESIZE) {
    return -1;
  } else {
    return cache_len;
  }
}
/*----------------------------------------------------------------------------*/
/** 
 * Loads data from file to cache starting with data under current cache pointer.
 * i.e. seeks in file number of bytes between cache start and current cache pointer.
 * 
 * @param httpd_state with fd of file to read from
 * @return -1 if file is done
 */
static int
reload_cache(struct httpd_state *s)
{
  // calculate bytes to seek in file
  int toseek = file_cache_ptr - file_cache_array;
  httpd_fs_seek(s->fd, toseek, HTTPD_SEEK_CUR);

  return load_cache(s);
}
static int
cache_size() {
  return cache_len - (file_cache_ptr - file_cache_array);
}
/*---------------------------------------------------------------------------*/
/**
 * Parses shtml document for possible script includes.
 * Accepts includes of the form '%! scriptname'.
 * 
 * If scriptname starts with ':' it is assumed to be an SSI,
 * otherwise it will be handled as CGI call.
 */
static
PT_THREAD(handle_scripts(struct httpd_state *s))
{
  /* Note script includes will attach a leading : to the filename and a trailing zero */
  static char scriptname[MAX_SCRIPT_NAME_LENGTH + 1], *pptr;
  //  static uint16_t filelength;
  static int done, eof, eoc;

  PT_BEGIN(&s->scriptpt);
  eoc = eof = done = 0;

  /* Init cache (null-terminated)*/
  load_cache(s);

  while (!done) {
    
    /* Assure we can read at least a '%!' sequence. */
//    if (cache_size() < 2) {
//      reload_cache(s);
//    }

    /* Check if we should start executing a script, flagged by %! */
    if (file_cache_ptr[0] == ISO_percent &&
            file_cache_ptr[1] == ISO_bang) {
      
      /* Test if a whole line is in cache, otherwise reload cache. */
      if (strchr(&file_cache_ptr[2], ISO_nl) == NULL) {
        reload_cache(s);
      }

      /* Extract name, if starts with colon include file else call cgi */
      s->scriptptr = get_scriptname(scriptname, &file_cache_ptr[2]);
      //      s->scriptlen = files[s->fd].len - (s->scriptptr - file_cache_ptr[0]);
      PRINTD("httpd: Handle script named %s\n", scriptname);

      /* Include scripts prefixed with ':' are SSI else CGI scripts */
      if (scriptname[0] == ISO_colon) {

#if WEBSERVER_CONF_INCLUDE
        PRINTD("IS SSI\n");
        s->scriptfd = httpd_fs_open(&scriptname[1], HTTPD_FS_READ);
        /* Send script if open succeeded. */
        if (s->scriptfd != -1) {
          s->sendfd = s->scriptfd;
          PT_WAIT_THREAD(&s->scriptpt, send_file(s));
        } else {
          PRINTD("failed opening %s\n", scriptname);
        }

        httpd_fs_close(s->scriptfd);
        /*TODO dont print anything if file not found */
#endif
        /* Execute unprefixed scripts. */
      } else {
        PRINTD("IS CGI\n");
#if WEBSERVER_CONF_CGI
        PT_WAIT_THREAD(&s->scriptpt, httpd_cgi(scriptname)(s, s->scriptptr));
#endif
      }

      skip_scriptline(s);

      file_cache_ptr = s->scriptptr;

      if (cache_size() < 2) {
        reload_cache(s);
      }
      if (cache_size() == 0) {
        done = 1;
      }

    } else { // no script

      /* get position of next percent character */
      if (file_cache_ptr[0] == ISO_percent) {
        pptr = (char *) strchr(&file_cache_ptr[1], ISO_percent);
      } else {
        pptr = (char *) strchr(&file_cache_ptr[0], ISO_percent);
      }

      /* calc new length to send */
      if (pptr == NULL) {
        /* no further percent sign found in cache. */

        /* Send to end of cache */
        s->sendlen = cache_len - (&file_cache_ptr[0] - &file_cache_array[0]);
        eoc = 1;// inidcates thate we need new cache

      } else {
        s->sendlen = (int) ((int) pptr - (int) &file_cache_ptr[0]);
      }

      if (s->sendlen > 0) {
        PRINTD("httpd: Sending %u bytes from 0x%04x\n", s->sendlen, (unsigned int) pptr);
        s->sendfd = s->fd;
        PT_WAIT_THREAD(&s->scriptpt, send_part_of_cache(s));
      }

      /* Reload cache if it was marked as empty. */
      if (eoc) {
        reload_cache(s);
      }
      /* If (reloaded) cache empty, stop sending */
      if (cache_size() == 0) {
        done = 1;
      }

    }
  }

  PT_END(&s->scriptpt);
}
#endif /* WEBSERVER_CONF_INCLUDE || WEBSERVER_CONF_CGI */
/*---------------------------------------------------------------------------*/
static unsigned short
generate_status(void *sstr)
{
  uint8_t slen = httpd_strlen((char *) sstr);
  httpd_memcpy(uip_appdata, httpd_str_http, sizeof (httpd_str_http) - 1);
  httpd_memcpy(uip_appdata + sizeof (httpd_str_http) - 1, (char *) sstr, slen);
  slen += sizeof (httpd_str_http) - 1;
  httpd_memcpy(uip_appdata + slen, httpd_str_server, sizeof (httpd_str_server) - 1);
  return slen + sizeof (httpd_str_server) - 1;
}
/*---------------------------------------------------------------------------*/
static unsigned short
generate_header(void *hstr)
{
  uint8_t slen = httpd_strlen((char *) hstr);
  httpd_memcpy(uip_appdata, httpd_str_content, sizeof (httpd_str_content) - 1);
  httpd_memcpy(uip_appdata + sizeof (httpd_str_content) - 1, (char *) hstr, slen);
  slen += sizeof (httpd_str_content) - 1;
  httpd_memcpy(uip_appdata + slen, httpd_str_crlf, sizeof (httpd_str_crlf) - 1);
  return slen + 4;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_headers(struct httpd_state *s, const char *statushdr))
{
  char *ptr;
  PSOCK_BEGIN(&s->sout);

  PSOCK_GENERATOR_SEND(&s->sout, generate_status, (char *) statushdr);

  ptr = strrchr(s->filename, ISO_period);
  if (httpd_strncmp("4", statushdr, 1) == 0) { //404
    PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_htm);
  } else if (ptr == NULL) {
#if WEBSERVER_CONF_BIN
    PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_bin);
#else
    PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_htm);
#endif 
  } else {
    ptr++;
#if WEBSERVER_CONF_INCLUDE || WEBSERVER_CONF_CGI
    if (httpd_strncmp(ptr, &httpd_mime_htm[5], 3) == 0 || httpd_strncmp(ptr, &httpd_str_shtml[1], 4) == 0) {
#else
    if (httpd_strncmp(ptr, &httpd_mime_htm[5], 3) == 0) {
#endif
      PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_htm);
#if WEBSEVER_CONF_CSS
    } else if (httpd_strcmp(ptr, &httpd_mime_css[5]) == 0) {
      PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_css);
#endif
#if WEBSERVER_CONF_PNG
    } else if (httpd_strcmp(ptr, &httpd_mime_png[6]) == 0) {
      PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_png);
#endif
#if WEBSERVER_CONF_GIF
    } else if (httpd_strcmp(ptr, &httpd_mime_gif[6]) == 0) {
      PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_gif);
#endif
#if WEBSERVER_CONF_JPG
    } else if (httpd_strcmp(ptr, httpd_mime_jpg) == 0) {
      PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_jpg);
#endif
#if WEBSERVER_CONF_TXT
    } else {
      PSOCK_GENERATOR_SEND(&s->sout, generate_header, &httpd_mime_txt);
#endif
    }
  }
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_output(struct httpd_state *s))
{
  char *ptr;

  PT_BEGIN(&s->outputpt);

#if DEBUGLOGIC
  httpd_strcpy(s->filename, httpd_str_indexfn);
#endif

  s->fd = httpd_fs_open(s->filename, HTTPD_FS_READ);
  if (s->fd == -1) { // Opening file failed.

    /* TODO: try index.htm ? */
#if WEBSERVER_CONF_INCLUDE || WEBSERVER_CONF_CGI
    /* If index.html not found try index.htm */
    if (httpd_strcmp(s->filename, httpd_str_indexfn) == 0) {
      httpd_strcpy(s->filename, httpd_str_indexfn3);
    }
    s->fd = httpd_fs_open(s->filename, HTTPD_FS_READ);
    if (s->fd != -1) {
      goto sendfile;
    }
    /* If index.html not found try index.shtml */
    if (httpd_strcmp(s->filename, httpd_str_indexfn) == 0) {
      httpd_strcpy(s->filename, httpd_str_indexsfn);
    }
    s->fd = httpd_fs_open(s->filename, HTTPD_FS_READ);
    if (s->fd == -1) {
      PRINTD("Opening %s failed\n", s->filename);
      goto psock_close;
    } else {
      goto sendfile;
    }
#endif
    /* If nothing was found, send 404 page */
    httpd_strcpy(s->filename, httpd_str_404fn);
    s->fd = httpd_fs_open(s->filename, HTTPD_FS_READ);
    PT_WAIT_THREAD(&s->outputpt, send_headers(s, httpd_str_404notf));
    PT_WAIT_THREAD(&s->outputpt, send_file(s));

  } else { // Opening file succeeded.

sendfile:
    PT_WAIT_THREAD(&s->outputpt, send_headers(s, httpd_str_200ok));
#if WEBSERVER_CONF_INCLUDE || WEBSERVER_CONF_CGI
    /* If filename ends with .shtml, scan file for script includes or cgi */
    ptr = strchr(s->filename, ISO_period);
    if ((ptr != NULL && httpd_strncmp(ptr, httpd_str_shtml, 6) == 0)
            || httpd_strcmp(s->filename, httpd_str_indexfn) == 0) {
      PT_INIT(&s->scriptpt);
      PT_WAIT_THREAD(&s->outputpt, handle_scripts(s));
    } else {
#else
    if (1) {
#endif
      PT_WAIT_THREAD(&s->outputpt, send_file(s));
      httpd_fs_close(s->fd);
    }
  }
psock_close:
  PSOCK_CLOSE(<y & s->sout);
  PT_END(&s->outputpt);
}
/*---------------------------------------------------------------------------*/
#if WEBSERVER_CONF_PASSQUERY
char httpd_query[WEBSERVER_CONF_PASSQUERY];
#endif

static
PT_THREAD(handle_input(struct httpd_state *s))
{

  PSOCK_BEGIN(&s->sin);

  PSOCK_READTO(&s->sin, ISO_space);

  if (httpd_strncmp(s->inputbuf, httpd_str_get, 4) != 0) {
    PSOCK_CLOSE_EXIT(&s->sin);
  }
  PSOCK_READTO(&s->sin, ISO_space);

  if (s->inputbuf[0] != ISO_slash) {
    PSOCK_CLOSE_EXIT(&s->sin);
  }

  if (s->inputbuf[1] == ISO_space) {
    httpd_strcpy(s->filename, httpd_str_indexfn);
  } else {
    uint8_t i;
    for (i = 0; i<sizeof (s->filename) + 1; i++) {
      if (i >= (PSOCK_DATALEN(&s->sin) - 1)) break;
      if (s->inputbuf[i] == ISO_space) break;
#if WEBSERVER_CONF_PASSQUERY
      /* Query string is left in the httpd_query buffer until zeroed by the application! */
      if (s->inputbuf[i] == ISO_qmark) {
        strncpy(httpd_query, &s->inputbuf[i + 1], sizeof (httpd_query));
        break;
      }
#endif
      s->filename[i] = s->inputbuf[i];
    }
    s->filename[i] = 0;
  }

#if WEBSERVER_CONF_LOG
  webserver_log_file(&uip_conn->ripaddr, s->filename);
  //  webserver_log(httpd_query);
#endif
#if WEBSERVER_CONF_LOADTIME
  s->pagetime = clock_time();
#endif
  s->state = STATE_OUTPUT;
  while (1) {
    PSOCK_READTO(&s->sin, ISO_nl);
#if WEBSERVER_CONF_LOG && WEBSERVER_CONF_REFERER
    if (httpd_strncmp(s->inputbuf, httpd_str_ref, 8) == 0) {
      s->inputbuf[PSOCK_DATALEN(&s->sin) - 2] = 0;
      petsciiconv_topetscii(s->inputbuf, PSOCK_DATALEN(&s->sin) - 2);
      webserver_log(s->inputbuf);
    }
#endif
  }
  PSOCK_END(&s->sin);
}
/*---------------------------------------------------------------------------*/
static void
handle_connection(struct httpd_state *s)
{
#if DEBUGLOGIC
  handle_output(s);
#endif
  handle_input(s);
  if (s->state == STATE_OUTPUT) {
    handle_output(s);
  }
}
/*---------------------------------------------------------------------------*/
void
httpd_appcall(void *state)
{
#if DEBUGLOGIC
  struct httpd_state *s; //Enter here for debugging with output directed to TCPBUF
  s = sg = (struct httpd_state *) memb_alloc(&conns);
  if (1) {
#else
  struct httpd_state *s = (struct httpd_state *) state;
  if (uip_closed() || uip_aborted() || uip_timedout()) {
    if (s != NULL) {
      if (s->fd >= 0) {
        httpd_fs_close(s->fd);
        s->fd = -1;
      }
      memb_free(&conns, s);
    }
  } else if (uip_connected()) {
    s = (struct httpd_state *) memb_alloc(&conns);
    if (s == NULL) {
      uip_abort();
      return;
    }
#endif
    tcp_markconn(uip_conn, s);
    PSOCK_INIT(&s->sin, (uint8_t *) s->inputbuf, sizeof (s->inputbuf) - 1);
    PSOCK_INIT(&s->sout, (uint8_t *) s->inputbuf, sizeof (s->inputbuf) - 1);
    PT_INIT(&s->outputpt);
    s->state = STATE_WAITING;
    s->timer = 0;
#if WEBSERVER_CONF_AJAX
    s->ajax_timeout = WEBSERVER_CONF_TIMEOUT;
#endif
    handle_connection(s);
  } else if (s != NULL) {
    if (uip_poll()) {
      ++s->timer;
#if WEBSERVER_CONF_AJAX
      if (s->timer >= s->ajax_timeout) {
#else
      if (s->timer >= WEBSERVER_CONF_TIMEOUT) {
#endif
        uip_abort();
        if (s->fd >= 0) {
          httpd_fs_close(s->fd);
          s->fd = -1;
        }
        memb_free(&conns, s);
      }
    } else {
      s->timer = 0;
    }
    handle_connection(s);
  } else {
    uip_abort();
  }
}
/*---------------------------------------------------------------------------*/
void
httpd_init(void)
{
  tcp_listen(UIP_HTONS(80));
  memb_init(&conns);
  PRINTD(" sizof(struct httpd_state) = %d\n",sizeof(struct httpd_state));
  PRINTA(" %d bytes used for httpd state storage\n", conns.size * conns.num);

}
/*---------------------------------------------------------------------------*/
