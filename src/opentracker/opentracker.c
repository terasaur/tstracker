/* This software was written by Dirk Engling <erdgeist@erdgeist.org>
   It is considered beerware. Prost. Skol. Cheers or whatever.
   Some of the stuff below is stolen from Fefes example libowfat httpd.

   $Id: opentracker.c,v 1.238 2012/05/28 15:24:33 erdgeist Exp $ */

/* System */
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <pthread.h>
#ifdef WANT_SYSLOGS
#include <syslog.h>
#endif

/* Libowfat */
#include <libowfat/socket.h>
#include <libowfat/io.h>
#include <libowfat/iob.h>
#include <libowfat/byte.h>
#include <libowfat/scan.h>
#include <libowfat/ip6.h>

/* Opentracker */
#include "trackerlogic.h"
#include "ot_mutex.h"
#include "ot_http.h"
#include "ot_udp.h"
#include "ot_accesslist.h"
#include "ot_stats.h"
#include "ot_livesync.h"
#include "opentracker.h"

/* Globals */
time_t       g_now_seconds;
char *       g_redirecturl;
uint32_t     g_tracker_id;
volatile int g_opentracker_running = 1;
int          g_self_pipe[2];

char * g_serverdir;
char * g_serveruser;
unsigned int g_udp_workers;

static void panic( const char *routine ) {
  fprintf( stderr, "%s: %s\n", routine, strerror(errno) );
  exit( 111 );
}

static void signal_handler( int s ) {
  if( s == SIGINT ) {
    /* Any new interrupt signal quits the application */
    signal( SIGINT, SIG_DFL);

    /* Tell all other threads to not acquire any new lock on a bucket
       but cancel their operations and return */
    g_opentracker_running = 0;

    trackerlogic_deinit();

#ifdef WANT_SYSLOGS
    closelog();
#endif

    exit( 0 );
  } else if( s == SIGALRM ) {
    /* Maintain our copy of the clock. time() on BSDs is very expensive. */
    g_now_seconds = time(NULL);
    alarm(5);
  }
}

static void defaul_signal_handlers( void ) {
  sigset_t signal_mask;
  sigemptyset(&signal_mask);
  sigaddset (&signal_mask, SIGPIPE);
  sigaddset (&signal_mask, SIGHUP);
  sigaddset (&signal_mask, SIGINT);
  sigaddset (&signal_mask, SIGALRM);
  pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
}

static void install_signal_handlers( void ) {
  struct   sigaction sa;
  sigset_t signal_mask;
  sigemptyset(&signal_mask);

  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if ((sigaction(SIGINT, &sa, NULL) == -1) || (sigaction(SIGALRM, &sa, NULL) == -1) )
    panic( "install_signal_handlers" );

  sigaddset (&signal_mask, SIGINT);
  sigaddset (&signal_mask, SIGALRM);
  pthread_sigmask (SIG_UNBLOCK, &signal_mask, NULL);
}

static size_t header_complete( char * request, ssize_t byte_count ) {
  int i = 0, state = 0;

  for( i=1; i < byte_count; i+=2 )
    if( request[i] <= 13 ) {
      i--;
      for( state = 0 ; i < byte_count; ++i ) {
        char c = request[i];
        if( c == '\r' || c == '\n' )
          state = ( state >> 2 ) | ( ( c << 6 ) & 0xc0 );
        else
          break;
        if( state >= 0xa0 || state == 0x99 ) return i + 1;
      }
  }
  return 0;
}

static void handle_dead( const int64 sock ) {
  struct http_data* cookie=io_getcookie( sock );
  if( cookie ) {
    iob_reset( &cookie->batch );
    array_reset( &cookie->request );
    if( cookie->flag & STRUCT_HTTP_FLAG_WAITINGFORTASK )
      mutex_workqueue_canceltask( sock );
    free( cookie );
  }
  io_close( sock );
}

static void handle_read( const int64 sock, struct ot_workstruct *ws ) {
  struct http_data* cookie = io_getcookie( sock );
  ssize_t byte_count;

  if( ( byte_count = io_tryread( sock, ws->inbuf, G_INBUF_SIZE ) ) <= 0 ) {
    handle_dead( sock );
    return;
  }

  /* If we get the whole request in one packet, handle it without copying */
  if( !array_start( &cookie->request ) ) {
    if( ( ws->header_size = header_complete( ws->inbuf, byte_count ) ) ) {
      ws->request = ws->inbuf;
      ws->request_size = byte_count;
      http_handle_request( sock, ws );
    } else
      array_catb( &cookie->request, ws->inbuf, byte_count );
    return;
  }

  array_catb( &cookie->request, ws->inbuf, byte_count );
  if( array_failed( &cookie->request ) || array_bytes( &cookie->request ) > 8192 ) {
    http_issue_error( sock, ws, CODE_HTTPERROR_500 );
    return;
  }

  while( ( ws->header_size = header_complete( array_start( &cookie->request ), array_bytes( &cookie->request ) ) ) ) {
    ws->request      = array_start( &cookie->request );
    ws->request_size = array_bytes( &cookie->request );
    http_handle_request( sock, ws );
#ifdef WANT_KEEPALIVE
    if( !ws->keep_alive )
#endif
      return;
  }
}

