#ifdef _WIN32
  	#include "SDL.h"
#else
	#include <SDL2/SDL.h>
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

/* Both must be odd numbers
for some reason it doesnt
work when the numbers are not the same
will fix it */
#define MAP_WIDTH 31
#define MAP_HEIGHT 31

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
const int wavLength[] = {0, 630876, 948408, 1408540};

/* variable declarations */
static Uint8 *audio_pos; /* global pointer to the audio buffer to be played */
static Uint32 audio_len; /* remaining length of the sample we have to play */
static Uint8  audio_selected = TRACK_NONE;

/*
Log an SDL error with some error message to the output stream of our choice
@param msg The error message to write, format will be msg error: SDL_GetError()
*/
void logSDLError( char *msg, const char *error ) {
	printf( "%s error: %s\n", msg, error );
}

SDL_Texture *loadTexture( const char *path )
{
	SDL_Texture *newTexture = NULL;
	
	SDL_Surface *loadedSurface = SDL_LoadBMP(path);

	if (loadedSurface == NULL)
	{
		logSDLError("SDL_LoadBMP", SDL_GetError());
	}
	else
	{
		newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
		if (newTexture == NULL)
		{
			logSDLError("SDL_CreateTextureFromSurface", SDL_GetError());
		}

		SDL_FreeSurface(loadedSurface);
	}

	return newTexture;
}



/* Arrange the N elements of ARRAY in random order.
   Ony effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
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

uint8_t map[MAP_WIDTH*MAP_HEIGHT];

/* 1D zortbuffer, ordered by vertical slices */
double distBuffer[SCREEN_WIDTH];

void sortSprites(int *order, double *dist, int amount);

