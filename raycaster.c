#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600
#define MAP_WIDTH 15
#define MAP_HEIGHT 8

#define PI 3.1415926535897932384626433
#define IDONTWANTTHIS 0

static const uint8_t map[]=
{
 7,1,2,3,4,5,6,7,1,2,3,4,5,6,7,
 6,0,0,0,0,0,0,0,0,0,0,0,0,0,6,
 5,0,3,0,0,0,0,0,0,0,0,0,0,0,5,
 4,0,0,0,0,3,3,3,0,0,0,0,0,0,4,
 3,0,0,0,0,3,0,3,0,0,0,0,0,0,3,
 2,0,0,0,0,3,3,3,0,0,0,0,0,0,2,
 1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
 7,6,5,4,3,2,1,2,3,4,5,6,7,1,2,
};

#ifdef IDONTWANTTHIS
/*
 111
2   3
2   3
 444
5   6
5   6
 777
*/
static const uint8_t digits[]=
{
/*1,2,3,4,5,6,7*/
  1,1,1,0,1,1,1, /* 0 */
  0,0,1,0,0,1,0, /* 1 */
  1,0,1,1,1,0,1, /* 2 */
  1,0,1,1,0,1,1, /* 3 */
  0,1,1,1,0,1,0, /* 4 */
  1,1,0,1,0,1,1, /* 5 */
  1,1,0,1,1,1,1, /* 6 */
  1,1,1,0,0,1,0, /* 7 */
  1,1,1,1,1,1,1, /* 8 */
  1,1,1,1,0,1,1  /* 9 */
};
#endif

/*
Log an SDL error with some error message to the output stream of our choice
@param msg The error message to write, format will be msg error: SDL_GetError()
*/
void logSDLError( char *msg, const char *error ) {
	printf( "%s error: %s\n", msg, error );
}

int main( int argc, char *argv[] ) {
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

	/* Game Code */

	/* Variables */
	uint8_t quit = 0;
	double playerX = 4;
	double playerY = 4;
	double playerAngle = PI/2;
	double movespeed = 0.0875, rotspeed = 0.07;
	int FPS = 0;
	int FOV = 100;

	if (argc > 1)
	{
	printf("hllo");
	FPS = atoi(argv[1]);
	}

	long long int time = 0, oldTime = 0, frametime;
	
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
						
						double newX = cos(playerAngle) * movespeed;
						double newY = sin(playerAngle) * movespeed;
						
#define MAPPOSX ((int)(playerY) * MAP_WIDTH + (int)(playerX + newX))
#define MAPPOSY ((int)(playerY + newY) * MAP_WIDTH + (int)(playerX))

						playerX += (map[MAPPOSX] == 0) * newX;
						playerY += (map[MAPPOSY] == 0) * newY;

#undef MAPPOSX
#undef MAPPOSY
						
						break;

						/* Right Arrow */
						case SDLK_RIGHT:
						playerAngle += rotspeed;
						break;
						
						/* Left Arrow */
						case SDLK_LEFT:
						playerAngle -= rotspeed;
						break;

						/* Down Arrow */
						case SDLK_DOWN:
						break;
						
						/* Space */
						case SDLK_SPACE:
						break;

					}
					break;
			}
		}

		playerAngle = fmod(playerAngle + 2*PI, 2*PI);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

		/* Clear the renderer */
		SDL_RenderClear( renderer );


		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			double rayangle = playerAngle - PI*FOV/720 + (PI*FOV*x)/(SCREEN_WIDTH*360);

			double helper_sin = sin(rayangle);
			double helper_cos = cos(rayangle);

			int mapX = (int)(playerX);
			int mapY = (int)(playerY);
			int mapPos;

			double rayDistX;
			double rayDistY;

			double deltaDistX = (helper_cos == 0) ? 1e30 : fabs(1 / helper_cos);
			double deltaDistY = (helper_sin == 0) ? 1e30 : fabs(1 / helper_sin);
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
				rayDistX = (mapX + 1 - playerX) * deltaDistX;
			}
			
			if (helper_sin < 0)
			{
				stepY = -1;
				rayDistY = (playerY - mapY) * deltaDistY;
			}
			else
			{
				stepY = 1;
				rayDistY = (mapY + 1 - playerY) * deltaDistY;
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
			lineHeight = (int)(SCREEN_WIDTH / (rayDistX - deltaDistX) / cos(rayangle - playerAngle));
			}
			else
			{
			lineHeight = (int)(SCREEN_WIDTH / (rayDistY - deltaDistY) / cos(rayangle - playerAngle));
			}
			
#define DRAW_Y1 (-lineHeight / 2 + SCREEN_HEIGHT / 2 < 0) ? 0 : -lineHeight / 2 + SCREEN_HEIGHT / 2
#define DRAW_Y2 (lineHeight / 2 + SCREEN_HEIGHT / 2 < SCREEN_HEIGHT) ? lineHeight / 2 + SCREEN_HEIGHT / 2 : SCREEN_HEIGHT - 1

			SDL_SetRenderDrawColor(renderer, (map[mapPos] & 4)*255, (map[mapPos] & 2)*255, (map[mapPos] & 1)*255, 0);
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
		SDL_RenderDrawLine(renderer, 55, 55, 55 - cos(playerAngle) * 35, 55 - sin(playerAngle) * 35);

		printf("%f, %f\n", playerX, playerY);

		oldTime = time;
		time = SDL_GetTicks64(); /* in milliseconds */

		frametime = (time - oldTime); /* in milliseconds */

		movespeed = frametime / 400.0; /* in squares/second */
		rotspeed = frametime / 500.0; /* in radians/second */
		 
		/* Update the screen */
		SDL_RenderPresent( renderer );

		oldTime = time;
		time = SDL_GetTicks64(); /* in milliseconds */

		frametime = (time - oldTime); /* in milliseconds */

		movespeed = frametime / 400.0; /* in squares/second */
		rotspeed = frametime / 500.0; /* in radians/second */

		/* Take a quick break after all that hard work */
		if (FPS) SDL_Delay( (int)(1000/FPS) );

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
