/*=============================================================================
 |       Author:  KEREM EMRE GÜNER (Terracotta Lite)
 |     Language:  C compiled with gcc 
 |   To Compile:  run "make" in the directory
 |
 +-----------------------------------------------------------------------------
 |
 |  Description:  A simple raycaster game that randomly generates mazes
 |      to use
 |
 |    Algorithm:  Raycaster by Lode Vandevenne and Recursize Backtracking
 |      maze generator by Aryan Abed-Esfahani.
 |
 |   Required Features Not Included:  The program doesn't handle drawing
 |      to the screen and uses SDL2 for that purpouse .
 |
 |   Known Bugs:  The maze is corrupted when the maze width and maze  
 |      height aren't the same, though I will keep it because it looks
 |      cool.
 |
 *===========================================================================*/

#include "config.h"

#ifdef _WIN32
  	#include "SDL.h"
#else
	#include <SDL2/SDL.h>
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_RATIO_WIDTH 5
#define SCREEN_RATIO_HEIGHT 3

/* Both must be odd numbers.
   For some reason it doesn't
   work when the numbers are not the same
   TODO: fix it */
#define MAP_WIDTH 101
#define MAP_HEIGHT 101

#define PI 3.1415926535897932384626433

#define MUS_PATH "sound.wav"

#define TRACK_NONE   0
#define TRACK_JITTER 1
#define TRACK_SWOOP  2
#define TRACK_CREEP  3

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

/*
  jitter: 630876
  swoop:  317532
  creep:  460132
*/
const int wavLength[] = {0, 630876, 948408, 1408540};	/* lengths of the audio sections contained inside the main audio file */

static Uint8 *audio_pos;	/* global pointer to the audio buffer to be played */
static Uint32 audio_len;	/* remaining length of the sample we have to play */
static Uint8  audio_selected = TRACK_NONE;

/* Log an SDL error with some error message to the output stream of our choice
   @param msg The error message to write, format will be msg error: SDL_GetError() */
void logSDLError( char *msg, const char *error ) {
	printf( "%s error: %s\n", msg, error );
}

/* Arrange the N elements of ARRAY in random order.
   Ony effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator.
   @param array The array to shuffle.
   @param n The number of elements in the array */
