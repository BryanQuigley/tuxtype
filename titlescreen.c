/***************************************************************************
 -  file: titlescreen.c
 -  description: splash, title and menu screen functionality 
                            ------------------
    begin                : Thur May 4 2000
    copyright            : (C) 2000 by Sam Hart
                         : (C) 2003 by Jesse Andrews
    email                : tuxtype-dev@tux4kids.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "globals.h"
#include "funcs.h"
#include "titlescreen.h"
#include "SDL_extras.h"

/* --- media for menus --- */

/* images of regular and selected text of menu items: */
static SDL_Surface* reg_text[TITLE_MENU_ITEMS + 1][TITLE_MENU_DEPTH + 1] = {NULL};
static SDL_Surface* sel_text[TITLE_MENU_ITEMS + 1][TITLE_MENU_DEPTH + 1] = {NULL};
/* this will contain pointers to all of the menu 'icons' */
static sprite* menu_gfx[TITLE_MENU_ITEMS + 1][TITLE_MENU_DEPTH + 1] = {NULL};

/* Background images for windowed and fullscreen modes: */
static SDL_Surface* win_bkgd = NULL; //640x480 background (windowed)
static SDL_Surface* fullscr_bkgd = NULL; //native resolution (fullscreen)

/* --- other media --- */
static SDL_Surface* title = NULL;
static SDL_Surface* speaker = NULL;
static SDL_Surface* speakeroff = NULL;
static SDL_Surface* profiles = NULL;
static sprite* Tux = NULL;
static Mix_Chunk* snd_move = NULL;
static Mix_Chunk* snd_select = NULL;
static TTF_Font* font = NULL;

/* --- locations we need --- */
static SDL_Rect text_dst[TITLE_MENU_ITEMS + 1];     // location of menu text
static SDL_Rect menu_gfxdest[TITLE_MENU_ITEMS + 1]; // location of menu sprite
static SDL_Rect menu_button[TITLE_MENU_ITEMS + 1];  // menu mouse event buttons
/* keep track of the width of each menu: */
static int menu_width[TITLE_MENU_DEPTH + 1];

static SDL_Rect Tuxdest;
static SDL_Rect Titledest;
static SDL_Rect spkrdest;
static SDL_Rect cursor;
static SDL_Rect profilesdest;

/* Local function prototypes: */
static void show_logo(void);
static int load_media(void);
static void load_menu(void);
static void recalc_rects(void);
static int chooseWordlist(void);
static void not_implemented(void);
static void unload_media(void);
static void unload_menu(void);

/* --- menu text --- */

/* --- define menu structure --- */
/* (these values are all in the Game_Type enum in globals.h) */
const int menu_item[][7]= {{0, 0,         0,         0,          0},
			   {0, CASCADE,   	LEVEL1,    	LEVEL1,	NOT_CODED },
			   {0, LASER,     	LEVEL2,    	LEVEL2,	FREETYPE   },
			   {0, LESSONS,  	LEVEL3,    	LEVEL3,	PROJECT_INFO },
			   {0, WPM,			INSTRUCT,	LEVEL4,	HIGH_SCORES},
			   {0, OPTIONS,   	MAIN,  		MAIN,	SET_LANGUAGE},
			   {0, QUIT_GAME, 	0,      	0,  	MAIN}};

/* --- menu icons --- */ //no icons wpm or highscore
const unsigned char *menu_icon[][7]=
{{"", "", 			"", 		"", 		""},
 {"", "cascade",	"easy",   	"grade1_", 	"list"   },
 {"", "comet",   	"medium", 	"grade2_", 	"practice" },
 {"", "lesson",		"hard",   	"grade3_", 	"keyboard"   },
 {"", "WPM", 		"tutor",  	"grade4_", 	"highscores" },
 {"", "tux_config",	"main",  	"main", 	"lang" },
 {"", "quit",    	"",   		"",    		"main"   }};

/* --- menu text --- */
static const char *menu_text[]=
{{"", "",            					"",             				"",            					""    },
 {"", gettext_noop("Fish Cascade"), 	gettext_noop("Easy"),       	gettext_noop("Space Cadet"), 	gettext_noop("Edit Word Lists")},
 {"", gettext_noop("Comet Zap"),		gettext_noop("Medium"),       	gettext_noop("Pilot"),       	gettext_noop("Practice")},
 {"", gettext_noop("Lessons"),			gettext_noop("Hard"),         	gettext_noop("Ace"),         	gettext_noop("Project Info")},
 {"", gettext_noop("WPM Calculator"),	gettext_noop("Instructions"), 	gettext_noop("Commander"), 		gettext_noop("High Scores")},
 {"", gettext_noop("Options"),      	gettext_noop("Main Menu"), 		gettext_noop("Main Menu"),   	gettext_noop("Setup Language")},
 {"", gettext_noop("Quit"),         	"",    							"",   							gettext_noop("Main Menu")}};



/************************************************************************/
/*                                                                      */ 
/*         "Public" functions (callable throughout program)             */
/*                                                                      */
/************************************************************************/


