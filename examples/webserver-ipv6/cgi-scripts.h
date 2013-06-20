/* 
 * File:   cgi-scripts.h
 * Author: enrico
 *
 * Created on 20. Juni 2013, 00:12
 */

#ifndef CGI_SCRIPTS_H
#define	CGI_SCRIPTS_H

#if WEBSERVER_CONF_NANO==1
/* Enable specific cgi's */
#define WEBSERVER_CONF_HEADER    1
//#define WEBSERVER_CONF_HEADER_W3C  1 //Proper header
#define WEBSERVER_CONF_HEADER_MENU 1 //with links to other pages
//#define WEBSERVER_CONF_HEADER_ICON 1 //with favicon
#define WEBSERVER_CONF_FILESTATS 1
#define WEBSERVER_CONF_TCPSTATS  0
#define WEBSERVER_CONF_PROCESSES 0
#define WEBSERVER_CONF_ADDRESSES 1
#define WEBSERVER_CONF_NEIGHBORS 1
#define WEBSERVER_CONF_NEIGHBOR_STATUS 0
#define WEBSERVER_CONF_ROUTES    0
#define WEBSERVER_CONF_ROUTE_LINKS  0
#define WEBSERVER_CONF_SENSORS   0
#define WEBSERVER_CONF_STATISTICS   0
#define WEBSERVER_CONF_TICTACTOE 0   //Needs passquery of at least 10 chars 
//#define WEBSERVER_CONF_PASSQUERY 10
#if WEBSERVER_CONF_PASSQUERY
extern char httpd_query[WEBSERVER_CONF_PASSQUERY];
#endif
/* Log page accesses */
#define WEBSERVER_CONF_LOG       0
/* Include referrer in log */
#define WEBSERVER_CONF_REFERER   0
/*-----------------------------------------------------------------------------*/
#elif WEBSERVER_CONF_NANO==2
/* Enable specific cgi's */
#define WEBSERVER_CONF_HEADER    1
//#define WEBSERVER_CONF_HEADER_W3C  1 //Proper header
#define WEBSERVER_CONF_HEADER_MENU 1 //with links to other pages
//#define WEBSERVER_CONF_HEADER_ICON 1 //with favicon
#define WEBSERVER_CONF_FILESTATS 1
#define WEBSERVER_CONF_TCPSTATS  1
#define WEBSERVER_CONF_PROCESSES 1
#define WEBSERVER_CONF_ADDRESSES 1
#define WEBSERVER_CONF_NEIGHBORS 1
#define WEBSERVER_CONF_NEIGHBOR_STATUS 1
#define WEBSERVER_CONF_ROUTES    1
#define WEBSERVER_CONF_ROUTE_LINKS  1
#define WEBSERVER_CONF_SENSORS   1
#define WEBSERVER_CONF_STATISTICS   1
//#define WEBSERVER_CONF_TICTACTOE 1   //Needs passquery of at least 10 chars 
#define WEBSERVER_CONF_SHOW_ROOM 0
#define WEBSERVER_CONF_PASSQUERY 10
#if WEBSERVER_CONF_PASSQUERY
extern char httpd_query[WEBSERVER_CONF_PASSQUERY];
#endif

/* Log page accesses */
#define WEBSERVER_CONF_LOG       0
/* Include referrer in log */
#define WEBSERVER_CONF_REFERER   1

/*-----------------------------------------------------------------------------*/
#elif WEBSERVER_CONF_NANO==3
/* Enable specific cgi's */
#define WEBSERVER_CONF_HEADER    1
//#define WEBSERVER_CONF_HEADER_W3C  1 //Proper header
#define WEBSERVER_CONF_HEADER_MENU 1 //with links to other pages
//#define WEBSERVER_CONF_HEADER_ICON 1 //with favicon
#define WEBSERVER_CONF_FILESTATS 1
#define WEBSERVER_CONF_TCPSTATS  1
#define WEBSERVER_CONF_PROCESSES 1
#define WEBSERVER_CONF_ADDRESSES 1
#define WEBSERVER_CONF_NEIGHBORS 1
#define WEBSERVER_CONF_ROUTES    1
#define WEBSERVER_CONF_NEIGHBORS 1
#define WEBSERVER_CONF_NEIGHBOR_STATUS 1
#define WEBSERVER_CONF_ROUTES    1
#define WEBSERVER_CONF_ROUTE_LINKS  1
#define WEBSERVER_CONF_SENSORS   1
#define WEBSERVER_CONF_STATISTICS   1
#define WEBSERVER_CONF_TICTACTOE 1   //Needs passquery of at least 10 chars 
#define WEBSERVER_CONF_PASSQUERY 10
#if WEBSERVER_CONF_PASSQUERY
extern char httpd_query[WEBSERVER_CONF_PASSQUERY];
#endif

/* Log page accesses */
#define WEBSERVER_CONF_LOG       1
/* Include referrer in log */
#define WEBSERVER_CONF_REFERER   1

#else
#error Specified WEBSERVER_CONF_NANO configuration not supported.
#endif /* WEBSERVER_CONF_NANO */

#endif	/* CGI_SCRIPTS_H */

