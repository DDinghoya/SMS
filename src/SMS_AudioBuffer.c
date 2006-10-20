/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
# include "SMS.h"
# include "SMS_AudioBuffer.h"
# include "SMS_Data.h"

static SMS_AudioBuffer s_AudioBuffer;

#include <kernel.h>

static SMS_AudioBuffer s_AudioBuffer;
static int             s_Sema;

#ifdef LOCK_QUEUES
static int s_SemaLock;
# define LOCK() WaitSema ( s_SemaLock )
# define UNLOCK() SignalSema ( s_SemaLock )
#else
# define LOCK()
# define UNLOCK()
#endif  /* LOCK_QUEUES */

static unsigned char* _sms_audio_buffer_alloc ( int aSize, unsigned char** appPtr ) {

 unsigned char* lpPtr;
 int            lSize;

 LOCK(); {

  *appPtr = s_AudioBuffer.m_pInp;

  lSize = ( aSize + 71 ) & 0xFFFFFFC0;
  lpPtr = s_AudioBuffer.m_pInp + lSize;

  if (  SMS_RB_EMPTY( s_AudioBuffer )  ) goto ret;

  while ( 1 ) {

   if ( s_AudioBuffer.m_pInp > s_AudioBuffer.m_pOut )

    if ( lpPtr < s_AudioBuffer.m_pEnd ) {
ret:
     *( int* )s_AudioBuffer.m_pInp             = aSize;
     *( int* )( s_AudioBuffer.m_pInp + lSize ) = -1;

     UNLOCK();

     return s_AudioBuffer.m_pInp + 4;

    } else {

     s_AudioBuffer.m_pInp = s_AudioBuffer.m_pBeg;
     lpPtr                = s_AudioBuffer.m_pBeg + lSize;

    }  /* end else */

   else if ( lpPtr < s_AudioBuffer.m_pOut )

    goto ret;

   else {

    s_AudioBuffer.m_fWait = 1;

    UNLOCK();
     WaitSema ( s_Sema );
    LOCK();

   }  /* end else */

  }  /* end while */

 } UNLOCK();

}  /* end _sms_audio_buffer_alloc */

static void _sms_audio_buffer_free ( unsigned char* apPtr ) {

 s_AudioBuffer.m_pInp = apPtr;

}  /* end _sms_audio_buffer_free */

static int _sms_audio_buffer_release ( void ) {

 LOCK(); {

  int lSize = *( int* )s_AudioBuffer.m_pOut;

  SMS_ADV_ABOUT( &s_AudioBuffer, lSize );

  if (  *( int* )s_AudioBuffer.m_pOut == -1  ) {

   if (  SMS_RB_EMPTY( s_AudioBuffer )  ) {

    UNLOCK();
    return 1;

   }  /* end if */

   s_AudioBuffer.m_pOut = s_AudioBuffer.m_pBeg;

  }  /* end if */

  if ( s_AudioBuffer.m_fWait ) {

   s_AudioBuffer.m_fWait = 0;
   SignalSema ( s_Sema );

  }  /* end if */

 } UNLOCK();

 return 0;

}  /* end _sms_audio_buffer_release */

static void _sms_audio_buffer_destroy ( void ) {

 DeleteSema ( s_Sema );
#ifdef LOCK_QUEUES
 DeleteSema ( s_SemaLock );
#endif  /* LOCK_QUEUES */
}  /* end _sms_audio_buffer_destroy */

static void _sms_audio_buffer_reset ( void ) {

 s_AudioBuffer.m_pInp  =
 s_AudioBuffer.m_pOut  =
 s_AudioBuffer.m_pBeg  = UNCACHED_SEG( AUD_BUFF                              );
 s_AudioBuffer.m_pEnd  = UNCACHED_SEG( &g_DataBuffer[ SMS_DATA_BUFFER_SIZE ] );
 s_AudioBuffer.m_pPos  = NULL;
 s_AudioBuffer.m_Len   = 0;
 s_AudioBuffer.m_fWait = 0;

}  /* end _sms_audio_buffer_reset */

SMS_AudioBuffer* SMS_InitAudioBuffer ( void ) {

 ee_sema_t lSema;

 lSema.init_count = 0;
 lSema.max_count  = 1;
 s_Sema = CreateSema ( &lSema );
# ifdef LOCK_QUEUES
 lSema.init_count = 1;
 s_SemaLock = CreateSema ( &lSema );
# endif  /* LOCK_QUEUES */
 s_AudioBuffer.Alloc   = _sms_audio_buffer_alloc;
 s_AudioBuffer.Free    = _sms_audio_buffer_free;
 s_AudioBuffer.Release = _sms_audio_buffer_release;
 s_AudioBuffer.Destroy = _sms_audio_buffer_destroy;
 s_AudioBuffer.Reset   = _sms_audio_buffer_reset;

 _sms_audio_buffer_reset ();

 return &s_AudioBuffer;

}  /* end SMS_InitAudioBuffer */