/****************************************
* TitleScreen: Display the title screen *
*****************************************
* display title screen, get input
*/
void TitleScreen(void)
{

  Uint32 frame = 0;
  Uint32 start = 0;

  /* NOTE for 'depth', think pages like a restaurant menu, */
  /* not heirarchical depth - choice of term is misleading */
  int menu_depth; // how deep we are in the menu

  int i, j, tux_frame = 0;
  int done = 0;
  int firstloop = 1;
  int menu_opt = NONE;
  int sub_menu = NONE;
  int update_locs = 1;
  int redraw = 0;
  int key_menu = 1;
  int old_key_menu = 5;
  wchar_t phrase[128];
  FILE *fp;
  unsigned char fn[FNLEN];
  int found = 0;
  unsigned char str[1000];


  if (settings.sys_sound)
  {
    settings.menu_sound = 1;
    settings.menu_music = 1;
  }

  
  /* FIXME phrase(s) should come from file */

  ConvertFromUTF8(phrase, "The quick brown fox jumps over the lazy dog.");

//  wcscpy(phrase, "Now is the time for all good men to come to the aid of their country.");
  start = SDL_GetTicks();


  /*
  * Display the Standby screen.... 
  */
  show_logo();

  /* Load media and menu data: */
  if (!load_media())
  {
    fprintf(stderr, "TitleScreen - load_media() failed!");
    return;
  }

  SDL_WM_GrabInput(SDL_GRAB_ON); // User input goes to TuxType, not window manager


  /***************************
  * Tux and Title animations *
  ***************************/

  LOG( "->Now Animating Tux and Title onto the screen\n" );

  Tuxdest.x = 0;
  Tuxdest.y = screen->h;
  Tuxdest.w = Tux->frame[0]->w;
  Tuxdest.h = Tux->frame[0]->h;

  Titledest.x = screen->w;
  Titledest.y = 10;
  Titledest.w = title->w;
  Titledest.h = title->h;

  spkrdest.x = screen->w - speaker->w - 10;
  spkrdest.y = screen->h - speaker->h - 10;
  spkrdest.w = speaker->w;
  spkrdest.h = speaker->h;
  
  profilesdest.x = 572;
  profilesdest.y = 420;
  profilesdest.w = profiles->w; 
  profilesdest.h = profiles->h;

  /* --- wait if the first time in the game --- */

  if (settings.show_tux4kids)
  {
    while ((SDL_GetTicks() - start) < 2000)
    {
      SDL_Delay(50);
    }
    settings.show_tux4kids = 0;
  }

  SDL_ShowCursor(1);    
  /* FIXME not sure the next line works in Windows: */
  TransWipe(CurrentBkgd(), RANDOM_WIPE, 10, 20);
  /* Make sure background gets drawn (since TransWipe() doesn't */
  /* seem to work reliably as of yet):                          */
  SDL_BlitSurface(CurrentBkgd(), NULL, screen, NULL);
  SDL_UpdateRect(screen, 0, 0, 0, 0);

  /* --- Pull tux & logo onscreen --- */
  for (i = 0; i <= (PRE_ANIM_FRAMES * PRE_FRAME_MULT); i++)
  {
    start = SDL_GetTicks();
    SDL_BlitSurface(CurrentBkgd(), &Tuxdest, screen, &Tuxdest);
    SDL_BlitSurface(CurrentBkgd(), &Titledest, screen, &Titledest);

    Tuxdest.y -= Tux->frame[0]->h / (PRE_ANIM_FRAMES * PRE_FRAME_MULT);
    Titledest.x -= (screen->w) / (PRE_ANIM_FRAMES * PRE_FRAME_MULT);

    SDL_BlitSurface(Tux->frame[0], NULL, screen, &Tuxdest);
    SDL_BlitSurface(title, NULL, screen, &Titledest);

    SDL_UpdateRect(screen, Tuxdest.x, Tuxdest.y, Tuxdest.w, Tuxdest.h);
    SDL_UpdateRect(screen, Titledest.x, Titledest.y, Titledest.w + 40, Titledest.h);

    while ((SDL_GetTicks() - start) < 33) 
    {
      SDL_Delay(2);
    }
  }

  recalc_rects();

  /* Pick speaker graphic according to whether music is on: */
  if ( settings.menu_music )
    SDL_BlitSurface(speaker, NULL, screen, &spkrdest);
  else
    SDL_BlitSurface(speakeroff, NULL, screen, &spkrdest);

  /* Start playing menu music if desired: */
  if (settings.menu_music)
    MusicLoad( "tuxi.ogg", -1 );

  LOG( "Tux and Title are in place now\n" );

  /* Move mouse to top button: */
  cursor.x = menu_button[1].x + (menu_button[1].w / 2);
  cursor.y = menu_button[1].y + (3 * menu_button[1].h / 4);
  SDL_WarpMouse(cursor.x, cursor.y);
  SDL_WM_GrabInput(SDL_GRAB_OFF);


  /****************************
  * Main Loop Starts Here ... *
  ****************************/


  menu_depth = 1;
  firstloop = 1;
  Tuxdest.y = screen->h - Tux->frame[0]->h;

  profilesSelector();
  
  while (!done) 
  {

    start=SDL_GetTicks();

    /* ---process input queue --- */

    menu_opt = NONE; // clear the option so we don't change twice!

    old_key_menu = key_menu;

    /* Retrieve any user interface events: */
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {

        case SDL_MOUSEMOTION:
        {
          cursor.x = event.motion.x;
          cursor.y = event.motion.y;
          break;
        }


        /* Handle mouse clicks based on mouse location: */
        case SDL_MOUSEBUTTONDOWN:
        {
          cursor.x = event.motion.x;
          cursor.y = event.motion.y;

          for (j = 1; j <= TITLE_MENU_ITEMS; j++)
          {
            if (inRect(menu_button[j], cursor.x, cursor.y))
            {
              menu_opt = menu_item[j][menu_depth];
              if (settings.menu_sound)
              {
                PlaySound(snd_select);
              }
              DEBUGCODE
              {
                fprintf(stderr, "->>BUTTON CLICK menu_opt = %d\n", menu_opt);
                fprintf(stderr, "->J = %d menu_depth=%d\n", j, menu_depth);
              }
            }
          }

          /* If mouse over speaker, toggle menu music off or on: */
          if (inRect(spkrdest, cursor.x, cursor.y))
          {
            if (settings.menu_music)
            {
              MusicUnload();
              settings.menu_music = 0;
            }
            else
            {
              settings.menu_music = 1;
              MusicLoad("tuxi.ogg", -1);
            }
            redraw = 1;
          }
          /* If mouse is over profiles, call selector */
          if ((cursor.x >= profilesdest.x && cursor.x <= (profilesdest.x + profilesdest.w)) &&
        		  (cursor.y >= profilesdest.y && cursor.y <= (profilesdest.y + profilesdest.h)))
          {
        	  profilesSelector();
          }
          break;
        }



        case SDL_QUIT:
        {
          menu_opt = QUIT_GAME;
          break;
        }


        /* Handle key press events based on key value: */
        case SDL_KEYDOWN:
        {
          switch (event.key.keysym.sym)
          {
            case SDLK_ESCAPE:
            {
              /* Go to main menu (if in submenu) or quit: */
              if (menu_depth != 1) 
                menu_opt = MAIN;
              else
                menu_opt = QUIT_GAME;

              if (settings.menu_sound)
                PlaySound(snd_select);
              break;
            }


            /* Toggle screen mode: */
            case SDLK_F10:
            {
              SwitchScreenMode();
              recalc_rects();
              redraw = 1;
              break;
            }


            /* Toggle menu music: */
            case SDLK_F11:
            {
              if (settings.menu_music)
              {
                MusicUnload( );
                settings.menu_music = 0;
              }
              else
              {
                settings.menu_music = 1;
                MusicLoad("tuxi.ogg", -1);
              }
              redraw = 1;
              break;
            }


            /* --- reload translation/graphics/media: for themers/translaters --- */
            case SDLK_F12:
            {
              unload_media();
              LoadLang();
              load_media();
              redraw = 1;
              break;
            }


            case SDLK_UP:
            {
              if (settings.menu_sound)
                PlaySound(snd_move);
              key_menu--;
              if (key_menu < 1)
                key_menu = 5;
              break;
            }


            case SDLK_DOWN:
            {
              key_menu++;
              if (settings.menu_sound)
                PlaySound(snd_move);
              if (key_menu > 5)
                key_menu = 1;
              break;
            }


            case SDLK_RETURN:
            {
              if (key_menu)
              {
                menu_opt = menu_item[key_menu][menu_depth];
                if (settings.menu_sound)
                  PlaySound(snd_select);
              }
              break;
            }


            default:     /* Some other key pressed - do nothing: */
            {
              break;
            }
          }             /* End of switch(event.key.keysym.sym) statement */
        }               /* End of case: SDL_KEYDOWN: */


        default:        /* Some other type of SDL event - do nothing;    */
        {
          break;
        }
      }                 /* End of switch(event.type) statement           */
    }	              /* End of while (SDL_PollEvent(&event)) loop     */



    /* --- warp mouse to follow keyboard input --- */

    if (old_key_menu != key_menu)
    {
      cursor.x = menu_button[key_menu].x + (menu_button[key_menu].w / 2);
      cursor.y = menu_button[key_menu].y + (3 * menu_button[key_menu].h / 4);
      SDL_WarpMouse(cursor.x, cursor.y);
    }



    /* --- do menu processing --- */


    if (menu_opt == QUIT_GAME)
      done = 1;


    if (menu_opt == LASER)
    {
      menu_depth = LASER_SUBMENU;
      sub_menu = LASER;
      update_locs = 1;
      redraw = 1;
    }


    if (menu_opt == CASCADE)
    {
      menu_depth = CASCADE_SUBMENU;
      sub_menu = CASCADE;
      update_locs = 1;
      redraw=1;
    }


    if (menu_opt == OPTIONS)
    {
      menu_depth = OPTIONS_SUBMENU;
      sub_menu = OPTIONS;
      update_locs = 1;
      redraw = 1;
    }


    if (menu_opt == MAIN)
    {
      menu_depth = ROOTMENU;
      update_locs = 1;
      redraw = 1;
    }


    if (menu_opt == NOT_CODED)
    {
      not_implemented();
      redraw = 1;
    }


    if (menu_opt == PROJECT_INFO)
    {
      ProjectInfo();
      redraw = 1;
    }


    if (menu_opt == LESSONS)
    {
      SDL_BlitSurface(CurrentBkgd(), NULL, screen, NULL);
      SDL_Flip( screen );
      unload_media();

      if (settings.menu_music)
        MusicUnload( );

      TestLesson();

      load_media();
      redraw = 1;

      if (settings.menu_music)
        MusicLoad( "tuxi.ogg", -1 );
    }


    if (menu_opt == SET_LANGUAGE)
    {
      unload_media();
      ChooseTheme();
      LoadLang();
      LoadKeyboard();
      load_media();
      redraw = 1;

      if (settings.menu_music)
        MusicLoad( "tuxi.ogg", -1 );
    }


    if (menu_opt == LEVEL1)
    {  
      if (chooseWordlist()) 
      {
        unload_media();

        switch (sub_menu)
        {
          case CASCADE: PlayCascade( EASY ); break;
          case LASER:   PlayLaserGame(  EASY ); break;
        }
      }

      load_media();

      if (settings.menu_music)
        MusicLoad("tuxi.ogg", -1);

      redraw = 1;
    }


    if (menu_opt == LEVEL2)
    {
      if (chooseWordlist())
      {
        unload_media();

        switch (sub_menu)
        {
          case CASCADE: PlayCascade( MEDIUM ); break;
          case LASER:   PlayLaserGame(  MEDIUM ); break;
        }


        if (settings.menu_music)
          MusicLoad( "tuxi.ogg", -1 );
      }

      load_media();
      redraw = 1;
    }



    if (menu_opt == LEVEL3)
    {
      if (chooseWordlist())
      {
        unload_media();

        switch (sub_menu)
        {
          case CASCADE: PlayCascade( HARD ); break;
          case LASER:   PlayLaserGame(  HARD ); break;
        }


        if (settings.menu_music)
          MusicLoad( "tuxi.ogg", -1 );
      }

      load_media();
      redraw = 1;
    }



    if (menu_opt == LEVEL4)
    {
      if (chooseWordlist())
      {
        unload_media();

        switch (sub_menu)
        {
          case CASCADE: PlayCascade( INSANE ); break;
          case LASER:   PlayLaserGame(  INSANE ); break;
        }

        if (settings.menu_music)
          MusicLoad( "tuxi.ogg", -1 );
      }

      load_media();
      redraw = 1;
    }



    if (menu_opt == INSTRUCT)
    {
      unload_media();

      switch (sub_menu)
      {
        case CASCADE: InstructCascade(); break;
        case LASER:   InstructLaser();   break;
      }

      load_media();

      if (settings.menu_music)
        MusicLoad( "tuxi.ogg", -1 );

      redraw = 1;
    }



    if (menu_opt == FREETYPE)
    {
      unload_media();
      found = 0;

      if (!settings.use_english)
      {
        sprintf(fn , "%s/phrases.txt", settings.theme_data_path);
        if (CheckFile(fn))
          found = 1;

        /* Now look in default path if desired or needed: */
        if (!found)
        {
          sprintf(fn , "%s/words/words3.txt", settings.theme_data_path);
          if (CheckFile(fn))
            found = 1;
        }

        if (!found)
        {
          sprintf(fn , "%s/words/words2.txt", settings.theme_data_path);
          if (CheckFile(fn))
            found = 1;
        }

        if (!found)
        {
          sprintf(fn , "%s/words/words1.txt", settings.theme_data_path);
          if (CheckFile(fn))
            found = 1;
        }
      }

      /* Now checking English: */
      if (!found)
      {
        sprintf(fn , "%s/phrases.txt", settings.default_data_path);
        if (CheckFile(fn))
          found = 1;
      }

      /* Now do Phrases activity if phrase loaded successfully: */
      if (found)
      {
        fp=fopen(fn,"r");
        fscanf( fp, "%[^\n]\n", str);
        ConvertFromUTF8(phrase, str);
        Phrases( phrase );
        //Practice();
        load_media();
        redraw = 1;
        fclose(fp);
      }
      else
      {
        fprintf(stderr, "LoadKeyboard(): Error finding file for keyboard setup!\n");
      }
    }
    
    if(menu_opt == WPM)
    {
    	
    }
    
    if(menu_opt == HIGH_SCORES)

    /* ------ End menu_opt processing ----------- */



    if (redraw)
    {
      LOG("TitleScreen() - redraw requested\n");
      recalc_rects();

      SDL_BlitSurface(CurrentBkgd(), NULL, screen, NULL); 
      SDL_BlitSurface(title, NULL, screen, &Titledest);

      if ( settings.menu_music )
        SDL_BlitSurface(speaker, NULL, screen, &spkrdest);
      else
        SDL_BlitSurface(speakeroff, NULL, screen, &spkrdest);

      /* Screen will be updated due to update_locs - see ~30 lines down: */
//      SDL_UpdateRect(screen, 0, 0, 0, 0);
      frame = redraw = 0;   // so we redraw tux
      update_locs = 1;      // so we redraw menu
      firstloop = 1;
    }



    /* --- create new menu screen when needed --- */

    if (update_locs)
    {
      LOG("TitleScreen() - update_locs requested\n");

      /* --- erase the last menu --- */
      for (i = 1; i <= TITLE_MENU_ITEMS; i++)
      {
        text_dst[i].x = screen->w/2 - 70;//290;
        text_dst[i].w = reg_text[i][menu_depth]->w;
        text_dst[i].h = reg_text[i][menu_depth]->h;
        SDL_BlitSurface(CurrentBkgd(), &menu_button[i], screen, &menu_button[i]);
        menu_button[i].w = menu_width[menu_depth] + 20;
      }


      update_locs = 0;

      /* --- draw the full menu --- */

      for (j = 1; j <= TITLE_MENU_ITEMS; j++)
      {
        DOUT(j);
        DrawButton(&menu_button[j], 10, REG_RGBA);
        if (reg_text[j][menu_depth] != NULL)
          SDL_BlitSurface(reg_text[j][menu_depth], NULL, screen, &text_dst[j]);
        if (menu_gfx[j][menu_depth] != NULL)
          SDL_BlitSurface(menu_gfx[j][menu_depth]->default_img, NULL, screen, &menu_gfxdest[j]);
      }

      SDL_UpdateRect(screen, 0, 0, 0, 0);

      LOG("TitleScreen() - update_locs completed\n");
    }



    /* --- make tux blink --- */

    switch (frame % TUX6)
    {
      case 0:    tux_frame = 1; break;
      case TUX1: tux_frame = 2; break;
      case TUX2: tux_frame = 3; break;
      case TUX3: tux_frame = 4; break;			
      case TUX4: tux_frame = 3; break;
      case TUX5: tux_frame = 2; break;
      default: tux_frame = 0;
    }

    if (tux_frame)
    {
      SDL_Rect blink_src, blink_dest;
      blink_src.x = 0;
      blink_src.y = 0;
      blink_src.w = Tuxdest.w;
      blink_src.h = Tuxdest.h;
      blink_dest.x = Tuxdest.x + blink_src.x;
      blink_dest.y = Tuxdest.y + blink_src.y;
      blink_dest.w = blink_src.w;
      blink_dest.h = blink_src.h;
//      SDL_BlitSurface(CurrentBkgd(), , screen, &Tuxdest);
      SDL_BlitSurface(Tux->frame[tux_frame - 1], &blink_src, screen, &blink_dest);
    }


    /* --- check if mouse is in a menu option --- */

    key_menu = 0;

    for (j = 1; j <= TITLE_MENU_ITEMS; j++)
    {
      if ((cursor.x >= menu_button[j].x && cursor.x <= (menu_button[j].x + menu_button[j].w)) &&
          (cursor.y >= menu_button[j].y && cursor.y <= (menu_button[j].y + menu_button[j].h)))
      {
        key_menu = j; // update menu to point
        break;        // Don't need to check rest of menu
      }
    }


    /* --- return old selection to unselected state --- */

    if (old_key_menu && (key_menu != old_key_menu))
    {
      SDL_BlitSurface(CurrentBkgd(), &menu_button[old_key_menu], screen, &menu_button[old_key_menu]);
      DrawButton(&menu_button[old_key_menu], 10, REG_RGBA);
      SDL_BlitSurface(reg_text[old_key_menu][menu_depth], NULL, screen, &text_dst[old_key_menu]);
      SDL_BlitSurface(menu_gfx[old_key_menu][menu_depth]->default_img, NULL, screen, &menu_gfxdest[old_key_menu]);
    }


    /* --- draw current selection --- */

    if ((key_menu != 0) &&
       ((old_key_menu != key_menu) || (frame % 5 == 0))) // Redraw every fifth frame?
    {
      if (key_menu != old_key_menu)
      {
        REWIND(menu_gfx[key_menu][menu_depth]);
        PlaySound(snd_move);
      }

      SDL_BlitSurface(CurrentBkgd(), &menu_button[key_menu], screen, &menu_button[key_menu]);
      DrawButton(&menu_button[key_menu], 10, SEL_RGBA);
      SDL_BlitSurface(sel_text[key_menu][menu_depth], NULL, screen, &text_dst[key_menu]);
      SDL_BlitSurface(menu_gfx[key_menu][menu_depth]->frame[menu_gfx[key_menu][menu_depth]->cur], NULL, screen, &menu_gfxdest[key_menu]);

      NEXT_FRAME(menu_gfx[key_menu][menu_depth]);
    }


    // HACK This is still more than we need to update every frame but
    // it cuts cpu on my machine %60 so it seems better...
    if ( settings.menu_music )
      SDL_BlitSurface(speaker, NULL, screen, &spkrdest);
    else
      SDL_BlitSurface(speakeroff, NULL, screen, &spkrdest);

    SDL_UpdateRect(screen, spkrdest.x, spkrdest.y, spkrdest.w, spkrdest.h);

    for (i = 1; i < 6; i++)
    {
      SDL_UpdateRect(screen, menu_button[i].x, menu_button[i].y, menu_button[i].w, menu_button[i].h);
    }

    if (tux_frame)
      SDL_UpdateRect(screen, Tuxdest.x, Tuxdest.y, Tuxdest.w, Tuxdest.h);

    if (firstloop)
      SDL_UpdateRect(screen, Tuxdest.x, Tuxdest.y, Tuxdest.w, Tuxdest.h);

    firstloop = 0;

    /* Wait so we keep frame rate constant: */
    while ((SDL_GetTicks() - start) < 33)
    {
      SDL_Delay(20);
    }

    frame++;
  } /* ----------- End of 'while(!done)' loop ------------  */



  LOG( "->>Freeing title screen images\n" );

  unload_media();

  LOG( "->TitleScreen():END \n" );
}



