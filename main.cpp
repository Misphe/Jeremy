#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <string>

using std::string;
using std::ostringstream;

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define FULLSCREEN false


//player's walking speed in pixels per second
#define HORIZONTAL_MOVEMENT 250
#define VERTICAL_MOVEMENT 150
#define PLATFORMS 7

#define GRAVITY 0.004
#define JUMP_POWER 0.4
#define HOLD_POWER 0.5
#define MAP_SPEED 0.2

#define STARTING_Y (SCREEN_HEIGHT/2)
#define STARTING_X 200

#define PLATFORMS_Y_DIFFERENCE 40
#define PLATFORM_WIDTH (150 + rand()%100)
#define PLATFORM_GAP (40 + rand()%40)

#define ENEMY_SPEED 0.25




bool quit;
bool gameover = false;
bool new_game;
bool pause = false;

Uint8 red = 255;
Uint8 blue = 255;
Uint8 green = 255;

struct Jump {
	bool in_jump;
	float jump_value;
}jump;


void DrawSurface(SDL_Renderer* renderer, SDL_Texture* texture, int x, int y) {
	// Get the width and height of the texture
	int w, h;
	SDL_QueryTexture(texture, NULL, NULL, &w, &h);

	// Create the destination rectangle
	SDL_Rect dest;
	dest.x = x - w / 2;
	dest.y = y - h / 2;
	dest.w = w;
	dest.h = h;

	// Render the texture to the screen
	SDL_RenderCopy(renderer, texture, NULL, &dest);
}


// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};


// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) 
// bπdü poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
	};


struct sdl_structures {
	SDL_Event event;
	SDL_Surface* charset;
	SDL_Surface* screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	SDL_Surface* JeremyLeft;
	SDL_Surface* JeremyRight;
	SDL_Surface* EnemyLeft;
	SDL_Surface* EnemyRight;
	SDL_Texture* player;
	SDL_Texture* enemy;
	SDL_Texture* charsetTexture;
	SDL_Window* window;
	SDL_Renderer* renderer;
}sdl;

void DrawText(string text, int width, int height, int x, int y) {
	int charW = 8;
	int charH = 8;
	Uint8 red = 255;
	Uint8 green = 0;
	Uint8 blue = 0;
	for (char c : text) {
		int charIndex = c;
		int charX = (charIndex % 16) * charW;
		int charY = (charIndex / 16) * charH;
		SDL_Rect charRect = { charX, charY, charW, charH };
		SDL_Rect destRect = { x, y, width, height };
		SDL_SetRenderDrawColor(sdl.renderer, red, green, blue, 255);
		SDL_RenderFillRect(sdl.renderer, &destRect);
		SDL_RenderCopy(sdl.renderer, sdl.charsetTexture, &charRect, &destRect);
		x += width;
	}
}


struct Player {
	float x;
	float y;
	float pre_x;
	float pre_y;
}player;

struct Platforms {
	float x;
	float y;
	float width;
}platform[PLATFORMS];

struct Enemy {
	float x;
	float y;
	float speed;
	int width = 30;
	int height = 40;
	int plat;
}enemy[PLATFORMS];



struct colors {
	int black= SDL_MapRGB(sdl.screen->format, 0x00, 0x00, 0x00);
	int green = SDL_MapRGB(sdl.screen->format, 0x00, 0xFF, 0x00);
	int red = SDL_MapRGB(sdl.screen->format, 0xFF, 0x00, 0x00);
	int blue = SDL_MapRGB(sdl.screen->format, 0x11, 0x11, 0xCC);
	int white = SDL_MapRGB(sdl.screen->format, 255, 255, 255);
}const color;

void DrawRect(double x, double y, double width, int red, int green, int blue) {
	SDL_SetRenderDrawColor(sdl.renderer, red, green, blue, 255);
	SDL_Rect rect = { x, y, width, SCREEN_HEIGHT - y};
	SDL_RenderFillRect(sdl.renderer, &rect);
}

