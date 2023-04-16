#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600

/* Both must be odd numbers
for some reason it doesnt
work when the numbers are not the same
will fix it */
#define MAP_WIDTH 31
#define MAP_HEIGHT 31

#define PI 3.1415926535897932384626433

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

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
				printf("O");
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
	if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
		logSDLError( "SDL_Init" , SDL_GetError() );
		EXIT_CODE = 1;
		goto quit_1;
	}
	
	/* Create Window */
	SDL_Window *window = SDL_CreateWindow( "Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE );
	if ( window == NULL ) {
		logSDLError( "CreateWindow", SDL_GetError() );
		EXIT_CODE = 2;
		goto quit_2;
	}

	/* Create Renderer */
	SDL_Renderer *renderer = SDL_CreateRenderer( window,  -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	if ( renderer == NULL ) {
		logSDLError( "SDL_CreateRenderer", SDL_GetError() );
		EXIT_CODE = 3;
		goto quit_3;
	}

	createMap(map);

	/* Game Code */

	/* Variables */
	uint8_t quit = 0;

	int FOV = 60;
	int FPS = 0;
	double planeX = atan(FOV/100.0);
	double planeY = 1;

	double playerX = 4;
	double playerY = 4;
	/*double playerAngle = PI/2;*/
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
						/* ESCAPE (for quit) */
						case SDLK_ESCAPE:
						quit = 1;
						break;
						
						/* -- Player Movement -- */

						case SDLK_UP:
						case SDLK_w:
						/* TODO: Fix getting into block because of the order of checks */
						
						newX = dirX * movespeed;
						newY = dirY * movespeed;
						
#define MAPPOSX ((int)(playerY) * MAP_WIDTH + (int)(playerX + newX))
#define MAPPOSY ((int)(playerY + newY) * MAP_WIDTH + (int)(playerX))

						playerX += (map[MAPPOSX] == 0) * newX;
						playerY += (map[MAPPOSY] == 0) * newY;

#undef MAPPOSX
#undef MAPPOSY
						
						break;

						/* Right Arrow */
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
						
						/* Left Arrow */
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

					}
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

			/*
			double helper_sin = sin(rayangle);
			double helper_cos = cos(rayangle);
			*/

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
				stepY = 1;
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
