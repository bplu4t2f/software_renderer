#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <vector>

#include "software_renderer.h"
#include "model.h"
//#include "wavefront_obj.h"

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

template<typename T>
struct Buffer
{
	Buffer()
		: data(NULL), size(0)
	{
	}

	Buffer(int size)
		: data(new T[size]), size(size)
	{
		this->zero_entire_buffer();
	}

	Buffer(Buffer &copy) = delete;

	Buffer(Buffer &&move)
	{
		this->data = move.data;
		this->size = move.size;
		move.data = NULL;
		move.size = 0;
	}

	void resize(int size)
	{
		delete this->data;
		this->data = new T[size];
		this->size = size;
		this->zero_entire_buffer();
	}

	void zero_entire_buffer()
	{
		memset(this->data, 0, size * sizeof(T));
	}

	T *data;
	int size;

	~Buffer()
	{
		delete this->data;
		this->size = 0;
	}
};

struct Renderer_State
{
	// Actually we shouldn't call this RGBA. It doesn't matter what the pixel format is,
	// so it's more beneficial to define this so that it matches the surface's pixel
	// format, so that we can optimize the fragment shader as much as possible.
	// Stores one color per vertex.
	uint32_t color_rgba[3];
	SDL_Surface *surface;
	// If this is null, z buffer test is disabled.
	Buffer<float> *z_buffer = NULL;
	// Vertices of the triangle being rasterized; includes z coordinates.
	Vec3f vertices[3];
};

static void fragment_shader(void *user_state, int x, int y, int alpha, float bary1, float bary2, float bary3)
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

	// INTERPOLATION according to the barycentric coordinates !!
	// In case of lines, bary3 is always 0. So we just treat it the same way as with a triangle
	// and everything will be fine.
	// We take the color component of each vertex from "state". Then we weight the vertexes according
	// to the barycentric coordinates.
	// This is equivalent to:
	// C = row_vector(C1, C2, C3) * col_vector(bary1, bary2, bary3).
	// Well mathematically we can also do it all at once by adding more rows to the left operand.
	// That convertes the row_vector to a matrix with one row per color component (4x3 matrix).

	// TODO: Fix rounding error
	int bary[3] = { (int)(bary1 * 255.0f), (int)(bary2 * 255.0f), (int)(bary3 * 255.0f) };

	uint32_t result_rgba = 0;
	// BIGGEST TODO: Yeah quantized fix point division bit shift arithmetic magic wasn't a good idea.
	// When have two vertices, each with a value of 0xff, and you have weights of exactly 0.5, then that
	// will be converted to 0x7f and then added together, which is 0xfe.
	// In other words, this is fast but inaccurate.
	// TODO: This assumes RGBA pixel format. Check surface->format and act accordingly.
	for (int color_component = 0; color_component < 4; ++color_component)
	{
		int shift = color_component * 8;
		static_assert(sizeof(state->color_rgba) / sizeof(state->color_rgba[0]) == 3, "Renderer state must contain 3 vertices.");
		int result = 0;
		for (int vertex = 0; vertex < 3; ++vertex)
		{
			// TODO: Fix rounding error
			result += (((state->color_rgba[vertex] >> shift) & 0xff) * (bary[vertex] + 1)) >> 8;
		}
#ifdef DEBUG
		if (result < 0 || result > 255)
		{
			// bogus math
			SDL_assert(false);
		}
#endif

		// TODO: This assumes RGBA pixel format. Check surface->format and act accordingly.
		// TODO: This generally sucks really bad tbh
		if (color_component == 3)
		{
			// Alpha component -> We need to scale it with "alpha" as well. This is important
			// if a fragment is only partially covered.
			result = (result * (alpha + 1)) >> 8;
		}

		result_rgba |= result << shift;
	}

	if (x == 13 && y == 1)
	{
		int j = 5;
	}

	//
	// Z-buffer test
	//
	if (state->z_buffer != NULL)
	{
		// Calculate the final z coordinate of the pixel being drawn first by interpolation with the
		// barycentric coordinates.
		float result_z = 0;
		static_assert(sizeof(state->vertices) / sizeof(state->vertices[0]) == 3, "Renderer state must contain 3 vertices.");
		result_z = state->vertices[0].z * bary1 + state->vertices[1].z * bary2 + state->vertices[2].z * bary3;
		// 0 is the new INT_MAX 
		// TODO fix this for real though >.>
		result_z -= 10000;
		// The actual test
		int z_buffer_index = y * state->surface->w + x;
		// TODO we can reduce z-fighting issues by adding a small epsilon
		if (result_z >= state->z_buffer->data[z_buffer_index])
		{
			// Discard fragment - z buffer test failed
			return;
		}
		state->z_buffer->data[z_buffer_index] = result_z;
	}

	int offset = y * surface->pitch + x * surface->format->BytesPerPixel;
	uint32_t *pixel = (uint32_t *)((char *)surface->pixels + offset);
	// TODO: Implement alpha compositing. HA!
	*pixel = result_rgba;
}

