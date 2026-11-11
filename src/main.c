/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <banana.h>
#include <libretro.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

static bwin_t window;
static bool running=true, failed=false;
static retro_keyboard_event_t keyboard;
static unsigned char held[RETROK_LAST];
static int mouse_x, mouse_y, buttons;
static double fps=60.0;
static int sample_rate=44100;
static char directory[512], content[512];
static const struct retro_core_option_definition *options;
static unsigned video_frames,audio_frames,audio_nonzero;
static unsigned presented_frames, late_frames, queue_peak_ms;
static unsigned render_ms, audio_ms;
static bool audio_available = true;

/* This core supplies a static software framebuffer. It remains valid while
   we queue the audio and present the frame, before the next retro_run. */
static const uint32_t *frame;
static unsigned frame_width, frame_height;
static size_t frame_pitch;
static bool frame_dirty;
static unsigned *source_columns;
static unsigned map_width, map_source_width;
static int mapped_window_width, mapped_window_height;

/* Include the driver's own ring (about 300 ms on Banana OS build-318). */
enum { AUDIO_HIGH_WATER_MS = 410, STARTUP_LEAD_MS = 45, MAX_CATCHUP_MS = 60 };

/* The pinned core ignores its overlay option when using a .conf. Mounts
   should write directly to the game directory chosen in that file. */
extern bool mount_overlay;

