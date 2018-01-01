#pragma once

#ifdef __INTELLISENSE__
#define WAVEFRONT_OBJ_IMPLEMENTATIONS
#endif

#ifdef WAVEFRONT_OBJ_IMPLEMENTATIONS
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#else

#endif
#include <vector>

struct wavefront_obj_vec3f
{
	float x, y, z;
};
struct wavefront_obj_vec2f
{
	float s, t;
};
struct wavefront_obj_face
{
	int vertices[3];
	int texture_coords[3];
	int normals[3];
};

// Add __declspec(dllexport) for example, or change it to static
// SEARCH FOR THIS TO FIND ALL API FUNCTIONS
#ifndef WAVEFRONT_OBJ_API
#define WAVEFRONT_OBJ_API extern
#endif

// Use this to define __stdcall for example
#ifndef WAVEFRONT_OBJ_ENTRY
#define WAVEFRONT_OBJ_ENTRY
#endif


#ifdef WAVEFRONT_OBJ_IMPLEMENTATIONS
// sad
template <typename T>
static T min(T a, T b)
{
	return a < b ? a : b;
}
#endif



WAVEFRONT_OBJ_API int WAVEFRONT_OBJ_ENTRY wavefront_obj_read(
	const char *data,
	std::vector<wavefront_obj_vec3f> *vertices,
	std::vector<wavefront_obj_vec2f> *texture_coords,
	std::vector<wavefront_obj_vec3f> *normals,
	std::vector<wavefront_obj_face> *faces
)
#ifdef WAVEFRONT_OBJ_IMPLEMENTATIONS
{
	const char *the_start = data;

	// Parsing in C is suffering
	while (1)
	{
		if (data == NULL)
		{
			break;
		}

		while (isspace(*data))
		{
			data += 1;
		}

		if (*data == '\0')
		{
			break;
		}

		const char *line_end = strchr(data, '\n');
		int line_length = line_end != NULL ? line_end - data : INT_MAX;

		if (!strncmp(data, "v ", min(line_length, 2)))
		{
			// Vertex
			data += 2;
			wavefront_obj_vec3f vec;
			if (sscanf(data, " %f %f %f", &vec.x, &vec.y, &vec.z) == 3)
			{
				vertices->push_back(std::move(vec));
			}
			else
			{
				return data - the_start;
			}
		}
		else if (!strncmp(data, "vt ", min(line_length, 3)))
		{
			// Texture coords
			data += 3;
			wavefront_obj_vec2f vec;
			if (sscanf(data, " %f %f", &vec.s, &vec.t) == 2)
			{
				texture_coords->push_back(std::move(vec));
			}
			else
			{
				return data - the_start;
			}
		}
		else if (!strncmp(data, "vn ", min(line_length, 3)))
		{
			// Normal vector
			data += 3;
			wavefront_obj_vec3f vec;
			if (sscanf(data, " %f %f %f", &vec.x, &vec.y, &vec.z) == 3)
			{
				normals->push_back(std::move(vec));
			}
			else
			{
				return data - the_start;
			}
		}
		else if (!strncmp(data, "f ", min(line_length, 2)))
		{
			// Face yo
			data += 2;
			wavefront_obj_face face;
			int stupid;
			// TODO handle other formats
			// TODO handle quads (?)
			if (sscanf(data, " %d/%d/%d %d/%d/%d %d/%d/%d",
				
				&face.vertices[0],
				&face.texture_coords[0],
				&face.normals[0],
				
				&face.vertices[1],
				&face.texture_coords[1],
				&face.normals[1],
				
				&face.vertices[2],
				&face.texture_coords[2],
				&face.normals[2]
			) == (9))
			{
				faces->push_back(std::move(face));
			}
			else
			{
				return data - the_start;
			}
		}

		// Wow even though parsing in C sucks, this turned out extraordinarily terrible.
		// TODO make this whole function good

		data = line_end;
	}

	return -1;
}
#else
;
#endif