static void shuffle(int *array, size_t n)
{
  if (n > 1) 
  {
    size_t i;
    for (i = 0; i < n - 1; i++) 
    {
      size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

/* Self-call itself to randomly generate a maze. 
   Picks a direction and calls itself in that direction.
   @param x The X coordinate of the starting point for building the maze
   @param y The Y coordinate of the starting point for building the maze
   @param mazegrid The array containing the maze */
static void MAZE_Recurse(int x, int y, uint8_t *mazegrid)
{
	int x_1, x_2, y_1, y_2;
	mazegrid[y * MAP_WIDTH + x] = 1;
  
#define MAZE_isUpUsed    (mazegrid[(y + 2) * MAP_WIDTH + x] == 1)
#define MAZE_isDownUsed  (mazegrid[(y - 2) * MAP_WIDTH + x] == 1)
#define MAZE_isLeftUsed  (mazegrid[(y * MAP_WIDTH) + x - 2] == 1)
#define MAZE_isRightUsed (mazegrid[(y * MAP_WIDTH) + x + 2] == 1)
#define MAZE_isEverywhereUsed (MAZE_isUpUsed && MAZE_isDownUsed && MAZE_isLeftUsed && MAZE_isRightUsed)

	if (MAZE_isEverywhereUsed == 0)
	{
		int MAZE_directions[] = {1, 2, 3, 4};
		shuffle(MAZE_directions, 4);
		for (int i = 0; i < 4; i++)
		{
			switch (MAZE_directions[i])
			{
				case 1:
				y_1 = y - 2;
				y_2 = y - 1;
				x_1 = x;
				x_2 = x;
				break;
				  
				case 2:
				y_1 = y + 2;
				y_2 = y + 1;
				x_1 = x;
				x_2 = x;
				break;
				
				case 3:
				x_1 = x - 2;
				x_2 = x - 1;
				y_1 = y;
				y_2 = y;
				break;

				case 4:
				x_1 = x + 2;
				x_2 = x + 1;
				y_1 = y;
				y_2 = y;
				break;
			}
			if (mazegrid[y_1 * MAP_WIDTH + x_1] != 1)
			{
				mazegrid[y_2 * MAP_WIDTH + x_2] = 1;
				MAZE_Recurse(x_1, y_1, mazegrid);
			}
		}
	}
}

/* Calls the MAZE_RECURSE function appropraitely to create a maze.
   @param mazegrid The array to put the maze in */
static void createMap(uint8_t *mazegrid)
{
	for (int i = 0; i < MAP_WIDTH; i++)
	{
		for (int j = 0; j < MAP_HEIGHT; j++)
		{
			if (i % 2 == 1 || j % 2 == 1)
			{
				mazegrid[j * MAP_WIDTH + i] = 2;
			}
			if (i == 0 || j == 0 || i == MAP_WIDTH - 1 || j == MAP_HEIGHT - 1)
			{
				mazegrid[j * MAP_WIDTH + i] = 1;
			}
		}
	}

	MAZE_Recurse((MAP_WIDTH - (MAP_WIDTH % 4))/2, (MAP_HEIGHT - (MAP_HEIGHT % 4))/2, mazegrid);

	for (int i = 0; i < MAP_WIDTH; i++)
	{
		for (int j = 0; j < MAP_HEIGHT; j++)
		{
			int mazepos = j * MAP_HEIGHT + i;
			if (mazegrid[mazepos] == 2)
		{
				mazegrid[mazepos] = (rand() % 7) + 1; 
				printf("\xe2\x96\x88");
			}
			else
			{
				mazegrid[mazepos] = 0;
				printf(" ");
			}
		}
		printf("\n");
	}
}

uint8_t map[MAP_WIDTH*MAP_HEIGHT]; /* our map (or maze) */

int main( int argc, char *argv[] ) {

	/* ----- INITIALIZE EVERYTHING ----- */
	
	srand(time(NULL));
	uint8_t EXIT_CODE = 0;		/* the exit code will be changed if a problem occurs */
	int	RAW_SCREEN_WIDTH,	/* for the calculation of SCREEN_WIDTH and SCREEN_HEIGHT */
		RAW_SCREEN_HEIGHT,	/* for the calculation of SCREEN_WIDTH and SCREEN_HEIGHT */
		SCREEN_WIDTH,
		SCREEN_HEIGHT;
			
	/* Initialize SDL */
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) != 0 )
	{
		logSDLError( "SDL_Init" , SDL_GetError() );
		EXIT_CODE = 1;
		goto quit_1;
	}
	
	/* Create Window */
	SDL_Window *window = SDL_CreateWindow( "Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 240, SDL_WINDOW_RESIZABLE );
	if ( window == NULL )
	{
		logSDLError( "SDL_CreateWindow", SDL_GetError() );
		EXIT_CODE = 2;
		goto quit_2;
	}

	/* Create Renderer */
	SDL_Renderer *renderer = SDL_CreateRenderer( window,  -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	if ( renderer == NULL )
	{
		logSDLError( "SDL_CreateRenderer", SDL_GetError() );
		EXIT_CODE = 3;
		goto quit_3;
	}

	/* local variables */
	static Uint32 wav_length;	/*  length of our sample */
	static Uint8 *wav_buffer;	/*  buffer containing our audio file */
	static SDL_AudioSpec wav_spec;	/* the specs of our piece of music */
	
	/* load the WAV (audio) */
	/* the specs, length and buffer of our wav are filled */
	if (SDL_LoadWAV(MUS_PATH, &wav_spec, &wav_buffer, &wav_length) == NULL)
	{
		logSDLError("SDL_LoadWAV", SDL_GetError());
		EXIT_CODE = 4;
		goto quit_4;
	}

	/* set our global static variables */
	audio_pos = wav_buffer + 603876;	/* copy sound buffer and scroll to the wanted track */
	audio_len = wav_length - 1091008;	/* copy file length and scroll to the wanted track */
	
	/* Open the audio device */
	SDL_AudioDeviceID deviceID = SDL_OpenAudioDevice(NULL, 0, &wav_spec, NULL, 0);
	if (deviceID == 0)
	{
		logSDLError("SDL_OpenAudioDevice", SDL_GetError());
		EXIT_CODE = 5;
		goto quit_5;
	}

	/* queue the first track */
	SDL_QueueAudio(deviceID, audio_pos, audio_len);
	
	/* start playing the audio */
	SDL_PauseAudioDevice(deviceID, 0);

	/* create the map (maze) */
	createMap(map);

#define INIT_PLANEY (tan(FOV / 2))
#define INIT_PLAYERX ((MAP_WIDTH - (MAP_WIDTH % 4))/2 + 0.5)
#define INIT_PLAYERY ((MAP_HEIGHT - (MAP_HEIGHT % 4))/2 + 0.5)

	/* Variables */
	uint8_t quit = 0;		/* either 1 or 0 indicating if we should quit or shouldn't */
	int	FOV = 850,
		m,
		FPS = 0;
	double	planeX = 0,
		planeY = INIT_PLANEY,
		playerX = INIT_PLAYERX,	/* initial player X coordinate */
		playerY = INIT_PLAYERY,	/* initial player Y coordinate */
		dirX = -1,		/* initial player direction (cosine of the angle) */	
		dirY = 0,		/* initial player direction (sine of the angle) */
		movespeed = 0.0875,	/* initial player move speed will change according to FPS */
		rotspeed = 0.07,	/* initial player rotation speed will change according to FPS */
		monsterX = 2.5,		/* initial monster X coordinate */
		monsterY = 2.5,		/* initial monster Y coordinate */
		newX,			/* helper */
		newY,			/* helper */
		rotcos,			/* helper */
		rotsin,			/* helper */
		oldDirX,		/* helper */
		oldPlaneX;		/* helper */

#undef INIT_PLANEY
#undef INIT_PLAYERX
#undef INIT_PLAYERY

	if (argc > 1)
	{
	FPS = atoi(argv[1]);
	}

	long long unsigned int	time = 0,
				lastTime = 0,
				oldTime = 0,
				section1,
				frametime;

	/* capture the mouse */
	SDL_CaptureMouse(SDL_TRUE);
	SDL_ShowCursor(SDL_DISABLE);

	/* ----- MAIN LOOP ----- */
	
	SDL_Event e;
	while ( quit == 0 ) {
	
		section1 = SDL_GetTicks64();

		while ( SDL_PollEvent(&e) ) {
			switch ( e.type ) {
				case SDL_QUIT:
					quit = 1;
					break;
			
				case SDL_KEYDOWN:
				switch ( e.key.keysym.sym ) {

					/* -- Mission Critical -- */

					/* Q (for quit) */
					case SDLK_q:
					quit = 1;
					break;
					
					/* ESCAPE (to get out of mouse capture) */
					case SDLK_ESCAPE:
					SDL_ShowCursor(SDL_ENABLE);
					SDL_CaptureMouse(SDL_FALSE);
					break;
					
					/* -- Player Movement -- */

					case SDLK_UP:
					case SDLK_w:
					newX = dirX * movespeed;
					newY = dirY * movespeed;
					
#define MAPPOSX ((int)(playerY) * MAP_WIDTH + (int)(playerX + newX))
#define MAPPOSY ((int)(playerY + newY) * MAP_WIDTH + (int)(playerX))

					playerX += (map[MAPPOSX] == 0) * newX;
					playerY += (map[MAPPOSY] == 0) * newY;

#undef MAPPOSX
#undef MAPPOSY
					break;

					case SDLK_DOWN:
					case SDLK_s:
					newX = dirX * movespeed;
					newY = dirY * movespeed;
					
#define MAPPOSX ((int)(playerY) * MAP_WIDTH + (int)(playerX - newX))
#define MAPPOSY ((int)(playerY - newY) * MAP_WIDTH + (int)(playerX))

					playerX -= (map[MAPPOSX] == 0) * newX;
					playerY -= (map[MAPPOSY] == 0) * newY;

#undef MAPPOSX
#undef MAPPOSY
					break;

					case SDLK_RIGHT:
					rotcos = cos(-rotspeed);
					rotsin = sin(-rotspeed);
					oldDirX = dirX;
					dirX = dirX * rotcos - dirY * rotsin;
					dirY = oldDirX * rotsin + dirY * rotcos;
					oldPlaneX = planeX;
					planeX = planeX * rotcos - planeY * rotsin;
					planeY = oldPlaneX * rotsin + planeY * rotcos;
					break;
					
					case SDLK_LEFT:
					rotcos = cos(rotspeed);
					rotsin = sin(rotspeed);
					oldDirX = dirX;
					dirX = dirX * rotcos - dirY * rotsin;
					dirY = oldDirX * rotsin + dirY * rotcos;
					oldPlaneX = planeX;
					planeX = planeX * rotcos - planeY * rotsin;
					planeY = oldPlaneX * rotsin + planeY * rotcos;
					break;

					case SDLK_1:
					if (SDL_GetQueuedAudioSize(deviceID) == 0)
					{
						audio_pos = wav_buffer + 603876;	/* scroll to the track */
						audio_len = wav_length - 1091008;	/* scroll to the track */
						/* play the audio */
						SDL_QueueAudio(deviceID, audio_pos, audio_len);
					}
					break;


				}
				break;

				case SDL_MOUSEBUTTONDOWN:
				SDL_ShowCursor(SDL_DISABLE);
				SDL_CaptureMouse(SDL_TRUE);
				break;

				case SDL_MOUSEMOTION:
				SDL_GetMouseState(NULL, &m);
				SDL_WarpMouseInWindow(window, SCREEN_WIDTH/2, m);
				rotcos = cos(rotspeed * e.motion.xrel / -7.0);
				rotsin = sin(rotspeed * e.motion.xrel / -7.0);
				oldDirX = dirX;
				dirX = dirX * rotcos - dirY * rotsin;
				dirY = oldDirX * rotsin + dirY * rotcos;
				oldPlaneX = planeX;
				planeX = planeX * rotcos - planeY * rotsin;
				planeY = oldPlaneX * rotsin + planeY * rotcos;
				break;
				
					
			}
		}

#if DEBUG
		printf("INPUT: %lld\n", SDL_GetTicks64() - section1);
		section1 = SDL_GetTicks64();
#endif

		/* ---- End of Input ---- */

		SDL_GetWindowSize(window, &RAW_SCREEN_WIDTH, &RAW_SCREEN_HEIGHT);
		if (RAW_SCREEN_WIDTH/SCREEN_RATIO_WIDTH <= RAW_SCREEN_HEIGHT/SCREEN_RATIO_HEIGHT)
		{
			SCREEN_WIDTH = RAW_SCREEN_WIDTH;
			SCREEN_HEIGHT = SCREEN_WIDTH/SCREEN_RATIO_WIDTH*SCREEN_RATIO_HEIGHT;
		}
		else
		{
			SCREEN_HEIGHT = RAW_SCREEN_HEIGHT;
			SCREEN_WIDTH = SCREEN_HEIGHT/SCREEN_RATIO_HEIGHT*SCREEN_RATIO_WIDTH;
		}

		/* clear the renderer with black */
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear( renderer );


		/* --- Raycast --- */
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			double	cameraX = 2 * x / (double)SCREEN_WIDTH - 1,
				helper_cos = dirX + planeX * cameraX,
				helper_sin = dirY + planeY * cameraX,
				rayDistX,
				rayDistY,
				deltaDistX = (helper_cos == 0) ? 1e30 : fabs(1 / helper_cos),
				deltaDistY = (helper_sin == 0) ? 1e30 : fabs(1 / helper_sin),
				ultimateDist;
			int	mapX = (int)(playerX),	/* x coordinate of the cell we are at */
				mapY = (int)(playerY),	/* y coordinate of the cell we are at */
				mapPos,			/* the location of the cell in the array (helper) */
				lineHeight,		/* the height of the line to draw (helper) */
				hit = 0,
				side;
			int8_t	stepX,			/* the direection the ray moves in the map, either 1 or -1 */
				stepY;			/* the direection the ray moves in the map, either 1 or -1 */

			if (helper_cos < 0)
			{
				stepX = -1;
				rayDistX = (playerX - mapX) * deltaDistX;
			}
			else
			{
				stepX = 1;
				rayDistX = (mapX + 1.0 - playerX) * deltaDistX;
			}
			
			if (helper_sin < 0)
			{
				stepY = -1;
				rayDistY = (playerY - mapY) * deltaDistY;
			}
			else
			{
				stepY = 1;
				rayDistY = (mapY + 1.0 - playerY) * deltaDistY;
			}

			while (hit == 0)
			{
				if (rayDistX < rayDistY)
				{
					rayDistX += deltaDistX;
					mapX += stepX;
					side = 0;
				}
				else
				{
					rayDistY += deltaDistY;
					mapY += stepY;
					side = 1;
				}

				mapPos = mapY * MAP_WIDTH + mapX;

				if (map[mapPos]) hit = 1;
			}

			if (side == 0)
			{
			ultimateDist = (rayDistX - deltaDistX);
			}
			else
			{
			ultimateDist = (rayDistY - deltaDistY);
			}

			lineHeight = (int)(SCREEN_WIDTH / ultimateDist);
			
#define DRAW_Y1 (-lineHeight / 2 + SCREEN_HEIGHT / 2 < 0) ? 0 : -lineHeight / 2 + SCREEN_HEIGHT / 2
#define DRAW_Y2 (lineHeight / 2 + SCREEN_HEIGHT / 2 < SCREEN_HEIGHT) ? lineHeight / 2 + SCREEN_HEIGHT / 2 : SCREEN_HEIGHT - 1

			SDL_SetRenderDrawColor(renderer, (map[mapPos] & 4)*63, (map[mapPos] & 2)*63, (map[mapPos] & 1)*63, 0);
			SDL_RenderDrawLine(renderer, x, DRAW_Y1, x, DRAW_Y2); 

#undef DRAW_Y1
#undef DRAW_Y2

#define DRAW_Y1 (-lineHeight / 20 + SCREEN_HEIGHT / 2 < 0) ? 0 : -lineHeight / 20 + SCREEN_HEIGHT / 2
#define DRAW_Y2 (lineHeight / 20 + SCREEN_HEIGHT / 2 < SCREEN_HEIGHT) ? lineHeight / 20 + SCREEN_HEIGHT / 2 : SCREEN_HEIGHT - 1

			/* TODO: make this only run when there is an enemy in view and make it look like a filled circle */			
			if (0)
			{
				SDL_SetRenderDrawColor(renderer, 60, 60, 60, 0);
				SDL_RenderDrawLine(renderer, x, DRAW_Y1, x, DRAW_Y2); 
			}

#undef DRAW_Y1
#undef DRAW_Y2

		}

#if DEBUG
		printf("RAYCAST: %lld\n", SDL_GetTicks64() - section1);
		section1 = SDL_GetTicks64();
#endif
		
		/* --- Monster --- */
		
#define monsdirX (playerX-monsterX)
#define monsdirY (playerY-monsterY)
/*#define TRACK_NUMBER ((int)(3*x/MAP_WIDTH + 1))*/
#define TRACK_NUMBER 2

		double x = sqrt(monsdirX*monsdirX + monsdirY*monsdirY);

		if (x<1) {
#if DEBUG
			quit = 0;
#else
			quit = 1;
#endif
		}
		else
		{
			monsterX += monsdirX*movespeed/x;
			monsterY += monsdirY*movespeed/x;

			if (time - lastTime > x*100)
			{
				lastTime = time;
				audio_pos = wav_buffer + wavLength[TRACK_NUMBER];	/* scroll to the track */
				audio_len = wav_length - wavLength[4-TRACK_NUMBER];	/* scroll to the track */
				/* play the audio */
				SDL_ClearQueuedAudio(deviceID);
				SDL_QueueAudio(deviceID, audio_pos, audio_len);
			}
		}

#undef TRACK_NUMBER
#undef monsdirX
#undef monsdirY

		double invDet = 1.0 / (planeX * dirY - dirX * planeY);
		
#define monsdiffX (monsterX - playerX)
#define monsdiffY (monsterY - playerY)

		double transformX = invDet * (dirY * monsdiffX - dirX * monsdiffY);
		double transformY = invDet * (-planeY * monsdiffX + planeX * monsdiffY);

#undef monsdiffX
#undef monsdiffY

		int spriteScreenX = (int)((SCREEN_WIDTH / 2) * (1 + transformX / transformY));

		int spriteHeight = abs((int)(SCREEN_HEIGHT / transformY));

		int drawStartY = SCREEN_HEIGHT / 2 - spriteHeight / 2;
		if (drawStartY < 0) drawStartY = 0;
		int drawEndY   = SCREEN_HEIGHT / 2 + spriteHeight / 2;
		if (drawEndY  >= SCREEN_HEIGHT) drawEndY = SCREEN_HEIGHT - 1;
		
		int spriteWidth = abs((int)(SCREEN_HEIGHT / (transformY)));

		int drawStartX = spriteScreenX - spriteWidth / 2;
		if (drawStartX < 0) drawStartX = 0;
		int drawEndX   = spriteScreenX + spriteWidth / 2;
		if (drawEndX  >= SCREEN_HEIGHT) drawEndX = SCREEN_WIDTH - 1;

		if (transformY > 0)
		for (int x = 0; x < spriteWidth; x++)
		{
			if (drawStartX + x > 0 && drawStartX + x < SCREEN_WIDTH)
			{
#if GRAPHICS==PATATO
				SDL_SetRenderDrawColor(renderer,
				(int)((double)(x)/spriteWidth*255),
				255-(int)((double)(x)/spriteWidth*255),
				abs(128-(int)((double)(x)/spriteWidth*255)), 0);
				SDL_RenderDrawLine(renderer, drawStartX + x, drawStartY, drawStartX + x, drawEndY);
			}
#else
				for (int y = 0; y < spriteHeight; y++)
				{
					int dx = spriteWidth / 2 - x;
					int dy = spriteHeight / 2 - y;
					if ((dx*dx + dy*dy) > (spriteWidth*spriteHeight/4))
					{
					SDL_SetRenderDrawColor(renderer, 5, 10, 5, 0);
					}
					else
					{
					SDL_SetRenderDrawColor(renderer,
#if GRAPHICS==CRAZY
					(int)((double)(x*y)/spriteWidth * spriteHeight * 255),
					(int)((double)(x*x)/spriteWidth * spriteWidth * 255),
					(int)((double)(y*y)/spriteHeight * spriteHeight * 255), 0);
#else
					(int)((double)(x)/spriteWidth * 255),
					(int)((double)255),
					(int)((double)(y)/spriteHeight * 255), 0);
#endif
					}

					
					SDL_RenderDrawPoint(renderer,  drawStartX + dx, drawStartY + dy);
				}
			}
#endif
		}

#if DEBUG
		printf("MONSTER: %lld\n", SDL_GetTicks64() - section1);
		section1 = SDL_GetTicks64();
#endif

		oldTime = time;
		time = SDL_GetTicks64(); /* in milliseconds */

		frametime = (time - oldTime); /* in milliseconds */

		movespeed = frametime / 400.0; /* in squares/second */
		rotspeed = frametime / 250.0; /* in radians/second */
		 
		/* update the screen */
		SDL_RenderPresent( renderer );

#if DEBUG
		printf("RENDER: %lld\n", SDL_GetTicks64() - section1);
#endif

		/* take a quick break after all that hard work */
		if (FPS) { if (1000/FPS - frametime > 0) SDL_Delay( (int)(1000/FPS - frametime)); }
		else     { SDL_Delay( 1               ); }

	}

	quit_5: SDL_CloseAudioDevice(deviceID);

	quit_4: SDL_FreeWAV(wav_buffer);
	
        /* destroy renderer */
	quit_3: SDL_DestroyRenderer( renderer );
	renderer = NULL;

	/* destroy window */
	quit_2: SDL_DestroyWindow( window );
	window = NULL;

	/* quit SDL subsystems */
	quit_1: SDL_Quit();
	return EXIT_CODE;
}