/************************************************************************/
/*                                                                      */ 
/*       "Private" functions (local to titlescreen.c)                   */
/*                                                                      */
/************************************************************************/

static void show_logo(void)
{
  if (settings.show_tux4kids)
  {
    SDL_Rect logo_rect;
    SDL_Surface* t4k_logo = LoadImage("standby.png", IMG_REGULAR|IMG_NO_THEME);
    
    if (t4k_logo) /* Avoid segfault */
    { 
      logo_rect.x = screen->w/2 - t4k_logo->w/2;  // Center horizontally
      logo_rect.y = screen->h/2 - t4k_logo->h/2;  // Center vertically
      logo_rect.w = t4k_logo->w;
      logo_rect.h = t4k_logo->h;

      /* Black out screen: */
      SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
      SDL_BlitSurface(t4k_logo, NULL, screen, &logo_rect);
      SDL_UpdateRect(screen, 0, 0, 0, 0);
      SDL_FreeSurface(t4k_logo);  // Unload image
    }
    else
      fprintf(stderr, "Couldn't load 'standby.png'\n");
  }
}


/* Renders the menu text for all items and loads the associated */
/* animated sprites, if any.                                    */
static void load_menu(void)
{
  unsigned char fn[FNLEN];
  int max, i, j;

  SDL_ShowCursor(1);

  LOG("loading & parsing menu\n");

  for (j = 1; j <= TITLE_MENU_DEPTH; j++)  /* Each 'depth' is a different menu */
  {
    max = 0;
    for (i = 1; i <= TITLE_MENU_ITEMS; i++)
    {
      DEBUGCODE
      {
        fprintf(stderr, "i = '%d'\tj = '%d'\ttext = '%s'\n",
                i, j,  gettext((unsigned char*)menu_text[j+5*i]));
      }

      /* --- create text surfaces --- */
      reg_text[i][j] = BlackOutline( gettext((unsigned char*)menu_text[j+5*i]), font, &white);
      sel_text[i][j] = BlackOutline( gettext((unsigned char*)menu_text[j+5*i]), font, &yellow);

      /* (first make sure ptr valid to avoid segfault) */
      if (sel_text[i][j] && sel_text[i][j]->w > max)
        max = sel_text[i][j]->w;

      /* --- load animated icon for menu item --- */
      sprintf(fn, "menu/%s", menu_icon[i][j]);
      menu_gfx[i][j] = LoadSprite(fn, IMG_ALPHA);
    }
    menu_width[j] = max + 20 + 40; // Not clear where '20' and '40' are coming from
  }

  recalc_rects();
}



