
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>
#include <mikmod.h>

#include "version.h"
#include "flashcart.h"

#include "gamestate.h"
#include "engine.h"


//tbh I'm not sure what's going on here, I just lifted this from the libdragon dfs example
MIKMODAPI extern UWORD md_mode __attribute__((section (".data")));
MIKMODAPI extern UWORD md_mixfreq __attribute__((section (".data")));

volatile uint32_t animcounter = 0;
volatile bool game_over = false;

void update_counter( int ovfl )
{
    animcounter++;
}

void init_all_systems(void);
void draw_intros(void);

int main(void)
{
    init_all_systems();
    int state = INTROS;
    GAME* game;

    while(1)
    {
        switch(state)
        {
            case(INTROS):
                draw_intros();
                state = MAIN_MENU;
                break;
            case(MAIN_MENU):
                state = PREP_GAME;
                break;
            case(PREP_GAME):
                game = setup_main_game();
                state = MAIN_GAME;
                break;
            case(MAIN_GAME):
                update_audio(game);
                update_controller(game);
                update_logic(game);
                update_graphics(game);

                if(game_over)
                {
                    game_over = false;
                    state = RESET;
                }
                break;
            case(RESET):
                cleanup_main_game(game);
                state = INTROS;
                break;
            default:
                state = INTROS;
                break;
        }

    }    

}

void init_all_systems()
{
    /* enable interrupts (on the CPU) */
    init_interrupts();

    /* Initialize peripherals */
    dfs_init( DFS_DEFAULT_LOCATION );
    rdp_init();
    controller_init();
    timer_init();
    audio_init(44100, 2);
    MikMod_RegisterAllDrivers();
    MikMod_RegisterAllLoaders();

    md_mode |= DMODE_16BITS;
    md_mode |= DMODE_SOFT_MUSIC;
    md_mode |= DMODE_SOFT_SNDFX;
    md_mode |= DMODE_STEREO;
                                            
    md_mixfreq = audio_get_frequency();

    MikMod_Init("");
    MikMod_SetNumVoices(-1, MAX_SFX);
    MikMod_EnableOutput();
    display_init( RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE );

    //start a timer that will trigger 30 times per second
    //the "update_counter" function is called each time the timer is triggered
    new_timer(TIMER_TICKS(1000000 / 30), TF_CONTINUOUS, update_counter);
}

void draw_intros()
{
    int fp = 0;
    int logo = 0;
    uint32_t logocounter = animcounter;
    fp = dfs_open("/jamlogo.sprite");
    sprite_t *jamlogo = malloc( dfs_size( fp ) );
    dfs_read( jamlogo, 1, dfs_size( fp ), fp );
    dfs_close( fp );

    fp = dfs_open("/n64logo.sprite");
    sprite_t *n64logo = malloc( dfs_size( fp ) );
    dfs_read( n64logo, 1, dfs_size( fp ), fp );
    dfs_close( fp );

    fp = dfs_open("/brewlogo.sprite");
    sprite_t *brewlogo = malloc( dfs_size( fp ) );
    dfs_read( brewlogo, 1, dfs_size( fp ), fp );
    dfs_close( fp );

    /* intro movies */
    do
    {
        static display_context_t disp = 0;

        /* Grab a render buffer */
        while( !(disp = display_lock()) );
       
        /*Fill the screen */
        graphics_fill_screen( disp, 0x00000000 );

        /* Set the text output color */
        graphics_set_color( 0x00000000, 0x00000000 );

		//graphics_draw_text( disp, 16, 16, "N64Brew Jam Test" );

		/* Draw jam logo */
        if(animcounter - logocounter > 120)
        {
            logo++;
            logocounter = animcounter;
        }

            switch(logo)
            {
                case(0):
                case(1):
                    graphics_draw_sprite( disp, 103, 51, n64logo);
                    break;
                case(2):
                    graphics_draw_sprite( disp, 120, 45, brewlogo);
                    graphics_set_color( 0xFFFFFFFF, 0x00000000 );
                    graphics_draw_text( disp, 116, 190, "(C)KIVAN117" );
                    break;
                case(3):
                    graphics_draw_sprite( disp, 8, 79, jamlogo);
                    graphics_set_color( 0xFFFFFFFF, 0x00000000 );
                    graphics_draw_text( disp, 116, 190, "(C)KIVAN117" );
                    break;
                default:
                    break;
            }
            

        /* Force backbuffer flip */
        display_show(disp);
        
        controller_scan();
        struct controller_data keys = get_keys_pressed();
        if( keys.c[0].start || keys.c[0].A )
        {
            logo++;
            /* Lazy switching */
            //mode = 1 - mode;
        }
    } while (logo < 4);

    free(n64logo);
    free(jamlogo);
    free(brewlogo);
}