int main( int argc, char *argv[] ) {
	srand(time(NULL));
	uint8_t EXIT_CODE = 0;

	/* Initialize SDL */
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) != 0 )
	{
		logSDLError( "SDL_Init" , SDL_GetError() );
		EXIT_CODE = 1;
		goto quit_1;
	}
	
	/* Create Window */
	SDL_Window *window = SDL_CreateWindow( "Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE );
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

	// local variables
	static Uint32 wav_length; // length of our sample
	static Uint8 *wav_buffer; // buffer containing our audio file
	static SDL_AudioSpec wav_spec; // the specs of our piece of music
	
	/* Load the WAV */
	// the specs, length and buffer of our wav are filled
	if (SDL_LoadWAV(MUS_PATH, &wav_spec, &wav_buffer, &wav_length) == NULL)
	{
		logSDLError("SDL_LoadWAV", SDL_GetError());
		EXIT_CODE = 4;
		goto quit_4;
	}

	// set our global static variables
	audio_pos = wav_buffer + 603876; // copy sound buffer
	audio_len = wav_length - 1091008; // copy file length
	
	/* Open the audio device */
	SDL_AudioDeviceID deviceID = SDL_OpenAudioDevice(NULL, 0, &wav_spec, NULL, 0);
	if (deviceID == 0)
	{
		logSDLError("SDL_OpenAudioDevice", SDL_GetError());
		EXIT_CODE = 5;
		goto quit_5;
	}

	SDL_QueueAudio(deviceID, audio_pos, audio_len);
	
	/* Start playing */
	SDL_PauseAudioDevice(deviceID, 0);

	createMap(map);

	/* Game Code */

	/* Variables */
	uint8_t quit = 0;

	int FOV = 80;
	int FPS = 0;
	double planeX = 0;
	double planeY = atan(FOV/100.0);

	double playerX = (MAP_WIDTH - (MAP_WIDTH % 4))/2 + 0.5;
	double playerY = (MAP_HEIGHT - (MAP_HEIGHT % 4))/2 + 0.5;
	
	double dirX = -1;
	double dirY = 0;
	double movespeed = 0.0875, rotspeed = 0.07;

	double newX;
	double newY;
	double rotcos;
	double rotsin;
	double oldDirX;
	double oldPlaneX;

	if (argc > 1)
	{
	FPS = atoi(argv[1]);
	}

	long long unsigned int time = 0;
	long long unsigned int oldTime = 0;
	long long unsigned int frametime;

	SDL_CaptureMouse(SDL_TRUE);
	SDL_ShowCursor(SDL_DISABLE);
	
	SDL_Event e;
	while ( quit == 0 ) {
		while ( SDL_PollEvent(&e) ) {
			switch ( e.type ) {
				case SDL_QUIT:
					quit = 1;
					break;
			
				case SDL_KEYDOWN:
				switch ( e.key.keysym.sym ) {

					/* Mission Critical */

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
						audio_pos = wav_buffer + 603876; // copy sound buffer
						audio_len = wav_length - 1091008; // copy file length
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

		/*playerAngle = fmod(playerAngle + 2*PI, 2*PI);*/

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

		/* Clear the renderer */
		SDL_RenderClear( renderer );

		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			/*double rayangle = playerAngle - PI*FOV/360 + (PI*FOV*x)/(SCREEN_WIDTH*180);*/
			double cameraX = 2 * x / (double)SCREEN_WIDTH - 1;
			double helper_cos = dirX + planeX * cameraX;
			double helper_sin = dirY + planeY * cameraX;

			int mapX = (int)(playerX);
			int mapY = (int)(playerY);
			int mapPos;

			double rayDistX;
			double rayDistY;

			double deltaDistX = (helper_cos == 0) ? 1e30 : fabs(1 / helper_cos);
			double deltaDistY = (helper_sin == 0) ? 1e30 : fabs(1 / helper_sin);
			double ultimateDist;
			int lineHeight;

			int8_t stepX;
			int8_t stepY;
			
			int hit = 0;
			int side;
			
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

			distBuffer[x] = ultimateDist;

			lineHeight = (int)(SCREEN_WIDTH / ultimateDist);
			
#define DRAW_Y1 (-lineHeight / 2 + SCREEN_HEIGHT / 2 < 0) ? 0 : -lineHeight / 2 + SCREEN_HEIGHT / 2
#define DRAW_Y2 (lineHeight / 2 + SCREEN_HEIGHT / 2 < SCREEN_HEIGHT) ? lineHeight / 2 + SCREEN_HEIGHT / 2 : SCREEN_HEIGHT - 1

			SDL_SetRenderDrawColor(renderer, (map[mapPos] & 4)*63, (map[mapPos] & 2)*63, (map[mapPos] & 1)*63, 0);
			SDL_RenderDrawLine(renderer, x, DRAW_Y1, x, DRAW_Y2); 

#undef DRAW_Y1
#undef DRAW_Y2

		}

		SDL_Rect plane;
		plane.x = 20;
		plane.y = 20;
		plane.w = 70;
		plane.h = 70;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderFillRect(renderer, &plane);
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
		SDL_RenderDrawLine(renderer, 55, 55, 55 - dirX * 35, 55 - dirY * 35);

		/*printf("%f, %f\n", playerX, playerY);*/

		oldTime = time;
		time = SDL_GetTicks64(); /* in milliseconds */

		frametime = (time - oldTime); /* in milliseconds */

		movespeed = frametime / 400.0; /* in squares/second */
		rotspeed = frametime / 250.0; /* in radians/second */
		 
		/* Update the screen */
		SDL_RenderPresent( renderer );

		/* Take a quick break after all that hard work */
		if (FPS) { SDL_Delay( (int)(1000/FPS) ); }
		else     { SDL_Delay( 1               ); }

	}

	quit_5: SDL_CloseAudioDevice(deviceID);

	quit_4: SDL_FreeWAV(wav_buffer);
	
        /* Destroy renderer */
	quit_3: SDL_DestroyRenderer( renderer );
	renderer = NULL;

	/* Destroy window */
	quit_2: SDL_DestroyWindow( window );
	window = NULL;

	/* Quit SDL subsystems */
	quit_1: SDL_Quit();
	return EXIT_CODE;
}
