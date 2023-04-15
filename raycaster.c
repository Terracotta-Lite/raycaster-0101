#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600
#define MAP_WIDTH 8
#define MAP_HEIGHT 8

#define PI 3.1415926535897932384626433
#define IDONTWANTTHIS 0

static const uint8_t map[]=
{
 7,1,2,3,4,5,6,7,
 6,0,0,0,0,0,0,1,
 5,0,3,0,0,0,0,2,
 4,0,0,0,0,0,0,3,
 3,0,0,0,0,0,0,4,
 2,0,0,0,0,3,0,5,
 1,0,0,0,0,0,0,6,
 7,6,5,4,3,2,1,7
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

int main( int argc, char **argv ) {
	uint8_t EXIT_CODE = 0;

	/* Initialize SDL */
	if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
		logSDLError( "SDL_Init" , SDL_GetError() );
		EXIT_CODE = 1;
		goto quit_1;
	}
	
	/* Create Window */
	SDL_Window *window = SDL_CreateWindow( "Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0 );
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
	double movespeed, rotspeed;
	long long int time, oldTime = 0, frametime;
	
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
						playerX += ((playerAngle > PI/2 && playerAngle < PI/2+PI) ? -1 : 1) *
						(map[(int)(playerY) * MAP_WIDTH + (int)(playerX + cos(playerAngle) * movespeed - 1)] == 0) ? cos(playerAngle) * movespeed : 0;
						playerY += ((playerAngle > PI) ? -1 : 1) *
						(map[(int)(playerY + sin(playerAngle) * movespeed) * MAP_WIDTH + (int)(playerX)] == 0) ? sin(playerAngle) * movespeed : 0;
						/*
						playerX += cos(playerAngle) * movespeed;
						playerY += sin(playerAngle) * movespeed;
						*/
						break;

						/* Right Arrow */
						case SDLK_RIGHT:
						playerAngle += 0.25;
						break;
						
						/* Left Arrow */
						case SDLK_LEFT:
						playerAngle -= 0.25;
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

		playerAngle = fmod(playerAngle, 2*PI);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

		/* Clear the renderer */
		SDL_RenderClear( renderer );


		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			double rayangle = playerAngle - PI/6 + (PI*x)/(SCREEN_WIDTH*3);

			double helper_sin = sin(rayangle);
			double helper_cos = cos(rayangle);

			int mapX = (int)(playerX);
			int mapY = (int)(playerY);
			int mapPos;

			double deltaDistX = (helper_cos == 0) ? 1e30 : fabs(1 / helper_cos);
			double deltaDistY = (helper_sin == 0) ? 1e30 : fabs(1 / helper_sin);

			int8_t stepX, stepY;
			double rayDistX, rayDistY;
			
			if (helper_cos < 0)
			{
				stepX = -1;
				rayDistX = (mapX - playerX) * deltaDistX;
			}
			else
			{
				stepY = 1;
				rayDistX = (mapX + 1 - playerX) * deltaDistX;
			}
			
			if (helper_sin < 0)
			{
				stepY = -1;
				rayDistY = (mapY - playerY) * deltaDistY;
			}
			else
			{
				stepY = 1;
				rayDistY = (mapY + 1 - playerY) * deltaDistY;
			}

			int hit = 0;
			int side;
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

				mapPos = mapY * MAP_WIDTH + mapX - 1;

				if (map[mapPos]) hit = 1;
			}

			double ultimateDist;
			if(side == 0) { ultimateDist = (rayDistX - deltaDistX); }
			else          { ultimateDist = (rayDistY - deltaDistY); }
			
			int lineHeight = (int)(SCREEN_WIDTH / ultimateDist);
			
#define DRAW_Y1 (-lineHeight / 2 + SCREEN_HEIGHT / 2 > 0) ? -lineHeight / 2 + SCREEN_HEIGHT / 2 : 0
#define DRAW_Y2 (lineHeight / 2 + SCREEN_HEIGHT / 2 < SCREEN_HEIGHT) ? lineHeight / 2 + SCREEN_HEIGHT / 2 : SCREEN_HEIGHT - 1

			SDL_SetRenderDrawColor(renderer, (map[mapPos] & 4)*255, (map[mapPos] & 2)*255, (map[mapPos] & 1)*255, 0);
			SDL_RenderDrawLine(renderer, x, DRAW_Y1, x, DRAW_Y2); 

#undef DRAW_Y1
#undef DRAW_Y2

		}

		oldTime = time;
		time = SDL_GetTicks64(); /* in milliseconds */

		frametime = (time - oldTime); /* in milliseconds */

		movespeed = frametime / 400.0; /* in squares/second */
		rotspeed = frametime; /* in radians/second */
		
		printf("x:%f  y:%f\n", playerX, playerY);
		 
		/* Update the screen */
		SDL_RenderPresent( renderer );

		/* Take a quick break after all that hard work */
		SDL_Delay( 33 );

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