void FreeSDL() {
	SDL_FreeSurface(sdl.charset);
	SDL_FreeSurface(sdl.screen);
	SDL_DestroyTexture(sdl.charsetTexture);
	SDL_DestroyWindow(sdl.window);
	SDL_DestroyRenderer(sdl.renderer);
	SDL_Quit();
}

void InitSDL() {
	int rc;
	if (FULLSCREEN == true)rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, &sdl.window, &sdl.renderer);
	else rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdl.window, &sdl.renderer);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(sdl.renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(sdl.window, "Bank");


	sdl.charset = SDL_LoadBMP("./charset.bmp");
	sdl.charsetTexture = SDL_CreateTextureFromSurface(sdl.renderer, sdl.charset);
	SDL_FreeSurface(sdl.charset);


	SDL_ShowCursor(SDL_DISABLE);
	sdl.JeremyLeft = SDL_LoadBMP("./sprites/JeremyLeftBMP.bmp");
	sdl.JeremyRight = SDL_LoadBMP("./sprites/JeremyRightBMP.bmp");
	sdl.player = SDL_CreateTextureFromSurface(sdl.renderer, sdl.JeremyLeft);

	sdl.EnemyLeft = SDL_LoadBMP("./sprites/EnemyLeft.bmp");
	sdl.EnemyRight = SDL_LoadBMP("./sprites/EnemyRight.bmp");
	sdl.enemy = SDL_CreateTextureFromSurface(sdl.renderer, sdl.EnemyLeft);
}

void RenderScreen() {
	//SDL_UpdateTexture(sdl.scrtex, NULL, sdl.screen->pixels, sdl.screen->pitch);
  //SDL_RenderClear(renderer);
	//SDL_RenderCopy(sdl.renderer, sdl.scrtex, NULL, NULL);
	SDL_RenderPresent(sdl.renderer);
	SDL_SetRenderDrawColor(sdl.renderer, 33, 150, 243, 255); //white color
	SDL_RenderClear(sdl.renderer);
}

bool EmptySpace(float x, float y) {
	float x_right = x + sdl.JeremyLeft->w / 2;
	float x_left = x - sdl.JeremyLeft->w / 2;
	float y_down = y + sdl.JeremyLeft->h / 2;
	float pre_y_down = player.pre_y + sdl.JeremyLeft->h / 2;
	for (int i = 0; i < PLATFORMS; i++) {
		if (x_right >= platform[i].x && x_right <= platform[i].x + platform[i].width && pre_y_down > platform[i].y) {
			return false;
		}
		if (x_left >= platform[i].x && x_left <= platform[i].x + platform[i].width && pre_y_down > platform[i].y) {
			return false;
		}
	}
	return true;
}

void Move(const Uint8* input, double delta) {
	while (SDL_PollEvent(&sdl.event)) {
		switch (sdl.event.type) {
			case SDL_KEYDOWN:
				if (sdl.event.key.keysym.sym == SDLK_ESCAPE) {
					quit = true;
				}
				if (!pause && !gameover) {
					if (sdl.event.key.keysym.sym == SDLK_LEFT) {
						SDL_DestroyTexture(sdl.player);
						sdl.player = SDL_CreateTextureFromSurface(sdl.renderer, sdl.JeremyLeft);
					}
					if (sdl.event.key.keysym.sym == SDLK_RIGHT) {
						SDL_DestroyTexture(sdl.player);
						sdl.player = SDL_CreateTextureFromSurface(sdl.renderer, sdl.JeremyRight);
					}
				}
				if (sdl.event.key.keysym.sym == SDLK_p) {
					pause = !pause;
				}
				if (sdl.event.key.keysym.sym == SDLK_n) {
					new_game = true;
				}
				break;
			case SDL_KEYUP:
				break;

			case SDL_QUIT:
				quit = true;
				break;
		}
	}
	if (!pause && !gameover) {
		if (input[SDL_SCANCODE_LEFT]) {
			player.pre_x = player.x;
			player.x -= HORIZONTAL_MOVEMENT / 1000.0 * delta;
			if (!EmptySpace(player.x, player.y)) {
				player.x += HORIZONTAL_MOVEMENT / 1000.0 * delta;
			}
		}
		else if (input[SDL_SCANCODE_RIGHT]) {
			player.pre_x = player.x;
			player.x += HORIZONTAL_MOVEMENT / 1000.0 * delta;
			if (!EmptySpace(player.x, player.y)) {
				player.x -= HORIZONTAL_MOVEMENT / 1000.0 * delta;
			}
		}
		if (input[SDL_SCANCODE_UP]) {
			if (!jump.in_jump) {
				jump.in_jump = true;
				jump.jump_value = 1;
			}
			jump.jump_value += GRAVITY * delta * HOLD_POWER;
		}
		else if (input[SDL_SCANCODE_DOWN]) {
			//player.y += VERTICAL_MOVEMENT / 1000.0 * delta;
		}
	}
}