static void recalc_rects(void)
{
  int i = 0;

  Tuxdest.x = 0;
  Tuxdest.y = screen->h - Tux->frame[0]->h;
  Tuxdest.w = Tux->frame[0]->w;
  Tuxdest.h = Tux->frame[0]->h;

  Titledest.x = 0;
  Titledest.y = 10;
  Titledest.w = title->w;
  Titledest.h = title->h;

  spkrdest.x = screen->w - speaker->w - profiles->w - 10;
  spkrdest.y = screen->h - speaker->h - 10;
  spkrdest.w = speaker->w;
  spkrdest.h = speaker->h;
  
  profilesdest.x = screen->w - speaker->w - 10;
  profilesdest.y = screen->h - profiles->h - 10;
  profilesdest.w = profiles->w; 
  profilesdest.h = profiles->h;
  /* --- setup menu item destinations --- */
  menu_button[1].x = screen->w/2 - 120; //240;
  menu_button[1].y = 100;
  menu_button[1].w = menu_width[1];  //calc from width of widest menu item
  menu_button[1].h = 50;

  menu_gfxdest[1].x = menu_button[1].x + 6; // inset graphic by (6, 4) */
  menu_gfxdest[1].y = menu_button[1].y + 4;
  menu_gfxdest[1].w = 40;
  menu_gfxdest[1].h = 50;

  text_dst[1].x = screen->w/2 - 70;//290;
  text_dst[1].y = menu_button[1].y + 15;

  /* FIXME each menu item drawn hardcoded 60 pixels below last - */
  /* perhaps increment should be "menu_button[j-1].h + MENU_ITEM_GAP" */
  for (i = 2; i < 6; i++) 
  {
    /* --- setup location of button text --- */
    text_dst[i].x = screen->w/2 - 70;
    text_dst[i].y = text_dst[i - 1].y + 60;

    /* --- setup location of button background --- */
    menu_button[i].x = menu_button[i - 1].x;
    menu_button[i].y = menu_button[i - 1].y + 60;
    menu_button[i].w = menu_button[i - 1].w;
    menu_button[i].h = menu_button[i - 1].h;

    /* --- setup location of animated icon --- */
    menu_gfxdest[i].x = menu_gfxdest[i - 1].x;
    menu_gfxdest[i].y = menu_gfxdest[i - 1].y + 60;
    menu_gfxdest[i].w = menu_gfxdest[i - 1].w;
    menu_gfxdest[i].h = menu_gfxdest[i - 1].h;
  }
}