static void log_message(enum retro_log_level level, const char *fmt, ...) {
    (void)level;
    va_list args; va_start(args,fmt); vfprintf(stderr,fmt,args); va_end(args);
}
static bool environment(unsigned cmd, void *data) {
    switch(cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback*)data)->log=log_message; return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *(enum retro_pixel_format*)data==RETRO_PIXEL_FORMAT_XRGB8888;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: *(unsigned*)data=1; return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS: options=data; return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
        options=((struct retro_core_options_intl*)data)->us; return true;
    case RETRO_ENVIRONMENT_GET_LANGUAGE: *(unsigned*)data=RETRO_LANGUAGE_ENGLISH; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *v=data;
        v->value=NULL;
        if (!strcmp(v->key,"dosbox_svn_use_options")) v->value="false";
        else if (!strcmp(v->key,"dosbox_svn_core_timing")) v->value="external";
        else if (!strcmp(v->key,"dosbox_svn_save_overlay")) v->value="false";
        else if (options) for (const struct retro_core_option_definition *o=options;o->key;o++)
            if (!strcmp(v->key,o->key)) {v->value=o->default_value;break;}
        return v->value!=NULL;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
    case RETRO_ENVIRONMENT_GET_FASTFORWARDING: *(bool*)data=false; return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY: *(const char**)data=directory; return true;
    case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
        keyboard=((struct retro_keyboard_callback*)data)->callback; return true;
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
        struct retro_system_av_info *av=data;
        if(av->timing.fps<1 || av->timing.fps>240 || av->timing.sample_rate<4000 || av->timing.sample_rate>192000) return false;
        fps=av->timing.fps; sample_rate=(int)av->timing.sample_rate; return true;
    }
    case RETRO_ENVIRONMENT_SET_GEOMETRY: return true; /* software scales each frame */
    case RETRO_ENVIRONMENT_SET_MESSAGE:
        fprintf(stderr,"DOSBox: %s\n",((struct retro_message*)data)->msg); return true;
    case RETRO_ENVIRONMENT_SHUTDOWN: running=false; return true;
    default: return false;
    }
}
static int native_key(unsigned key) {
    if(key>=32 && key<=126) return (int)key;
    if(key>=RETROK_F1 && key<=RETROK_F12) return BANANA_KEY_F1+key-RETROK_F1;
    switch(key) {
    case RETROK_RETURN:return '\n'; case RETROK_TAB:return '\t';
    case RETROK_BACKSPACE:return '\b'; case RETROK_ESCAPE:return 27;
    case RETROK_UP:return BANANA_KEY_UP; case RETROK_DOWN:return BANANA_KEY_DOWN;
    case RETROK_LEFT:return BANANA_KEY_LEFT; case RETROK_RIGHT:return BANANA_KEY_RIGHT;
    case RETROK_HOME:return BANANA_KEY_HOME; case RETROK_END:return BANANA_KEY_END;
    case RETROK_PAGEUP:return BANANA_KEY_PGUP; case RETROK_PAGEDOWN:return BANANA_KEY_PGDN;
    case RETROK_DELETE:return BANANA_KEY_DELETE;
    case RETROK_LSHIFT:return BANANA_KEY_SHIFT;
    case RETROK_LCTRL:return BANANA_KEY_CTRL; case RETROK_LALT:return BANANA_KEY_ALT;
    default:return 0;
    }
}
static void poll_input(void) {
    banana_event_t ev;
    mouse_x=mouse_y=0;
    while(bwin_event(&window,&ev)) {
        if(ev.type==BANANA_EV_CLOSE) running=false;
        if(ev.type==BANANA_EV_RESIZE && frame) frame_dirty=true;
        if(ev.type==BANANA_EV_MOUSE_DOWN) {
            if(!bwin_mouse_captured(&window)) bwin_capture_mouse(&window,1);
            if(ev.button>=1 && ev.button<=3) buttons|=1<<(ev.button-1);
        }
        if(ev.type==BANANA_EV_MOUSE_UP && ev.button>=1 && ev.button<=3) buttons&=~(1<<(ev.button-1));
        if(ev.type==BANANA_EV_MOUSE_REL) {mouse_x+=ev.x;mouse_y+=ev.y;}
        if((ev.type==BANANA_EV_CAPTURE || ev.type==BANANA_EV_FOCUS) && !ev.x) buttons=0;
    }
    for(unsigned k=0;k<RETROK_LAST;k++) {
        int n=native_key(k); bool down=n && banana_key_down(n);
        if(down!=held[k]) {held[k]=down;if(keyboard) keyboard(down,k,0,0);}
    }
}
static int16_t input(unsigned port,unsigned device,unsigned index,unsigned id) {
    (void)index;
    if(port) return 0;
    if(device==RETRO_DEVICE_KEYBOARD) return id<RETROK_LAST ? held[id]:0;
    if(device==RETRO_DEVICE_MOUSE) {
        switch(id) {
        case RETRO_DEVICE_ID_MOUSE_X:return mouse_x < -32768 ? -32768 : mouse_x>32767 ? 32767:mouse_x;
        case RETRO_DEVICE_ID_MOUSE_Y:return mouse_y < -32768 ? -32768 : mouse_y>32767 ? 32767:mouse_y;
        case RETRO_DEVICE_ID_MOUSE_LEFT:return !!(buttons&1);
        case RETRO_DEVICE_ID_MOUSE_RIGHT:return !!(buttons&2);
        case RETRO_DEVICE_ID_MOUSE_MIDDLE:return !!(buttons&4);
        }
    }
    return 0;
}
static void video(const void *data,unsigned w,unsigned h,size_t pitch) {
    if(!data) return;
    video_frames++;
    if(data==RETRO_HW_FRAME_BUFFER_VALID || !w || !h || w>2048 || h>2048 ||
       pitch<w*4 || pitch%sizeof(uint32_t)) {
        fprintf(stderr,"DOSBox: invalid software frame\n"); failed=true;running=false;return;
    }
    frame = data;
    frame_width = w;
    frame_height = h;
    frame_pitch = pitch;
    frame_dirty = true;
}

