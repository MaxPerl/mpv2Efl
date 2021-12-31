// Build with: gcc -o mpv mpv.c `pkg-config --cflags --libs mpv elementary`

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <Elementary.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>

static int render_event;
typedef struct _cb_data cbd;

struct _cb_data
{
   Evas_Object *glview;
   mpv_handle   *mpv;
   mpv_render_context *mpv_gl;
};

static cbd *cb_data = NULL;

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void *get_proc_address(void *fn_ctx, const char *name)
{
    Evas_Object *glview = fn_ctx;
    Evas_GL *gl = elm_glview_evas_gl_get(glview);
    void *addr = evas_gl_proc_address_get(gl,name);
    return addr;
}

void _render_update(void *ctx) {
    printf("CALL RENDER\n");
    ecore_event_add(render_event,NULL, NULL,NULL); 
    //evas_object_smart_callback_call(gl,"render",NULL);
}

Eina_Bool _on_render(void *data, int type, void *event)
{
    mpv_render_context *mpv_gl = cb_data->mpv_gl;
    Evas_Object *gl = cb_data->glview;
        
        int w, h;
    
        elm_glview_size_get(gl, &w, &h);
        printf("RENDER %d %d\n",w,h);
        
        mpv_render_param params[] = {
            // Specify the default framebuffer (0) as target. This will
            // render onto the entire screen. If you want to show the video
            // in a smaller rectangle or apply fancy transformations, you'll
            // need to render into a separate FBO and draw it manually.
            {MPV_RENDER_PARAM_OPENGL_FBO, &(mpv_opengl_fbo){
                .fbo = 0,
                .w = w,
                .h = h,
            }}
        };
        // See render_gl.h on what OpenGL environment mpv expects, and
        // other API details.
        mpv_render_context_render(mpv_gl, params);
        
}

static Eina_Bool
on_mpv(void *data)
{
    mpv_handle *mpv = data;
    
    while (1) {
                    mpv_event *mp_event = mpv_wait_event(mpv, 0);
                    if (mp_event->event_id == MPV_EVENT_NONE)
                        break;
                }
   // Let the event continue to other callbacks which have not been called yet
   return ECORE_CALLBACK_PASS_ON;
}

void _init_gl(Evas_Object *gl) {
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &(mpv_opengl_init_params){
            .get_proc_address = get_proc_address, gl,
        }}
    };

    // This makes mpv use the currently set GL context. It will use the callback
    // (passed via params) to resolve GL builtin functions, as well as extensions.
    
    mpv_render_context *mpv_gl;
    if (mpv_render_context_create(&mpv_gl, cb_data->mpv, params) < 0)
        die("failed to initialize mpv GL context");
    
    render_event = ecore_event_type_new();
    ecore_event_handler_add(render_event,_on_render,NULL);
    
    cb_data->mpv_gl = mpv_gl;
    
    // When normal mpv events are available.
    //mpv_set_wakeup_callback(mpv, on_mpv_events, NULL);

    // When there is a need to call mpv_render_context_update(), which can
    // request a new frame to be rendered.
    // (Separate from the normal event handling mechanism for the sake of
    //  users which run OpenGL on a different thread.)
    mpv_render_context_set_update_callback(mpv_gl, _render_update, NULL);
    
}

void _draw_gl(Evas_Object *gl) {
    
    
}


int main(int argc, char *argv[])
{
    Evas_Object *win, *bx, *gl;
    if (!(cb_data = calloc(1, sizeof(cbd)))) return 1;
    
    //elm_config_engine_set("opengl_x11");
    //elm_config_accel_preference_set("opengl");
    
    elm_init(argc,argv);
    elm_config_accel_preference_set("opengl_x11");
    
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    win = elm_win_util_standard_add("glview simple", "GLView Simple");
    elm_win_autodel_set(win, EINA_TRUE);
   
    bx = elm_box_add(win);
    evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, bx);
    evas_object_show(bx);

    //-//-//-// THIS IS WHERE GL INIT STUFF HAPPENS (ALA EGL)
    //-//
    // create a new glview object
    gl = elm_glview_add(win);
    evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    // mode is simply for supporting alpha, depth buffering, and stencil
   // buffering.
   elm_glview_mode_set(gl, ELM_GLVIEW_DEPTH_24);
   // resize policy tells glview what to do with the surface when it
   // resizes.  ELM_GLVIEW_RESIZE_POLICY_RECREATE will tell it to
   // destroy the current surface and recreate it to the new size
   elm_glview_resize_policy_set(gl, ELM_GLVIEW_RESIZE_POLICY_RECREATE);
   // render policy tells glview how it would like glview to render
   // gl code. ELM_GLVIEW_RENDER_POLICY_ON_DEMAND will have the gl
   // calls called in the pixel_get callback, which only gets called
   // if the object is visible, hence ON_DEMAND.  ALWAYS mode renders
   // it despite the visibility of the object.
   elm_glview_render_policy_set(gl, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
   // initialize callback function gets registered here
   elm_glview_init_func_set(gl, _init_gl);
   elm_glview_render_func_set(gl, _draw_gl);
    
    elm_box_pack_end(bx, gl);
    evas_object_show(gl);

    elm_object_focus_set(gl, EINA_TRUE);
    
    evas_object_resize(win, 320, 480);
    evas_object_show(win);
    
    
    mpv_handle *mpv = mpv_create();
    if (!mpv)
        die("context init failed");
    
    ecore_idler_add(on_mpv,mpv);
    
    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0)
        die("mpv init failed");
    
    mpv_request_log_messages(mpv, "debug");
    
    cb_data->glview = gl;
    cb_data->mpv = mpv;
    
    // Play this file.
     const char *cmd[] = {"loadfile", "./video.ogv", NULL};
     mpv_command_async(mpv,0, cmd);
    
    // run the mainloop and process events and callbacks
    elm_run();
            
    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.
    //mpv_render_context_free(mpv_gl);

    //mpv_detach_destroy(mpv);
    elm_shutdown();

    printf("properly terminated\n");
    return 0;
}