static void unload_menu(void)
{
  int i,j;
  printf("enter unload_menu()\n");

  for (i = 1; i <= TITLE_MENU_ITEMS; i++)
  {
    for (j = 1; j <= TITLE_MENU_DEPTH; j++)
    {
      if (reg_text[i][j])
        SDL_FreeSurface(reg_text[i][j]);
      if (sel_text[i][j])
        SDL_FreeSurface(sel_text[i][j]);
      if (menu_gfx[i][j])
        FreeSprite(menu_gfx[i][j]);
      reg_text[i][j] = sel_text[i][j] = NULL;
      menu_gfx[i][j] = NULL;
    }
  }
}


static int load_media(void)
{
  DEBUGCODE
  {
    fprintf(stderr, "Entering load_media():\n");
    fprintf(stderr, "default_data_path = %s\n", settings.default_data_path);
    fprintf(stderr, "theme_data_path = %s\n", settings.theme_data_path);
  }

  /* Make sure everything is unloaded before we start: */
  /* FIXME have not been to do this to run without crashing: */
  //unload_media();

  /* --- load sounds --- */
  if (settings.menu_sound)
  {
    snd_move = LoadSound("tock.wav");
    snd_select = LoadSound("pop.wav");
    if (!snd_move || !snd_select)
    {
      fprintf(stderr, "Could not load menu sounds - turning menu_sound off\n");
      settings.menu_sound = 0;
    }
  }
 
  /* --- load graphics --- */
  LoadBothBkgds("main_bkg.png");
  title = LoadImage( "title1.png", IMG_ALPHA );
  speaker = LoadImage( "sound.png", IMG_ALPHA );
  speakeroff = LoadImage( "nosound.png", IMG_ALPHA );
  profiles = LoadImage("profiles.png", IMG_ALPHA)
//  sel = LoadSprite("menu/sel", IMG_ALPHA);
//  reg = LoadSprite("menu/reg", IMG_ALPHA);
  Tux = LoadSprite("tux", IMG_ALPHA);

  DEBUGCODE
  {
    fprintf(stderr, "titlescreen.c load_media(): settings.theme_font_name is %s\n",
            settings.theme_font_name);
  }
  font = LoadFont(settings.theme_font_name, MENU_FONT_SIZE);

  /* Make sure we were successful: */
  if (!CurrentBkgd()
   || !title
   || !speaker
   || !speakeroff
   || !profiles
   || !Tux
   || !font)
  {
    fprintf(stderr, "load_media() - could not load all needed files\n");
    unload_media();
    return 0;
  }

  /* Should probably call this directly from TitleScreen() */
  load_menu();
  return 1;
}