static void handle_write( const int64 sock ) {
  struct http_data* cookie=io_getcookie( sock );
  if( !cookie || ( iob_send( sock, &cookie->batch ) <= 0 ) )
    handle_dead( sock );
}

static void handle_accept( const int64 serversocket ) {
  struct http_data *cookie;
  int64 sock;
  ot_ip6 ip;
  uint16 port;
  tai6464 t;

  while( ( sock = socket_accept6( serversocket, ip, &port, NULL ) ) != -1 ) {

    /* Put fd into a non-blocking mode */
    io_nonblock( sock );

    if( !io_fd( sock ) ||
        !( cookie = (struct http_data*)malloc( sizeof(struct http_data) ) ) ) {
      io_close( sock );
      continue;
    }
    memset(cookie, 0, sizeof( struct http_data ) );
    memcpy(cookie->ip,ip,sizeof(ot_ip6));

    io_setcookie( sock, cookie );
    io_wantread( sock );

    stats_issue_event( EVENT_ACCEPT, FLAG_TCP, (uintptr_t)ip);

    /* That breaks taia encapsulation. But there is no way to take system
       time this often in FreeBSD and libowfat does not allow to set unix time */
    taia_uint( &t, 0 ); /* Clear t */
    tai_unix( &(t.sec), (g_now_seconds + OT_CLIENT_TIMEOUT) );
    io_timeout( sock, t );
  }
}

static void * server_mainloop( void * args ) {
  struct ot_workstruct ws;
  time_t next_timeout_check = g_now_seconds + OT_CLIENT_TIMEOUT_CHECKINTERVAL;
  struct iovec *iovector;
  int    iovec_entries;

  (void)args;

  /* Initialize our "thread local storage" */
  ws.inbuf   = malloc( G_INBUF_SIZE );
  ws.outbuf  = malloc( G_OUTBUF_SIZE );
#ifdef _DEBUG_HTTPERROR
  ws.debugbuf= malloc( G_DEBUGBUF_SIZE );
#endif
  if( !ws.inbuf || !ws.outbuf )
    panic( "Initializing worker failed" );

  for( ; ; ) {
    int64 sock;

    io_wait();

    while( ( sock = io_canread( ) ) != -1 ) {
      const void *cookie = io_getcookie( sock );
      if( (intptr_t)cookie == FLAG_TCP )
        handle_accept( sock );
      else if( (intptr_t)cookie == FLAG_UDP )
        handle_udp6( sock, &ws );
      else if( (intptr_t)cookie == FLAG_SELFPIPE )
        io_tryread( sock, ws.inbuf, G_INBUF_SIZE );
      else
        handle_read( sock, &ws );
    }

    while( ( sock = mutex_workqueue_popresult( &iovec_entries, &iovector ) ) != -1 )
      http_sendiovecdata( sock, &ws, iovec_entries, iovector );

    while( ( sock = io_canwrite( ) ) != -1 )
      handle_write( sock );

    if( g_now_seconds > next_timeout_check ) {
      while( ( sock = io_timeouted() ) != -1 )
        handle_dead( sock );
      next_timeout_check = g_now_seconds + OT_CLIENT_TIMEOUT_CHECKINTERVAL;
    }

    livesync_ticker();

    /* Enforce setting the clock */
    signal_handler( SIGALRM );
  }
  return 0;
}

int64_t ot_try_bind( ot_ip6 ip, uint16_t port, PROTO_FLAG proto ) {
  int64 sock = proto == FLAG_TCP ? socket_tcp6( ) : socket_udp6( );

#ifndef WANT_V6
  if( !ip6_isv4mapped(ip) ) {
    exerr( "V4 Tracker is V4 only!" );
  }
#else
  if( ip6_isv4mapped(ip) ) {
    exerr( "V6 Tracker is V6 only!" );
  }
#endif

#ifdef _DEBUG
  {
  char *protos[] = {"TCP","UDP","UDP mcast"};
  char _debug[512];
  int off = snprintf( _debug, sizeof(_debug), "Binding socket type %s to address [", protos[proto] );
  off += fmt_ip6c( _debug+off, ip);
  snprintf( _debug + off, sizeof(_debug)-off, "]:%d...", port);
  fputs( _debug, stderr );
  }
#endif

  if( socket_bind6_reuse( sock, ip, port, 0 ) == -1 )
    panic( "socket_bind6_reuse" );

  if( ( proto == FLAG_TCP ) && ( socket_listen( sock, SOMAXCONN) == -1 ) )
    panic( "socket_listen" );

  if( !io_fd( sock ) )
    panic( "io_fd" );

  io_setcookie( sock, (void*)proto );

  if( (proto == FLAG_UDP) && g_udp_workers ) {
    io_block( sock );
    udp_init( sock, g_udp_workers );
  } else
    io_wantread( sock );

#ifdef _DEBUG
  fputs( " success.\n", stderr);
#endif

  return sock;
}