static void present_video(void) {
    unsigned start = banana_ticks();
    int dw=window.w,dh=dw*3/4;
    if(dh>window.h) {dh=window.h;dw=dh*4/3;}
    if(!window.px || dw <= 0 || dh <= 0) return;
    int x0=(window.w-dw)/2,y0=(window.h-dh)/2;
    if(mapped_window_width != window.w || mapped_window_height != window.h) {
        bwin_clear(&window,0);
        mapped_window_width = window.w;
        mapped_window_height = window.h;
    }
    if((unsigned)dw != map_width || frame_width != map_source_width) {
        if((unsigned)dw != map_width) {
            unsigned *columns = realloc(source_columns, (size_t)dw * sizeof(*columns));
            if(!columns) {
                fprintf(stderr,"DOSBox: cannot allocate video scale table\n");
                failed=true;
                running=false;
                return;
            }
            source_columns = columns;
            map_width = dw;
        }
        for(int x=0;x<dw;x++) source_columns[x] = (size_t)x * frame_width / dw;
        map_source_width = frame_width;
    }
    unsigned last_row = frame_height;
    for(int y=0;y<dh;y++) {
        uint32_t *dst = window.px + (size_t)(y + y0) * window.w + x0;
        unsigned row_index = (size_t)y * frame_height / dh;
        const uint32_t *row = (const uint32_t*)((const char*)frame + row_index * frame_pitch);
        if(row_index == last_row) {
            memcpy(dst, dst - window.w, (size_t)dw * sizeof(*dst));
        } else if((unsigned)dw == frame_width) {
            memcpy(dst, row, (size_t)dw * sizeof(*dst));
        } else {
            for(int x=0;x<dw;x++) dst[x] = row[source_columns[x]];
        }
        last_row = row_index;
    }
    bwin_update(&window);
    frame_dirty = false;
    presented_frames++;
    render_ms += banana_ticks() - start;
}
static size_t audio_batch(const int16_t *data,size_t frames) {
    if(!running || !audio_available || !frames) return frames;
    unsigned start = banana_ticks();
    audio_frames+=(unsigned)frames;
    for(size_t i=0;i<frames*2;i++) if(data[i]) {audio_nonzero++;break;}
    if(banana_play(data,frames*4,sample_rate,2,16)<0) {
        audio_available = false;
        fprintf(stderr,"DOSBox: audio device rejected PCM output\n");
    }
    audio_ms += banana_ticks() - start;
    return frames;
}
static void audio_sample(int16_t left,int16_t right) {int16_t p[2]={left,right};audio_batch(p,1);}

static void print_usage(FILE *stream) {
    fputs("Usage: dosbox [--frames N] PATH\n"
          "  PATH        A DOS .exe, .com, .bat, or DOSBox .conf file\n"
          "  --help      Show this help\n"
          "  --version   Show the Banana OS port version\n"
          "  --selftest  Run the runtime checks\n"
          "\nJazz Jackrabbit: copy /apps/dosbox/jazz.conf next to JAZZ.EXE,\n"
          "then run dosbox /path/to/Jazz/jazz.conf.\n", stream);
}