static void unload_media(void)
{
  LOG("Entering unload_media():\n");

  /* --- unload sounds --- */
  if (settings.menu_sound){
 
    if (snd_move)
    { 
      Mix_FreeChunk(snd_move);
      snd_move = NULL;
    }
    if (snd_select)
    { 
      Mix_FreeChunk(snd_select);
      snd_select = NULL;
    }
  }

  /* --- unload graphics --- */
  if (title)
  {
    SDL_FreeSurface(title);
    title = NULL;
  }
  if (speaker)
  {
    SDL_FreeSurface(speaker);
    speaker = NULL;
  }
  if (speakeroff)
  {
    SDL_FreeSurface(speakeroff);
    speakeroff = NULL;
  }
  if(profiles)
  {
	  SDL_FreeSurface(profiles);
	  profiles = NULL;
  }

  FreeBothBkgds();

  if (Tux)
  {
    FreeSprite(Tux);
    Tux = NULL;
  }

  if (font)
  {
    TTF_CloseFont(font);
    font = NULL;
  }

  LOG("Leaving load_media():\n");

  unload_menu();
}


static void not_implemented(void)
{
  SDL_Surface *s1 = NULL, *s2 = NULL, *s3 = NULL, *s4 = NULL;
  sprite* tux = NULL;
  SDL_Rect loc;
  int finished = 0, i;

  LOG( "NotImplemented() - creating text\n" );

  s1 = BlackOutline( gettext_noop("Work In Progress!"), font, &white);
  s2 = BlackOutline( gettext_noop("This feature is not ready yet"), font, &white);
  s3 = BlackOutline( gettext_noop("Discuss the future of TuxTyping at"), font, &white);

  /* we always want the URL in english */
  /* NOTE: all fonts are almost certain to include glyphs for ASCII, */
  /* so the following "english_font" hackery is probably unnecessary: */
  if (!settings.use_english)
  {
    TTF_Font *english_font;
    settings.use_english = 1;
    english_font = LoadFont(DEFAULT_MENU_FONT, MENU_FONT_SIZE);
    s4 = BlackOutline( "http://tuxtype.sf.net/forums", english_font, &white);
    TTF_CloseFont(english_font);
    settings.use_english = 0;
  }
  else 
    s4 = BlackOutline( "http://tuxtype.sf.net/forums", font, &white);

  tux = LoadSprite("tux/tux-egypt", IMG_ALPHA);

  if (s1 && s2 && s3 && s4 && tux)
  {
    LOG( "NotImplemented() - drawing screen\n" );

    SDL_BlitSurface(CurrentBkgd(), NULL, screen, NULL);
    loc.x = screen->w/2 - (s1->w/2); loc.y = 10;
    SDL_BlitSurface( s1, NULL, screen, &loc);
    loc.x = screen->w/2 - (s2->w/2); loc.y = 60;
    SDL_BlitSurface( s2, NULL, screen, &loc);
    loc.x = screen->w/2 - (s3->w/2); loc.y = screen->h/2 + 160;
    SDL_BlitSurface(s3, NULL, screen, &loc);
    loc.x = screen->w/2 - (s4->w/2); loc.y = screen->h/2 + 200;
    SDL_BlitSurface( s4, NULL, screen, &loc);

    loc.x = screen->w/2 - (tux->frame[0]->w/2);
    loc.y = screen->h - 280;
    loc.w = tux->frame[0]->w;
    loc.h = tux->frame[0]->h;
    SDL_BlitSurface( tux->frame[tux->cur], NULL, screen, &loc);

    SDL_UpdateRect(screen, 0, 0, 0, 0);

    i = 0;

    while (!finished)
    {
      while (SDL_PollEvent(&event)) 
      {
        switch (event.type)
        {
          case SDL_QUIT:
            exit(0);
          case SDL_MOUSEBUTTONDOWN:
          case SDL_KEYDOWN:
            finished = 1;
        }
      }

      i++;

      if (i %5 == 0)
      {
        NEXT_FRAME(tux);
        SDL_BlitSurface(CurrentBkgd(), &loc, screen, &loc);
        SDL_BlitSurface(tux->frame[tux->cur], NULL, screen, &loc);
        SDL_UpdateRect(screen, loc.x, loc.y, loc.w, loc.h);
      }

      SDL_Delay(40);
    }
  }
  else
    fprintf(stderr, "NotImplemented() - could not load needed graphic\n");

  SDL_FreeSurface(s1);
  SDL_FreeSurface(s2);
  SDL_FreeSurface(s3);
  SDL_FreeSurface(s4);
  s1 = s2 = s3 = s4 = NULL;
  FreeSprite(tux);
  tux = NULL;
}




