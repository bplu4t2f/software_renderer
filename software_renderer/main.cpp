#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <vector>

#include "software_renderer.h"

using namespace std;

struct Line
{
	Line(float x1, float y1, float x2, float y2)
	{
		this->x1 = x1;
		this->y1 = y1;
		this->x2 = x2;
		this->y2 = y2;
	}
	float x1, y1, x2, y2;
};

struct Renderer_State
{
	// Actually we shouldn't call this RGBA. It doesn't matter what the pixel format is,
	// so it's more beneficial to define this so that it matches the surface's pixel
	// format, so that we can optimize the fragment shader as much as possible.
	uint32_t color_rgba;
	SDL_Surface *surface;
};

static void fragment_shader(void *user_state, int x, int y, int alpha)
{
	// If anyone passes NULL or something that isn't a valid Renderer_State,
	// it's their fault for crashing the program.
	Renderer_State *state = (Renderer_State*)user_state;
	SDL_Surface *surface = state->surface;

	// "Scissor test" using the entire thing as clipping region
	if (x < 0 || x >= surface->w || y < 0 || y >= surface->h)
	{
		return;
	}

	int offset = y * surface->pitch + x * surface->format->BytesPerPixel;
	uint32_t *pixel = (uint32_t *)((char *)surface->pixels + offset);
	// TODO: This assumes RGBA pixel format. Check surface->format and act accordingly.
	uint32_t rgb = state->color_rgba & 0xffffff;
	// TODO: Fix rounding error
	uint32_t a = (((state->color_rgba >> 24) * alpha) >> 8);
	// TODO: Implement alpha compositing. HA!
	*pixel = rgb | (a << 24);
}

static void draw_scene(SDL_Surface *surface, vector<Line> *lines)
{
	Renderer_State state;
	state.surface = surface;
	Software_Renderer_Context context;
	context.fragment_shader_user_state = &state;
	context.fragment_shader = fragment_shader;

	for (auto it = lines->begin(); it != lines->end(); ++it)
	{
		state.color_rgba = 0xffffffff;
		rasterize_line_diamond_exit(&context, it->x1, it->y1, it->x2, it->y2, 2);
		state.color_rgba = 0xff00ffff;
		rasterize_line_bresenham(&context, it->x1, it->y1, it->x2, it->y2, 1);
		state.color_rgba = 0xffffff00;
		rasterize_line_xiaolin_wu(&context, it->x1, it->y1, it->x2, it->y2, true);
	}

	// Jelly Triangle.
	state.color_rgba = 0xffff00ff;
	fill_triangle(&context, 2.0f, 1.0f, 25.0f, 2.0f, 3.0f, 14.0f);
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("sdl error %s\n", SDL_GetError());
		return 1;
	}

	int img_init_flags = IMG_INIT_JPG | IMG_INIT_PNG;
	if (IMG_Init(img_init_flags) != img_init_flags)
	{
		printf("image error %s\n", IMG_GetError());
		return 1;
	}

	int window_x = 800;
	int window_y = 600;

	int mag = 20;

	SDL_Window *window = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_x, window_y, SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		printf("window failed %s\n", SDL_GetError());
		return 1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, 0, 0);
	if (renderer == NULL)
	{
		printf("renderer failed %s\n", SDL_GetError());
		return 1;
	}

	SDL_Surface *test_surface = SDL_CreateRGBSurface(0, window_x / mag, window_y / mag, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
	if (test_surface == NULL)
	{
		printf("surface failed %s\n", SDL_GetError());
		return 1;
	}

	if (SDL_MUSTLOCK(test_surface))
	{
		printf("WARNING::::::::: for some reason, we must lock the surface.\n");
	}

	vector<Line> lines;
	lines.push_back(Line(2.5f, 2.5f, 4.5f, 6.5f));
	lines.push_back(Line(12.5f, 6.5f, 14.5f, 2.5f));
	lines.push_back(Line(2.5f, 1.5f, 7.5f, 3.5f));
	lines.push_back(Line(1.5f, 2.5f, 3.75f, 8.0f));
	lines.push_back(Line(11.5f, 2.5f, 13.5f, 7.5f));
	lines.push_back(Line(1.5f, 10.5f, 2.5f, 18.5f));
	lines.push_back(Line(23.5f, 10.5f, 22.5f, 18.5f));
	lines.push_back(Line(5.5f, 15.5f, 20.5f, 16.5f));
	lines.push_back(Line(2.5f, 20.5f, 7.5f, 20.5f));
	lines.push_back(Line(25.5f, 10.5f, 25.5f, 15.5f));

	uint32_t before = SDL_GetTicks();
	draw_scene(test_surface, &lines);
	uint32_t elapsed = SDL_GetTicks() - before;
	printf("render time: %u ms\n", elapsed);

	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, test_surface);
	if (texture == NULL)
	{
		printf("texture failed\n", SDL_GetError());
		return 1;
	}

	printf("init ok?\n");

	while (true)
	{
		SDL_Event event;
		if (SDL_WaitEvent(&event) != 1)
		{
			printf("event failed\n");
			break;
		}

		do
		{
			if (event.type == SDL_QUIT)
			{
				printf("quit event\n");
				goto _quit_;
			}
		} while (SDL_PollEvent(&event) == 1);

		SDL_SetRenderDrawColor(renderer, 96, 96, 96, 0);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		
		{
			SDL_Rect checker_rect;
			checker_rect.w = mag;
			checker_rect.h = mag;
			for (checker_rect.y = 0; checker_rect.y < window_y; checker_rect.y += mag)
			{
				for (checker_rect.x = 0; checker_rect.x < window_x; checker_rect.x += mag)
				{
					if ((checker_rect.x + checker_rect.y) % (2 * mag) == 0)
					{
						SDL_RenderFillRect(renderer, &checker_rect);
					}
				}
			}
		}

		//rasterize_line(surface, 2.5f, 2.5f, 4.5f, 6.5f);
		//rasterize_line(surface, 12.5f, 6.5f, 14.5f, 2.5f);

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderCopy(renderer, texture, NULL, NULL);

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		for (auto it = lines.begin(); it != lines.end(); ++it)
		{
			SDL_RenderDrawLine(renderer, it->x1 * mag, it->y1 * mag, it->x2 * mag, it->y2 * mag);
		}

		SDL_RenderPresent(renderer);
	}

_quit_:
	
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(test_surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}