void EnemyMove(double delta) {
	for (int i = 0; i < PLATFORMS; i++) {
		if (delta > 0){
			if (enemy[i].x + enemy[i].width > platform[enemy[i].plat].x + platform[enemy[i].plat].width || enemy[i].x < platform[enemy[i].plat].x) {
				enemy[i].speed = -enemy[i].speed;
			}
		enemy[i].x += enemy[i].speed * delta * ENEMY_SPEED;
		}
	}
}

void DrawSprites() {
	DrawSurface(sdl.renderer, sdl.player, player.x, player.y);
	for (int i = 0; i < PLATFORMS; i++) {
		DrawSurface(sdl.renderer, sdl.enemy, enemy[i].x + enemy[i].width/2, enemy[i].y - enemy[i].height/2);
		DrawRect(platform[i].x, platform[i].y, platform[i].width, 102, 102, 102);
	}
}

void MapMove(float delta) {
	player.x -= delta * MAP_SPEED;
	for (int i = 0; i < PLATFORMS; i++) {
		platform[i].x -= delta * MAP_SPEED;
		enemy[i].x -= delta * MAP_SPEED;
	}
}

void SetPlatforms() {
	platform[0].x = 0;
	platform[0].width = 300;
	platform[0].y = STARTING_Y + sdl.JeremyLeft->h / 2;
	enemy[0].x = platform[0].x;
	enemy[0].y = platform[0].y;
	enemy[0].plat = 0;
	enemy[0].speed = (rand() % 1000)/1000.0;
	for (int i = 1; i < PLATFORMS; i++) {
		platform[i].x = platform[i-1].x + platform[i-1].width + 35 + rand()%70;
		platform[i].y = STARTING_Y + sdl.JeremyLeft->h/2;
		platform[i].width = PLATFORM_WIDTH;
		platform[i].y = platform[i-1].y + rand() % (2 * PLATFORMS_Y_DIFFERENCE + 1) - PLATFORMS_Y_DIFFERENCE;
		enemy[i].x = platform[i].x;
		enemy[i].y = platform[i].y;
		enemy[i].plat = i;
		enemy[i].speed = (rand() % 1000) / 1000.0;
	}
}

void ResetPlatforms() {
	if (platform[0].x + platform[0].width < 0) {
		if (platform[6].x + platform[6].width > SCREEN_WIDTH) {
			platform[0].x = platform[6].x + platform[6].width + PLATFORM_GAP;
		}
		else platform[0].x = SCREEN_WIDTH;

		platform[0].width = PLATFORM_WIDTH;
		platform[0].y = platform[PLATFORMS - 1].y + rand() % (2 * PLATFORMS_Y_DIFFERENCE + 1) - PLATFORMS_Y_DIFFERENCE;
		if (platform[0].y > SCREEN_HEIGHT - 50 || platform[0].y < 100) {
			platform[0].y = platform[PLATFORMS - 1].y;
		}
		enemy[0].x = platform[0].x;
		enemy[0].y = platform[0].y;
		enemy[0].plat = 0;
		enemy[0].speed = (rand() % 1000) / 1000.0;
	}

	for (int i = 1; i < PLATFORMS; i++) {
		if (platform[i].x + platform[i].width < 0) {
			if (platform[i-1].x + platform[i-1].width > SCREEN_WIDTH) {
				platform[i].x = platform[i-1].x + platform[i-1].width + PLATFORM_GAP;
			}
			else platform[i].x = SCREEN_WIDTH;
			platform[i].width = PLATFORM_WIDTH;
			platform[i].y = platform[i - 1].y + rand() % (2 * PLATFORMS_Y_DIFFERENCE + 1) - PLATFORMS_Y_DIFFERENCE;
			if (platform[i].y > SCREEN_HEIGHT - 50 || platform[i].y < 100) {
				platform[i].y = platform[i - 1].y;
			}
			enemy[i].x = platform[i].x;
			enemy[i].y = platform[i].y;
			enemy[i].plat = i;
			enemy[i].speed = (rand() % 1000) / 1000.0;
		}
	}
}