static void draw_scene(SDL_Surface *surface, vector<Line> *lines)
{
	Renderer_State state;
	state.surface = surface;
	Software_Renderer_Context context;
	context.fragment_shader_user_state = &state;
	context.fragment_shader = fragment_shader;

	// Jelly Triangle.
	state.color_rgba[0] = 0xffff00ff;
	state.color_rgba[1] = 0xffffff00;
	state.color_rgba[2] = 0x8000ffff;
	fill_triangle(&context, 2.0f, 1.0f, 25.0f, 2.0f, 3.0f, 14.0f);

	for (auto it = lines->begin(); it != lines->end(); ++it)
	{
		state.color_rgba[0] = 0xffffffff;
		//state.color_rgba[1] = 0xffffffff;
		//rasterize_line_diamond_exit(&context, it->x1, it->y1, it->x2, it->y2, 2);
		state.color_rgba[1] = 0xff00ffff;
		rasterize_line_bresenham(&context, it->x1, it->y1, it->x2, it->y2, 1);
		state.color_rgba[0] = 0xffff0000;
		state.color_rgba[1] = 0xffffff00;
		rasterize_line_xiaolin_wu(&context, it->x1, it->y1, it->x2, it->y2, false);
		//rasterize_line_xiaolin_wu(&context, it->x1, it->y1 + 1, it->x2, it->y2 + 1, true);
		//rasterize_line_xiaolin_wu(&context, it->x1 - 0.5f, it->y1 + 2, it->x2 + 0.5f, it->y2 + 2, false);
	}
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

	SDL_Surface *big_surface = SDL_CreateRGBSurface(0, window_x, window_y, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
	if (big_surface == NULL)
	{
		printf("big surface failed %s\n", SDL_GetError());
		return 1;
	}


	Model octahedron;
	octahedron.vertices.push_back(Vec3f(0, -1, 0));
	octahedron.vertices.push_back(Vec3f(0, +1, 0));
	octahedron.vertices.push_back(Vec3f(-1, 0, 0));
	octahedron.vertices.push_back(Vec3f(+1, 0, 0));
	octahedron.vertices.push_back(Vec3f(0, 0, -1));
	octahedron.vertices.push_back(Vec3f(0, 0, +1));
	octahedron.faces.push_back(Face(0, 2, 4));
	octahedron.faces.push_back(Face(0, 2, 5));
	octahedron.faces.push_back(Face(0, 3, 4));
	octahedron.faces.push_back(Face(0, 3, 5));
	octahedron.faces.push_back(Face(1, 2, 4));
	octahedron.faces.push_back(Face(1, 2, 5));
	octahedron.faces.push_back(Face(1, 3, 4));
	octahedron.faces.push_back(Face(1, 3, 5));

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
	lines.push_back(Line(25.5f, 7.5f, 32.5f, 14.5f));

	{
		uint32_t before = SDL_GetTicks();
		int iterations = 0;
		uint32_t elapsed;
		while (1)
		{
			draw_scene(test_surface, &lines);
			iterations += 1;
			elapsed = SDL_GetTicks() - before;
			if (elapsed >= 1000 || (iterations > 15 && elapsed > 100))
			{
				break;
			}
		}

		double iterations_per_second = (double)iterations / (double)elapsed * 1000.0;
		printf("line test rendering speed: %lf iterations/s\n", iterations_per_second);
	}

	{
		uint32_t before = SDL_GetTicks();
		int iterations = 0;
		uint32_t elapsed;
		while (1)
		{
			Renderer_State state;
			state.surface = big_surface;
			Buffer<float> z_buffer(state.surface->w * state.surface->h);
			state.z_buffer = &z_buffer;
			Software_Renderer_Context context;
			context.fragment_shader_user_state = &state;
			context.fragment_shader = fragment_shader;

			for (auto &face : octahedron.faces)
			{
				static_assert(sizeof(face.vertices) / sizeof(face.vertices[0]) == 3, "Need 3 vertices per face");

				for (int i = 0; i < 3; ++i)
				{
					int vertex_index = face.vertices[i];
					state.vertices[i] = octahedron.vertices[vertex_index];
					// Some color based on the vertex index:
					state.color_rgba[i] = 0xff000000 |
						(((vertex_index >> 0) & 1) * 255) |
						(((vertex_index >> 1) & 1) * 255) << 8 |
						(((vertex_index >> 2) & 1) * 255) << 16;
				}

				// Projection + Viewport Transform
				// TODO put this is a vertex shader
				// TODO implement vertex shading in the first place :/
				for (int i = 0; i < 3; ++i)
				{
					state.vertices[i].x = state.vertices[i].x * 200.0f + window_x / 2;
					state.vertices[i].y = state.vertices[i].y * 200.0f + window_y / 2;
				}

				
				fill_triangle(&context, state.vertices[0].x, state.vertices[0].y, state.vertices[1].x, state.vertices[1].y, state.vertices[2].x, state.vertices[2].y);
			}

			iterations += 1;
			elapsed = SDL_GetTicks() - before;
			if (elapsed >= 1000 || (iterations > 15 && elapsed > 100))
			{
				break;
			}
		}

		double iterations_per_second = (double)iterations / (double)elapsed * 1000.0;
		printf("octahedron rendering speed: %lf iterations/s\n", iterations_per_second);
	}

	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, test_surface);
	if (texture == NULL)
	{
		printf("texture failed %s\n", SDL_GetError());
		return 1;
	}

	SDL_Texture *big_texture = SDL_CreateTextureFromSurface(renderer, big_surface);
	if (big_texture == NULL)
	{
		printf("big texture failed %s\n", SDL_GetError());
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

		SDL_RenderCopy(renderer, big_texture, NULL, NULL);

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