int main(int argc,char **argv) {
    /* The SDK initializes libc but does not call C++ global constructors. */
    extern void (*__init_array_start[])(void), (*__init_array_end[])(void);
    for(void (**p)(void)=__init_array_start;p<__init_array_end;p++) (*p)();
    if (argc == 2 && !strcmp(argv[1], "--version")) {
        puts("DOSBox for Banana OS " DOSBOX_PORT_VERSION);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--help")) {
        print_usage(stdout);
        return 0;
    }
    if(argc==2 && !strcmp(argv[1],"--selftest")) {
        extern int dosbox_runtime_selftest(void);
        return dosbox_runtime_selftest();
    }
    if(!BANANA_HAS(banana_api(),key_down) || banana_api()->version<20) {
        fprintf(stderr,"DOSBox requires Banana OS API 20 or later (x86_64).\n");return 1;
    }
    unsigned frame_limit=0;
    const char *path=argc==2?argv[1]:NULL;
    if(argc==4 && !strcmp(argv[1],"--frames")) {
        char *end;unsigned long count=strtoul(argv[2],&end,10);
        if(*end || !count || count>1000000) {fprintf(stderr,"Invalid frame limit\n");return 1;}
        frame_limit=(unsigned)count;path=argv[3];
    }
    if(!path) {print_usage(stderr);return 1;}
    if(!realpath(path,content)) {perror(path);return 1;}
    struct stat st;
    if(stat(content,&st) || !S_ISREG(st.st_mode)) {fprintf(stderr,"Not a regular file: %s\n",content);return 1;}
    snprintf(directory,sizeof(directory),"%s",content);
    char *slash=strrchr(directory,'/'); if(slash) {if(slash==directory) slash[1]=0;else *slash=0;}
    if(bwin_open(&window,"DOSBox - Banana OS",480,360)) {fprintf(stderr,"Cannot open window: start the desktop with startx.\n");return 1;}
    bwin_resizable(&window,320,240);
    retro_set_environment(environment);retro_set_video_refresh(video);
    retro_set_audio_sample(audio_sample);retro_set_audio_sample_batch(audio_batch);
    retro_set_input_poll(poll_input);retro_set_input_state(input);
    retro_init();
    struct retro_game_info game={content,NULL,0,NULL};
    if(!retro_load_game(&game)) {fprintf(stderr,"DOSBox failed to load %s\n",content);failed=true;running=false;}
    mount_overlay = false;
    retro_set_controller_port_device(0,RETRO_DEVICE_KEYBOARD);
    struct retro_system_av_info av;retro_get_system_av_info(&av);
    fps=av.timing.fps; sample_rate=(int)av.timing.sample_rate;
    unsigned start=banana_ticks();
    double deadline=-STARTUP_LEAD_MS;
    unsigned last_present=start;
    unsigned steps=0;
    while(running) {
        retro_run();
        ++steps;
        if(!running) break;

        double period = 1000.0/fps;
        deadline += period;
        unsigned ticks = banana_ticks();
        double now = ticks - start;
        bool behind = now > deadline + period;
        if(behind && now > STARTUP_LEAD_MS * 2) late_frames++;
        /* On a slow frame, prioritize the emulator and sound. Keep drawing
           at least every ~33 ms while the game is producing new frames. */
        if(frame_dirty && (!behind || !presented_frames || ticks - last_present >= 33)) {
            present_video();
            last_present = ticks;
        }
        if(frame_limit && steps>=frame_limit) break;

        unsigned queued = audio_available ? banana_audio_queued_ms() : 0;
        if(queued > queue_peak_ms) queue_peak_ms = queued;
        if(queued > AUDIO_HIGH_WATER_MS) {
            /* Slow the producer instead of dropping already queued PCM. */
            unsigned wait = queued - AUDIO_HIGH_WATER_MS;
            banana_sleep(wait > 20 ? 20 : wait);
            now = banana_ticks() - start;
            if(deadline < now) deadline = now;
        }
        now = banana_ticks() - start;
        if(deadline>now) banana_sleep((unsigned)(deadline-now));
        else if(now-deadline>MAX_CATCHUP_MS) deadline=now-MAX_CATCHUP_MS;
    }
    unsigned elapsed = banana_ticks() - start;
    retro_unload_game();retro_deinit();
    if(audio_frames && audio_available) banana_audio_stop();
    bwin_close(&window);
    free(source_columns);
    printf("DOSBOX_SESSION: steps=%u target_fps=%.2f elapsed_ms=%u late=%u video=%u shown=%u render_ms=%u audio_frames=%u nonzero_audio_batches=%u audio_ms=%u queue_peak_ms=%u status=%s\n",
           steps,fps,elapsed,late_frames,video_frames,presented_frames,render_ms,
           audio_frames,audio_nonzero,audio_ms,queue_peak_ms,failed?"failed":"closed");
    return failed?1:0;
}