void DoJump(double delta) {
	bool in_jump = jump.in_jump;
	double x = player.x - sdl.JeremyLeft->w / 2;
	if (in_jump) {
		player.pre_y = player.y;
		player.y -= JUMP_POWER * jump.jump_value * delta;
		jump.jump_value -= GRAVITY * delta;
	}
}

void Fall() {
	double x = player.x - sdl.JeremyLeft->w / 2;
	double y = player.y - sdl.JeremyLeft->h / 2;
	double pre_y = player.pre_y - sdl.JeremyLeft->h / 2;
	for (int i = 0; i < PLATFORMS; i++) {
		if (x <= platform[i].x + platform[i].width && x >= platform[i].x - sdl.JeremyLeft->w && y >= platform[i].y - sdl.JeremyLeft->h) {
			jump.in_jump = false;
			break;
		}
		if (i == PLATFORMS - 1 && !jump.in_jump) {
			jump.in_jump = true;
			jump.jump_value = -0.001;
		}
	}
}

void DisplayPoints(int timer) {
	string text;
	text = "Points: " + std::to_string(timer / 1000);
	DrawText(text, 16, 16, SCREEN_WIDTH - 200, 50);
}

void GameOver() {
	int y_dif = sdl.JeremyLeft->h / 2;
	int x_dif = sdl.JeremyLeft->w / 2;
	if (player.y - y_dif >= SCREEN_HEIGHT || player.x + x_dif <= 0) {
		gameover = true;
	}

	for (int i = 0; i < PLATFORMS; i++) {
		if (player.x + x_dif > enemy[i].x && player.x - x_dif < enemy[i].x + enemy[i].width) {
			if (player.y + y_dif > enemy[i].y - enemy[i].height && player.y - y_dif < enemy[i].y) {
				gameover = true;
			}
		}
	}
}

void GameOverScreen() {
	if (gameover) {
		string text = "You lost! Press n to restart";
		DrawText(text, 20, 20, SCREEN_WIDTH / 2 - (text.size() * 20 / 2), SCREEN_HEIGHT / 2 - 10);
	}
}


// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	int t1, t2;
	jump.in_jump = false;
	srand(time(NULL));

	double delta;
	float distance = 0;
	const Uint8* input = SDL_GetKeyboardState(NULL);
	InitSDL();
	quit = false;

	t1 = t2 = SDL_GetTicks();
	int timer = 0;

	while(!quit) {
		t1 = t2 = SDL_GetTicks();
		timer = 0;
		new_game = false;
		gameover = false;
		SetPlatforms();
		player.pre_x = 100;
		player.pre_y = SCREEN_HEIGHT / 2;
		player.x = 100;
		player.y = SCREEN_HEIGHT / 2;
		while (!new_game && !quit ) {

			delta = SDL_GetTicks() - t2;
			t2 = SDL_GetTicks();

			if (!pause && !gameover) {
				timer += delta;
				MapMove(delta);
				EnemyMove(delta);
				ResetPlatforms();
				DoJump(delta);
				Fall();
				//GameOver();
			}
			DrawSprites();
			DisplayPoints(timer);
			GameOverScreen();
			RenderScreen();
			Move(input, delta);
		}
	}
	FreeSDL();
	return 0;
	};