void load_state(const char * const state_filename ) {
  FILE *  state_filehandle;
  char    inbuf[512];
  ot_hash infohash;
  unsigned long long base, downcount;
  int consumed;

  state_filehandle = fopen( state_filename, "r" );

  if( state_filehandle == NULL ) {
    fprintf( stderr, "Warning: Can't open config file: %s.", state_filename );
    return;
  }

  /* We do ignore anything that is not of the form "^[:xdigit:]:\d+:\d+" */
  while( fgets( inbuf, sizeof(inbuf), state_filehandle ) ) {
    int i;
    for( i=0; i<(int)sizeof(ot_hash); ++i ) {
      int eger = 16 * scan_fromhex( inbuf[ 2*i ] ) + scan_fromhex( inbuf[ 1 + 2*i ] );
      if( eger < 0 )
        continue;
      infohash[i] = eger;
    }

    if( i != (int)sizeof(ot_hash) ) continue;
    i *= 2;

    if( inbuf[ i++ ] != ':' || !( consumed = scan_ulonglong( inbuf+i, &base ) ) ) continue;
    i += consumed;
    if( inbuf[ i++ ] != ':' || !( consumed = scan_ulonglong( inbuf+i, &downcount ) ) ) continue;
    add_torrent_from_saved_state( infohash, base, downcount );
  }

  fclose( state_filehandle );
}

int drop_privileges ( const char * const serveruser, const char * const serverdir ) {
  struct passwd *pws = NULL;

#ifdef _DEBUG
  if( !geteuid() )
    fprintf( stderr, "Dropping to user %s.\n", serveruser );
  if( serverdir )
    fprintf( stderr, "ch%s'ing to directory %s.\n", geteuid() ? "dir" : "root", serverdir );
#endif

  /* Grab pws entry before chrooting */
  pws = getpwnam( serveruser );
  endpwent();

  if( geteuid() == 0 ) {
    /* Running as root: chroot and drop privileges */
    if( serverdir && chroot( serverdir ) ) {
      fprintf( stderr, "Could not chroot to %s, because: %s\n", serverdir, strerror(errno) );
      return -1;
    }

    if(chdir("/"))
      panic("chdir() failed after chrooting: ");

    /* If we can't find server user, revert to nobody's default uid */
    if( !pws ) {
      fprintf( stderr, "Warning: Could not get password entry for %s. Reverting to uid -2.\n", serveruser );
      setegid( (gid_t)-2 ); setgid( (gid_t)-2 );
      setuid( (uid_t)-2 );  seteuid( (uid_t)-2 );
    }
    else {
      setegid( pws->pw_gid ); setgid( pws->pw_gid );
      setuid( pws->pw_uid );  seteuid( pws->pw_uid );
    }

    if( geteuid() == 0 || getegid() == 0 )
      panic("Still running with root privileges?!");
  }
  else {
    /* Normal user, just chdir() */
    if( serverdir && chdir( serverdir ) ) {
      fprintf( stderr, "Could not chroot to %s, because: %s\n", serverdir, strerror(errno) );
      return -1;
    }
  }

  return 0;
}

int opentracker_main() {
  //ot_ip6 serverip, tmpip;
  //uint16_t tmpport;
  char * statefile = 0;
/*
  memset( serverip, 0, sizeof(ot_ip6) );
#ifndef WANT_V6
  serverip[10]=serverip[11]=-1;
  noipv6=1;
#endif
*/

#ifdef WANT_DEV_RANDOM
  srandomdev();
#else
  srandom( time(NULL) );
#endif

  /* Bind to our default tcp/udp ports */
  /*
  if( !bound) {
    ot_try_bind( serverip, 6969, FLAG_TCP );
    ot_try_bind( serverip, 6969, FLAG_UDP );
  }

  if( !g_udp_workers )
    udp_init( -1, 0 );
  */

#ifdef WANT_SYSLOGS
  openlog( "opentracker", 0, LOG_USER );
  setlogmask(LOG_UPTO(LOG_INFO));
#endif

  if( drop_privileges( g_serveruser ? g_serveruser : "nobody", g_serverdir ) == -1 )
    panic( "drop_privileges failed, exiting. Last error");

  g_now_seconds = time( NULL );

  /* Create our self pipe which allows us to interrupt mainloops
     io_wait in case some data is available to send out */
  if( pipe( g_self_pipe ) == -1 )
    panic( "selfpipe failed: " );
  if( !io_fd( g_self_pipe[0] ) )
    panic( "selfpipe io_fd failed: " );
  io_setcookie( g_self_pipe[0], (void*)FLAG_SELFPIPE );
  io_wantread( g_self_pipe[0] );

  defaul_signal_handlers( );
  /* Init all sub systems. This call may fail with an exit() */
  trackerlogic_init( );

  if( statefile )
    load_state( statefile );

  install_signal_handlers( );

  /* Kick off our initial clock setting alarm */
  alarm(5);

  server_mainloop( 0 );

  return 0;
}

const char *g_version_opentracker_c = "$Source: /home/cvsroot/opentracker/opentracker.c,v $: $Revision: 1.238 $\n";