#define MAX_WORD_LISTS 100

/* returns 0 if user pressed escape ...
 *         1 if word list was set correctly
 */
static int chooseWordlist(void)
{
  SDL_Surface* titles[MAX_WORD_LISTS] = {NULL};
  SDL_Surface* select[MAX_WORD_LISTS] = {NULL};
  SDL_Surface* left = NULL, *right = NULL;
  SDL_Rect leftRect, rightRect;
  SDL_Rect titleRects[8];
  int stop = 0;
  int loc = 0;
  int old_loc = 1;
  int lists = 0;
  int i;
  unsigned char wordPath[FNLEN];
  unsigned char wordlistFile[MAX_WORD_LISTS][200];
  unsigned char wordlistName[MAX_WORD_LISTS][200];

  DIR* wordsDir = NULL;
  struct dirent* wordsFile = NULL;
  FILE* tempFile = NULL;

  LOG("Entering chooseWordlist():\n");

  /* find the directory to load wordlists from */

  /* Check under theme directory first, if theme selected: */
  if (!settings.use_english)  /* Using theme: */
  {
    sprintf(wordPath,"%s/words", settings.theme_data_path);
    if (!CheckFile(wordPath))
    {
      fprintf(stderr, "chooseWordList() - theme contains no wordlist dir \n");
      return 0;
    }
  }
  else  /* No theme selected - using English: */
  {
    sprintf(wordPath,"%s/words", settings.default_data_path);
    if (!CheckFile(wordPath))
    {
      fprintf(stderr, "chooseWordList() - data path contains no wordlist dir \n");
      return 0;
    }
  }

  /* If we get to here, we know there is at least a wordlist directory */
  /* but not necessarily any valid files.                              */

  DEBUGCODE { fprintf(stderr, "wordPath is: %s\n", wordPath); }


  /* FIXME looks like a place for scandir() - or our own w32_scandir() */
  /* create a list of all the .txt files */

  wordsDir = opendir( wordPath );	

  do
  {
    wordsFile = readdir(wordsDir);
    if (!wordsFile)
      break; /* Loop continues until break occurs */

    /* must have at least .txt at the end */
    if (strlen(wordsFile->d_name) < 5)
      continue;

    if (strcmp(&wordsFile->d_name[strlen(wordsFile->d_name)-4],".txt"))
      continue;

    sprintf(wordlistFile[lists], "%s/%s", wordPath, wordsFile->d_name);

    /* load the name for the wordlist from the file ... (1st line) */
    tempFile = fopen( wordlistFile[lists], "r" );
    if (!tempFile)
      continue;

    fscanf(tempFile, "%[^\n]\n", wordlistName[lists]);

    /* check to see if it has a \r at the end of it (dos format!) */
    if (wordlistName[lists][strlen(wordlistName[lists]) - 1] == '\r')
      wordlistName[lists][strlen(wordlistName[lists]) - 1] = '\0';

    lists++;

    fclose(tempFile);
  } while (1); /* Loop continues until break occurs */

  closedir(wordsDir);	
 
 DEBUGCODE { fprintf(stderr, "Found %d .txt file(s) in words dir\n", lists); }



  /* let the user pick the list */

  /* Render SDL_Surfaces for list entries: */
//  titles[0] = BlackOutline( _("Alphabet"), font, &white );
//  select[0] = BlackOutline( _("Alphabet"), font, &yellow);

  /* NOTE - no longer hard-coding titles[0] to be alphabet - themes  */
  /* should include a regular word list file called "alphabet.txt"   */
  /* Should sort the list and always put the alphabet file first, if */
  /* present.                                                        */
  for (i = 0; i < lists; i++)
  {
    titles[i] = BlackOutline( wordlistName[i], font, &white );
    select[i] = BlackOutline( wordlistName[i], font, &yellow);
  }

  left = LoadImage("left.png", IMG_ALPHA);
  right = LoadImage("right.png", IMG_ALPHA);

  /* Get out if needed surface not loaded successfully: */
  if (!CurrentBkgd() || !left || !right)
  {
    fprintf(stderr, "chooseWordList(): needed image not available\n");
  
    for (i = 0; i < lists; i++)
    {
      SDL_FreeSurface(titles[i]);
      SDL_FreeSurface(select[i]);
      titles[i] = select[i] = NULL;
    }

    SDL_FreeSurface(left);
    SDL_FreeSurface(right);
    left = right = NULL;

    return 0;
  }
  


  leftRect.w = left->w;
  leftRect.h = left->h;
  leftRect.x = screen->w/2 - 80 - (leftRect.w/2);
  leftRect.y = screen->h/2 - 50;

  rightRect.w = right->w; rightRect.h = right->h;
  rightRect.x = screen->w/2 + 80 - (rightRect.w/2); rightRect.y = screen->h/2 - 50;

  /* set initial rect sizes */
  titleRects[0].y = 30;
  titleRects[0].w = titleRects[0].h = titleRects[0].x = 0;

  for (i = 1; i < 8; i++)
  { 
    titleRects[i].y = titleRects[i - 1].y + 50;
    titleRects[i].w = titleRects[i].h = titleRects[i].x = 0;
  }

  /* Main event loop for this screen: */
  while (!stop)
  {
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_QUIT:
          exit(0); /* FIXME may need to cleanup memory and exit more cleanly */
          break;

        case SDL_MOUSEMOTION:
          for (i=0; (i<8) && (loc-(loc%8)+i<lists); i++)
            if (inRect( titleRects[i], event.motion.x, event.motion.y ))
            {
              loc = loc-(loc%8)+i;
              break;
            }
          break;

        case SDL_MOUSEBUTTONDOWN:
                                        if (inRect( leftRect, event.button.x, event.button.y ))
                                                if (loc-(loc%8)-8 >= 0) {
                                                        loc=loc-(loc%8)-8;
                                                        break;
                                                }
                                        if (inRect( rightRect, event.button.x, event.button.y ))
                                                if (loc-(loc%8)+8 < lists) {
                                                        loc=loc-(loc%8)+8;
                                                        break;
                                                }
                                        for (i=0; (i<8) && (loc-(loc%8)+i<lists); i++)
                                                if (inRect(titleRects[i], event.button.x, event.button.y)) {
                                                        loc = loc-(loc%8)+i;
							ClearWordList(); /* clear old selection */
							//if (loc==0)
							//  UseAlphabet(); 
							//else
							GenerateWordList(wordlistFile[loc]); 
                                                        stop = 1;
                                                        break;
                                                }
                                        break;
                                case SDL_KEYDOWN:
                                        if (event.key.keysym.sym == SDLK_ESCAPE) { stop = 2; break; }
                                        if (event.key.keysym.sym == SDLK_RETURN) {
						ClearWordList(); /* clear old selection */
						//if (loc==0)
						//  UseAlphabet(); 
						//else
						GenerateWordList(wordlistFile[loc]); 
                                                stop = 1;
                                                break;
                                        }

                                        if ((event.key.keysym.sym == SDLK_LEFT) || (event.key.keysym.sym == SDLK_PAGEUP)) {
                                                if (loc-(loc%8)-8 >= 0)
                                                        loc=loc-(loc%8)-8;
                                        }

                                        if ((event.key.keysym.sym == SDLK_RIGHT) || (event.key.keysym.sym == SDLK_PAGEDOWN)) {
                                                if (loc-(loc%8)+8 < lists)
                                                        loc=(loc-(loc%8)+8);
                                        }

                                        if (event.key.keysym.sym == SDLK_UP) {
                                                if (loc > 0)
                                                        loc--;
                                        }

                                        if (event.key.keysym.sym == SDLK_DOWN) {
                                                if (loc+1<lists)
                                                        loc++;
                                        }
      }
    }

    if (old_loc != loc) {
                        int start;

                        SDL_BlitSurface(CurrentBkgd(), NULL, screen, NULL );

                        start = loc - (loc % 8);
                        for (i = start; i<MIN(start+8,lists); i++) {
                                titleRects[i%8].x = screen->w/2 - (titles[i]->w/2);
                                if (i == loc)
                                        SDL_BlitSurface(select[loc], NULL, screen, &titleRects[i%8]);
                                else
                                        SDL_BlitSurface(titles[i], NULL, screen, &titleRects[i%8]);
                        }

                        /* --- draw buttons --- */

                        if (start>0)
                                SDL_BlitSurface( left, NULL, screen, &leftRect );

                        if (start+8<lists)
                                SDL_BlitSurface( right, NULL, screen, &rightRect );

                        SDL_UpdateRect(screen, 0, 0, 0 ,0);
                }
                SDL_Delay(40);
                old_loc = loc;
  }

  /* --- clear graphics before leaving function --- */ 
  for (i = 0; i<lists; i++)
  {
    SDL_FreeSurface(titles[i]);
    SDL_FreeSurface(select[i]);
    titles[i] = select[i] = NULL;
  }

  SDL_FreeSurface(left);
  SDL_FreeSurface(right);
  left = right = NULL; /* Maybe overkill - about to be destroyed anyway */

  DEBUGCODE { fprintf( stderr, "Leaving chooseWordlist();\n" ); }

  if (stop == 2)
    return 0;

  return 1;
}

