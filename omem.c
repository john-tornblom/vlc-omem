/*****************************************************************************
 * omem.c: Memory stream output module
 *****************************************************************************
 * Copyright (C) 2003-2004 the VideoLAN team
 * $Id: 6dbed32e35d319f420547a3d9aecfaa02711c066 $
 *
 * Authors: John Törnblom <john.tornblom@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_block.h>
#include <vlc_sout.h>

/*****************************************************************************
 * Exported prototypes
 *****************************************************************************/
static int      Open    ( vlc_object_t * );
static void     Close   ( vlc_object_t * );

static sout_stream_id_t *Add ( sout_stream_t *, es_format_t * );
static int               Del ( sout_stream_t *, sout_stream_id_t * );
static int               Send( sout_stream_t *, sout_stream_id_t *, block_t* );


/*****************************************************************************
 * Exported API
 *****************************************************************************/

typedef void (*omem_handle_block_t) (void *arg, int id, const block_t *block);
typedef void (*omem_add_stream_t)   (void *arg, int id, 
				     const es_format_t *p_stream);
typedef void (*omem_del_stream_t)   (void *arg, int id);

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
struct sout_stream_sys_t {
  size_t              i_stream_count;
  void               *p_argument;
  omem_handle_block_t fp_handle_block_cb;
  omem_add_stream_t   fp_add_stream_cb;
  omem_del_stream_t   fp_del_stream_cb;

};

struct sout_stream_id_t {
  int      i_id;
  uint32_t i_codec;
  int      i_cat;

  union {
    struct {
      /* Audio specific */
      unsigned    i_channels;
      unsigned    i_rate;
    } audio;
    struct {
      /* Video specific */
      unsigned    i_height;
      unsigned    i_width;
    } video;
  } u;
};

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin ()
    set_description( ("Memory stream output") )
    set_capability( "sout stream", 50 )
    add_shortcut( "omem" )

    add_string( "omem-handle-block", "", "", "", true )
    add_string( "omem-argument", "", "", "", true )
    add_string( "omem-add-stream", "", "", "", true )
    add_string( "omem-del-stream", "", "", "", true )
    set_callbacks( Open, Close )
vlc_module_end ()

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    char *tmp;
    sout_stream_t *p_stream = (sout_stream_t*)p_this;
    sout_stream_sys_t *p_sys = calloc(1, sizeof(sout_stream_sys_t));
    if (!p_sys)
        return VLC_ENOMEM;

    tmp = var_InheritString(p_this, "omem-handle-block");
    if (tmp)
      p_sys->fp_handle_block_cb = (omem_handle_block_t)
	(intptr_t)strtoll(tmp, NULL, 0);
    free(tmp);

    tmp = var_InheritString(p_this, "omem-add-stream");
    if (tmp)
      p_sys->fp_add_stream_cb = (omem_add_stream_t)
	(intptr_t)strtoll(tmp, NULL, 0);
    free(tmp);

    tmp = var_InheritString(p_this, "omem-del-stream");
    if (tmp)
      p_sys->fp_del_stream_cb = (omem_del_stream_t)
	(intptr_t)strtoll(tmp, NULL, 0);
    free(tmp);

    tmp = var_InheritString(p_this, "omem-argument");
    if (tmp)
      p_sys->p_argument = (void *)(uintptr_t)strtoull(tmp, NULL, 0);
    free(tmp);

    if( !p_sys->fp_handle_block_cb || !p_sys->fp_add_stream_cb || 
       !p_sys->fp_del_stream_cb ) {
      msg_Err( p_this, "Invalid block/stream function pointers" );
      free(p_sys);
      return VLC_EGENERIC;
    }

    p_stream->pf_add    = Add;
    p_stream->pf_del    = Del;
    p_stream->pf_send   = Send;
    p_stream->p_sys     = p_sys;

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close:
 *****************************************************************************/
static void Close( vlc_object_t * p_this )
{
    sout_stream_t *p_stream = (sout_stream_t*)p_this;
    free( p_stream->p_sys );
}

/*****************************************************************************
 * Add:
 *****************************************************************************/
static sout_stream_id_t *Add( sout_stream_t *p_stream, es_format_t *p_fmt )
{
    sout_stream_id_t *id;
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    
    if( !p_sys->fp_add_stream_cb )
      return NULL;

    id = calloc( 1, sizeof( sout_stream_id_t ) );
    if( !id )
      return NULL;

    id->i_id    = p_sys->i_stream_count++;
    id->i_codec = p_fmt->i_codec;
    id->i_cat   = p_fmt->i_cat;

    switch(id->i_cat)
      {
      case VIDEO_ES:
	id->u.video.i_height = p_fmt->video.i_height;
	id->u.video.i_width = p_fmt->video.i_width;
	break;
      case AUDIO_ES:
	id->u.audio.i_channels = p_fmt->audio.i_channels;
	id->u.audio.i_rate = p_fmt->audio.i_rate;
	break;
      default:
	break;
      }

    p_sys->fp_add_stream_cb( p_sys->p_argument, id->i_id, p_fmt );

    return id;
}

/*****************************************************************************
 * Del:
 *****************************************************************************/
static int Del( sout_stream_t *p_stream, sout_stream_id_t *id )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    if( !p_sys->fp_del_stream_cb )
      return VLC_EGENERIC;

    p_sys->fp_del_stream_cb( p_sys->p_argument, id->i_id );

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Send:
 *****************************************************************************/
static int Send( sout_stream_t *p_stream, sout_stream_id_t *id,
                 block_t *p_buffer )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    if( !p_sys->fp_handle_block_cb )
      return VLC_EGENERIC;

    p_sys->fp_handle_block_cb( p_sys->p_argument, id->i_id, p_buffer );

    block_ChainRelease( p_buffer );

    return VLC_SUCCESS;
}

