#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vlc/vlc.h>
#include <vlc/plugins/vlc_es.h>

FILE *g_file = NULL;
int64_t g_pts = 0;

#define ARG_SIZE 14

int imem_get_data(void *arg, const char *cookie, int64_t *dts,  int64_t *pts, 
		  unsigned *flags, size_t *size, void **data)
{
  *size = 576;
  *data = calloc(1, *size);

  int rc = fread(*data, *size, 1, g_file);
  if(!rc)
    return -1;

  *pts = g_pts;
  *dts = g_pts;

  g_pts += 1152*2*8;

  return 0;
}

int imem_del_data(void *arg, const char *cookie, size_t size, void *data)
{
  free(data);

  return 0;
}

void omem_handle_packet(void *arg, int id, const block_t *block)
{
  printf("handle packet\n");
}

void omem_add_stream(void *arg, int index, const es_format_t *p_fmt)
{
  printf("add stream\n");
}

void omem_del_stream(void *arg, int index)
{
  printf("delete stream\n");
}

int main(int argc, char **argv)
{
  libvlc_instance_t *inst;
  libvlc_media_player_t *mp;
  libvlc_media_t *m;

  int i;
  char* arg[ARG_SIZE];

  g_file = fopen("audio.mp2", "rb");
  if(g_file == NULL) {
    printf("Can't open file\n");
    return -1;
  }

  for(i=0; i<ARG_SIZE; i++){
    arg[i] = (char*) malloc(128);
  }

  i = 0;
  sprintf(arg[i++], "-I=dummy");
  sprintf(arg[i++], "--ignore-config");
  sprintf(arg[i++], "--sout=#transcode{acodec=vorb}:omem");
  sprintf(arg[i++], "--imem-get=%ld" , (long) imem_get_data);
  sprintf(arg[i++], "--imem-release=%ld", (long) imem_del_data);
  sprintf(arg[i++], "--imem-codec=mp2a");
  sprintf(arg[i++], "--imem-cat=1"); // audio=1, video=2
 
  sprintf(arg[i++], "--omem-handle-packet=%ld" , (long) omem_handle_packet);
  sprintf(arg[i++], "--omem-add-stream=%ld", (long) omem_add_stream);
  sprintf(arg[i++], "--omem-del-stream=%ld", (long) omem_del_stream);
  sprintf(arg[i++], "--omem-argument=%ld", 0);

  sprintf(arg[i++], "-vvv" );

  // load the vlc engine
  inst = libvlc_new(ARG_SIZE,(const char * const*) arg);

  // create a new item
  m = libvlc_media_new_path(inst, "imem://");

  // create a media play playing environment
  mp = libvlc_media_player_new_from_media(m);

  // no need to keep the media now
  libvlc_media_release(m);

  // play the media_player
  libvlc_media_player_play(mp);

  sleep(10);

  // stop playing
  libvlc_media_player_stop(mp);

  // free the media_player
  libvlc_media_player_release(mp);

  libvlc_release(inst);


  return 0;
}
