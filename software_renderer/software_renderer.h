#pragma once

#ifdef __INTELLISENSE__
#define SOFTWARE_RENDERER_IMPLEMENTATIONS
#endif

#ifdef SOFTWARE_RENDERER_IMPLEMENTATIONS
#include <SDL.h>
#include <stdint.h>
#include <math.h>

// For std::swap (I'm so sorry...)
#include <utility>
using namespace std;
#else

#endif

// Add __declspec(dllexport) for example, or change it to static
// SEARCH FOR THIS TO FIND ALL API FUNCTIONS
#ifndef SOFTWARE_RENDERER_API
#define SOFTWARE_RENDERER_API extern
#endif

// Use this to define __stdcall for example
#ifndef SOFTWARE_RENDERER_ENTRY
#define SOFTWARE_RENDERER_ENTRY
#endif

// Sorry for the OOP bullshit. This is needed to allow implementors to specify a custom fragment shader
// so that we don't have to deal with clamping, colors, pixel format, or alpha blending.
// Currently the only thing this is used for IS the fragment shader, but there might be more to come.

struct Software_Renderer_Context
{
	void *fragment_shader_user_state;
	void(*fragment_shader)(void *user_state, int x, int y, int alpha);
};

#ifdef SOFTWARE_RENDERER_IMPLEMENTATIONS
static void set_pixel(struct Software_Renderer_Context *context, int x, int y, int alpha, bool transpose)
{
	if (transpose)
	{
		swap(x, y);
	}
	context->fragment_shader(context->fragment_shader_user_state, x, y, alpha);
}

template <typename T>
static int sgn(T val)
{
	return (T(0) < val) - (val < T(0));
}

template <typename T>
static T min(T a, T b)
{
	return a < b ? a : b;
}

template <typename T>
static T max(T a, T b)
{
	return a < b ? b : a;
}
#endif


// ██████╗  █████╗ ██████╗     ██████╗ ██╗ █████╗ ███╗   ███╗ ██████╗ ███╗   ██╗██████╗     ███████╗██╗  ██╗██╗████████╗
// ██╔══██╗██╔══██╗██╔══██╗    ██╔══██╗██║██╔══██╗████╗ ████║██╔═══██╗████╗  ██║██╔══██╗    ██╔════╝╚██╗██╔╝██║╚══██╔══╝
// ██████╔╝███████║██║  ██║    ██║  ██║██║███████║██╔████╔██║██║   ██║██╔██╗ ██║██║  ██║    █████╗   ╚███╔╝ ██║   ██║
// ██╔══██╗██╔══██║██║  ██║    ██║  ██║██║██╔══██║██║╚██╔╝██║██║   ██║██║╚██╗██║██║  ██║    ██╔══╝   ██╔██╗ ██║   ██║
// ██████╔╝██║  ██║██████╔╝    ██████╔╝██║██║  ██║██║ ╚═╝ ██║╚██████╔╝██║ ╚████║██████╔╝    ███████╗██╔╝ ██╗██║   ██║
// ╚═════╝ ╚═╝  ╚═╝╚═════╝     ╚═════╝ ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═════╝     ╚══════╝╚═╝  ╚═╝╚═╝   ╚═╝
//
// ██╗███╗   ███╗██████╗ ██╗     ███████╗███╗   ███╗███████╗███╗   ██╗████████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
// ██║████╗ ████║██╔══██╗██║     ██╔════╝████╗ ████║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
// ██║██╔████╔██║██████╔╝██║     █████╗  ██╔████╔██║█████╗  ██╔██╗ ██║   ██║   ███████║   ██║   ██║██║   ██║██╔██╗ ██║
// ██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝  ██║╚██╔╝██║██╔══╝  ██║╚██╗██║   ██║   ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
// ██║██║ ╚═╝ ██║██║     ███████╗███████╗██║ ╚═╝ ██║███████╗██║ ╚████║   ██║   ██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
// ╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝╚═╝     ╚═╝╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝

// (it's really bad don't use it)


#ifdef SOFTWARE_RENDERER_IMPLEMENTATIONS

struct Point
{
	float x, y;
};

// Given three colinear points p, q, r, the function checks if point q lies on line segment 'pr'
static bool on_segment(Point p, Point q, Point r)
{
	// This is only needed for the BAD diamond exit implementation
	return q.x <= fmax(p.x, r.x) && q.x >= fmin(p.x, r.x) &&
		q.y <= fmax(p.y, r.y) && q.y >= fmin(p.y, r.y);
}

// Finds the orientation of ordered triplet (p, q, r).
// 0: colinear
// 1: positive (counter-clockwise)
// -1: negative (clockwise)
static int orientation(Point p, Point q, Point r)
{
	// This is only needed for the BAD diamond exit implementation
	// See https://www.geeksforgeeks.org/orientation-3-ordered-points/ for details of below formula.
	float val = (q.x - p.x) * (r.y - q.y) - (q.y - p.y) * (r.x - q.x);
	return sgn(val);
}

// Returns true if line segment 'p1q1' and 'p2q2' intersect.
static bool doIntersect(Point p1, Point q1, Point p2, Point q2)
{
	// This is only needed for the BAD diamond exit implementation
	// Find the four orientations needed for general and
	// special cases
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case
	if (o1 != o2 && o3 != o4)
	{
		return true;
	}

	// Special Cases
	// p1, q1 and p2 are colinear and p2 lies on segment p1q1
	if (o1 == 0 && on_segment(p1, p2, q1))
	{
		return true;
	}

	// p1, q1 and p2 are colinear and q2 lies on segment p1q1
	if (o2 == 0 && on_segment(p1, q2, q1))
	{
		return true;
	}

	// p2, q2 and p1 are colinear and p1 lies on segment p2q2
	if (o3 == 0 && on_segment(p2, p1, q2))
	{
		return true;
	}

	// p2, q2 and q1 are colinear and q1 lies on segment p2q2
	if (o4 == 0 && on_segment(p2, q1, q2))
	{
		return true;
	}

	// Doesn't fall in any of the above cases
	return false;
}


// Returns true if the pixel specified by x and y should be rasterized when a line from p1 to q1 is being
// rendererd, according to the diamond exit rule HOWEVER the line doesn't actually need to exit the diamond.
// Any intersection with any of the diamond's side will cause this function to return yes, so if it is the 
// end point that is being checked, you must make sure that the end point is not contained within the diamond
// before calling this (because if it is, the pixel should not be rasterized).
static bool consider_pixel_for_diamond_rasterization(Point p1, Point q1, int x, int y)
{
	// This is only needed for the BAD diamond exit implementation
	// a /\ d 
	//  /  \  
	// /  e \ 
	// \    / 
	//  \  /  
	// b \/ c 
	Point corner_1;
	Point corner_2;
	// Line next to a
	corner_1.x = x;
	corner_1.y = y + 0.5f;
	corner_2.x = x + 0.5f;
	corner_2.y = y;
	if (doIntersect(p1, q1, corner_1, corner_2))
	{
		return true;
	}
	// Line next to b
	corner_2.y = y + 1;
	if (doIntersect(p1, q1, corner_1, corner_2))
	{
		return true;
	}
	// Line next to c
	corner_1.x = x + 1;
	if (doIntersect(p1, q1, corner_1, corner_2))
	{
		return true;
	}
	// Line next to d
	corner_2.y = y;
	if (doIntersect(p1, q1, corner_1, corner_2))
	{
		return true;
	}
	return false;
}
#endif


#ifdef SOFTWARE_RENDERER_IMPLEMENTATIONS
// Returns the x coordinate of the pixel that we need to draw in this row, if any.
// Returns -1 if we shouldn't draw any pixel.
static int consider_y_major_row(Point p1, Point q1, float dx, float dy, int y)
{
	float x1 = p1.x;
	float y1 = p1.y;

	// We don't quite know which pixels we need to consider at all, so we need to find that out first.
	// x-coordinate at the top edge of the pixel row (y) that we are currently considering.
	float x_top = x1 + dx * (((float)y - y1) / dy);
	// x-coordinate at the bottom edge of the pixel row that we are currently considering.
	// Coincides with the top edge of the next pixel row below the current one.
	float x_bottom = x1 + dx * (((float)(y + 1) - y1) / dy);

	int x_top_quantized = (int)x_top;
	int x_bottom_quantized = (int)x_bottom;

	// We must consider pixels (x_top_quantized, y) and (x_bottom_quantized, y), however
	// x_top_quantized and x_bottom_quantized might be the same, in which case we can skip one pixel.
	// This happens if the line does not cross the vertical boundary between to pixels that are next
	// to each other in this current pixel row (y).
	bool draw = consider_pixel_for_diamond_rasterization(p1, q1, x_top_quantized, y);
	if (draw)
	{
		return x_top_quantized;
	}
	else if (!draw && x_top_quantized != x_bottom_quantized)
	{
		draw = consider_pixel_for_diamond_rasterization(p1, q1, x_bottom_quantized, y);
		if (draw)
		{
			return x_bottom_quantized;
		}
	}

	return -1;
}
#endif

// Rasterizes a 1-width line from start point (x1, y1) to end point (x2, y2) according to the diamond exit rule.
// Integer coordinates refer to pixel boundaries (in other words, pixel centers are always some integer number + 0.5f).
// This produced an aliased line. As such, width can only be an integer.
// TODO: Disambiguate corner case when the line passes exactly through the corners of 2 adjacent (wrt the minor direction) diamonds.
SOFTWARE_RENDERER_API void SOFTWARE_RENDERER_ENTRY rasterize_line_diamond_exit(Software_Renderer_Context *context, float x1, float y1, float x2, float y2, int width)
#ifdef SOFTWARE_RENDERER_IMPLEMENTATIONS
{
	// DONT BOTHER WITH THIS, ITS BAD.

	int start_pixel_x = (int)x1;
	int start_pixel_y = (int)y1;
	int end_pixel_x = (int)x2;
	int end_pixel_y = (int)y2;

	int dx_quantized = end_pixel_x - start_pixel_x;
	int dy_quantized = end_pixel_y - start_pixel_y;

	bool y_major = abs(dy_quantized) >= abs(dx_quantized);
	if (!y_major)
	{
		{
			float tmp;
			tmp = x1;
			x1 = y1;
			y1 = tmp;
			tmp = x2;
			x2 = y2;
			y2 = tmp;
		}
		{
			int tmp;
			tmp = start_pixel_x;
			start_pixel_x = start_pixel_y;
			start_pixel_y = tmp;
			tmp = end_pixel_x;
			end_pixel_x = end_pixel_y;
			end_pixel_y = tmp;
		}
	}

	float dx = x2 - x1;
	float dy = y2 - y1;

	// Offset the minor direction so that we can draw the width directly.
	// This particular strategy came to my mind when I was trying to sleep yesterday:
	if (width > 1)
	{
		float offset = (width - 1) / 2.0f;
		x1 -= offset;
		x2 -= offset;
	}

	// For the doIntersect function.
	Point p1;
	p1.x = x1;
	p1.y = y1;
	Point q1;
	q1.x = x2;
	q1.y = y2;

	// We can assume that the line is y-major.
	// However, no assumptions about the direction can be made.
	// Both dx and dy may be positive or negative.

	int y_step = dy > 0 ? 1 : -1;
	// a /\ d 
	//  /  \  
	// /  e \ 
	// \    / 
	//  \  /  
	// b \/ c 

	// This loop counts for everything except the end point, which is treated separately.

	// For all points excluding the end point, a pixel is rasterized if the line
	// being drawn intersects any of the 4 lines that make up the pixel inscribed diamond.
	// The reason is that if it crosses any line, that means the line either just left the
	// diamond, or it entered the diamond and must eventually exit in order to get to the
	// end point.
	int y;
	for (y = start_pixel_y; y != end_pixel_y; y += y_step)
	{
		int draw_x = consider_y_major_row(p1, q1, dx, dy, y);
		if (draw_x >= 0)
		{
			for (int i = 0; i < width; ++i)
			{
				set_pixel(context, draw_x + i, y, 255, !y_major);
			}
		}
	}

	// The end point is only considered if it is not inside the diamond of the end pixel.
	float end_pixel_center_x = end_pixel_x + 0.5f;
	float end_pixel_center_y = end_pixel_y + 0.5f;
	if (fabs(x2 - end_pixel_center_x) + fabs(y2 - end_pixel_center_y) > 0.5f)
	{
		int draw_x = consider_y_major_row(p1, q1, dx, dy, y);
		if (draw_x >= 0)
		{
			for (int i = 0; i < width; ++i)
			{
				set_pixel(context, draw_x + i, y, 255, !y_major);
			}
		}
	}
}
#else
	;
#endif




// ██╗  ██╗██╗ █████╗  ██████╗ ██╗     ██╗███╗   ██╗    ██╗    ██╗██╗   ██╗
// ╚██╗██╔╝██║██╔══██╗██╔═══██╗██║     ██║████╗  ██║    ██║    ██║██║   ██║
//  ╚███╔╝ ██║███████║██║   ██║██║     ██║██╔██╗ ██║    ██║ █╗ ██║██║   ██║
//  ██╔██╗ ██║██╔══██║██║   ██║██║     ██║██║╚██╗██║    ██║███╗██║██║   ██║
// ██╔╝ ██╗██║██║  ██║╚██████╔╝███████╗██║██║ ╚████║    ╚███╔███╔╝╚██████╔╝
// ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝     ╚══╝╚══╝  ╚═════╝ 


// Xiaolin Wu's anti-aliased line rasterization algorithm.
// This one is actually simpler than the other ones because endpoints don't need to be treated specially.
// However, it does not support width.
// extend_endpoints extends both endpoints by 0.5 in the major direction. This makes it so that if you
// draw from exact pixel center to exact pixel center, the endpoints are rasterized at full opacity as opposed
// to half opacity. This makes rendering certain primitives easier (non-standard extension).
SOFTWARE_RENDERER_API void SOFTWARE_RENDERER_ENTRY rasterize_line_xiaolin_wu(Software_Renderer_Context *context, float x1, float y1, float x2, float y2, bool extend_endpoints)
#ifdef SOFTWARE_RENDERER_IMPLEMENTATIONS
{
	float dx = x2 - x1;
	float dy = y2 - y1;

	bool y_major = fabs(dy) > fabs(dx);
	if (!y_major)
	{
		// Make it y-major then
		swap(x1, y1);
		swap(x2, y2);
		swap(dx, dy);
	}

	int y_step = dy > 0 ? 1 : -1;
	// ignore start and end point for now
	float x_per_y_step = dx / dy * y_step;

	if (extend_endpoints)
	{
		y1 -= 0.5f * y_step;
		y2 += 0.5f * y_step;
		x1 -= 0.5f * x_per_y_step;
		x2 += 0.5f * x_per_y_step;
		// We don't need to recalculate dx and dy because their absolute values are irrelevant at this point.
		// We don't need to recalculate x_per_y_step because that shouldn't change (unless this is bugged).s
	}

	int start_point_y = (int)y1;
	int end_point_y = (int)y2;

	float start_point_y_distance_to_outgoing_edge = y1 - start_point_y;
	if (dy > 0)
	{
		start_point_y_distance_to_outgoing_edge = 1 - start_point_y_distance_to_outgoing_edge;
	}
	float end_point_y_distance_to_outgoing_edge = y2 - end_point_y;
	if (dy < 0)
	{
		end_point_y_distance_to_outgoing_edge = 1 - end_point_y_distance_to_outgoing_edge;
	}
	// We basically offset the entire line 0.5 to the left, that makes everything easier.
	// Because now we know we need to consider the (int)x pixel, and the (int)x + 1 pixel.
	// Before the transformation, we needed to consider the fractional part of x, and decide depending on
	// whether it's > 0.5 or not if we need to also consider the pixel to the right or left to it.
	float interp_x_at_center_y = x_per_y_step * (start_point_y_distance_to_outgoing_edge - 0.5f) + x1 - 0.5f;
	// Because we need to include the end point in the iterations
	int iteration_end = end_point_y + y_step;
	for (int y = start_point_y; y != iteration_end; y += y_step)
	{
		// This is the x coordinate of the left pixel of a pair that is next to each other and is affected
		// by the line.
		int x = (int)interp_x_at_center_y;
		float fractional_part = interp_x_at_center_y - x;
		// If the fractional part is very low, that means the line is going pretty much dead center through this pixel,
		// leaving little for the pixel to the right.
		// As the fractional part increases, this pixel loses intensity and the pixel to the right gains intensity.
		float alpha_l = 1.0f - fractional_part;
		float alpha_r = fractional_part;
		// Unfortunate branching for end points, but this code is easier to maintain and understand.
		// End points may have partial alpha based on their total coverage in the major direction.
		if (y == start_point_y)
		{
			alpha_l *= start_point_y_distance_to_outgoing_edge;
			alpha_r *= start_point_y_distance_to_outgoing_edge;
		}
		else if (y == end_point_y)
		{
			alpha_l *= end_point_y_distance_to_outgoing_edge;
			alpha_r *= end_point_y_distance_to_outgoing_edge;
		}
		set_pixel(context, x + 0, y, alpha_l * 255, !y_major);
		set_pixel(context, x + 1, y, alpha_r * 255, !y_major);
		interp_x_at_center_y += x_per_y_step;
	}
}
#else
;
#endif


//  ██████╗ ██████╗ ███████╗███████╗███████╗███╗   ██╗██╗  ██╗ █████╗ ███╗   ███╗    ██╗     ██╗███╗   ██╗███████╗
//  ██╔══██╗██╔══██╗██╔════╝██╔════╝██╔════╝████╗  ██║██║  ██║██╔══██╗████╗ ████║    ██║     ██║████╗  ██║██╔════╝
//  ██████╔╝██████╔╝█████╗  ███████╗█████╗  ██╔██╗ ██║███████║███████║██╔████╔██║    ██║     ██║██╔██╗ ██║█████╗  
//  ██╔══██╗██╔══██╗██╔══╝  ╚════██║██╔══╝  ██║╚██╗██║██╔══██║██╔══██║██║╚██╔╝██║    ██║     ██║██║╚██╗██║██╔══╝  
//  ██████╔╝██║  ██║███████╗███████║███████╗██║ ╚████║██║  ██║██║  ██║██║ ╚═╝ ██║    ███████╗██║██║ ╚████║███████╗
//  ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝    ╚══════╝╚═╝╚═╝  ╚═══╝╚══════╝


// Rasterizes a line using Bresenham's algorithm.
// Both end points are treated correctly according to the diamond exit rule.
// In fact, as far as I can tell, this is strictly superior to the other diamond rule algorithm, however may produce different
// results in ambiguous cases (i.e. when the line crosses exactly through the corner of 2 adjacent inscribed diamonds).
// TODO: Disambiguate corner case when the line passes exactly through the corners of 2 adjacent (wrt the minor direction) diamonds.
// TODO: Prove mathematically that this actually implements the diamond exit strategy.
SOFTWARE_RENDERER_API void SOFTWARE_RENDERER_ENTRY rasterize_line_bresenham(Software_Renderer_Context *context, float x1, float y1, float x2, float y2, int width)
#ifdef SOFTWARE_RENDERER_IMPLEMENTATIONS
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	
	bool y_major = fabs(dy) > fabs(dx);
	if (!y_major)
	{
		// Make it y-major then
		swap(x1, y1);
		swap(x2, y2);
		swap(dx, dy);
	}

	// Offset the minor direction so that we can draw the width directly.
	if (width > 1)
	{
		float offset = (width - 1) / 2.0f;
		x1 -= offset;
		x2 -= offset;
	}

	// Start point is only drawn if it's in its diamond, or above (if line is going top to bottom).
	int start_pixel_x = (int)x1;
	int start_pixel_y = (int)y1;
	// Used for the main loop to calculate the first x-coordinate
	float start_point_y_distance_to_outgoing_edge;
	{
		float fractional_y_position = y1 - start_pixel_y;
		bool draw = false;
		if (dy >= 0)
		{
			// Line going down, must be above
			draw = fractional_y_position <= 0.5f;
			// y-Distance to outgoing edge (= bottom edge) from start point
			start_point_y_distance_to_outgoing_edge = 1 - fractional_y_position;
		}
		else
		{
			// Line going up, must be below
			draw = fractional_y_position >= 0.5f;
			// y-Distance to outgoing edge (= top edge) from start point
			start_point_y_distance_to_outgoing_edge = fractional_y_position;
		}
		if (!draw)
		{
			// Check if it's inside the diamond
			float center_x = start_pixel_x + 0.5f;
			float center_y = start_pixel_y + 0.5f;
			draw = fabs(x1 - center_x) + fabs(y1 - center_y) <= 0.5f;
		}
		if (draw)
		{
			for (int i = 0; i < width; ++i)
			{
				set_pixel(context, start_pixel_x + i, start_pixel_y, 255, !y_major);
			}
		}
	}

	// End point is only drawn if it's below (if line is going top to bottom), but not in its diamond.
	int end_pixel_x = (int)x2;
	int end_pixel_y = (int)y2;
	{
		float fractional_y_position = y2 - end_pixel_y;
		bool draw = false;
		if (dy >= 0)
		{
			// Line going down, must be below
			draw = fractional_y_position >= 0.5f;
		}
		else
		{
			// Line going up, must be above
			draw = fractional_y_position <= 0.5f;
		}
		if (draw)
		{
			// Check if it's inside the diamond
			float center_x = end_pixel_x + 0.5f;
			float center_y = end_pixel_y + 0.5f;
			draw = fabs(x2 - center_x) + fabs(y2 - center_y) > 0.5f;
		}
		if (draw)
		{
			for (int i = 0; i < width; ++i)
			{
				set_pixel(context, end_pixel_x + i, end_pixel_y, 255, !y_major);
			}
		}
	}

	// Between the nightmare...
	int y_step = dy > 0 ? 1 : -1;
	float x_per_y = dx / dy;
	// This is now actually x_per_y_step, which is actually what we need because we're stepping in y_step direction,
	// not in positive direction.
	// The same applies to "start_point_y_distance_to_outgoing_edge + 0.5f" - we're stepping that much in y_step direction.
	x_per_y *= y_step;
	// Prepare x-coordinate of the first point of the main loop below:
	float interp_x_position_at_vertical_center = x_per_y * (start_point_y_distance_to_outgoing_edge + 0.5f) + x1;
	for (int y = start_pixel_y + y_step; y != end_pixel_y; y += y_step)
	{
		int x = (int)interp_x_position_at_vertical_center;
		for (int i = 0; i < width; ++i)
		{
			set_pixel(context, x + i, y, 255, !y_major);
		}
		interp_x_position_at_vertical_center += x_per_y;
	}
}
#else
;
#endif




//  ███████╗██╗██╗     ██╗         ████████╗██████╗ ██╗ █████╗ ███╗   ██╗ ██████╗ ██╗     ███████╗
//  ██╔════╝██║██║     ██║         ╚══██╔══╝██╔══██╗██║██╔══██╗████╗  ██║██╔════╝ ██║     ██╔════╝
//  █████╗  ██║██║     ██║            ██║   ██████╔╝██║███████║██╔██╗ ██║██║  ███╗██║     █████╗  
//  ██╔══╝  ██║██║     ██║            ██║   ██╔══██╗██║██╔══██║██║╚██╗██║██║   ██║██║     ██╔══╝  
//  ██║     ██║███████╗███████╗       ██║   ██║  ██║██║██║  ██║██║ ╚████║╚██████╔╝███████╗███████╗
//  ╚═╝     ╚═╝╚══════╝╚══════╝       ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚══════╝


SOFTWARE_RENDERER_API void SOFTWARE_RENDERER_ENTRY fill_triangle(
	Software_Renderer_Context *context,
	float x1, float y1, float x2, float y2, float x3, float y3
)
#ifdef SOFTWARE_RENDERER_IMPLEMENTATIONS
{
	// So apparently we need to convert each rasterized pixel's coordinates
	// into barycentric coordinates sooner or later anyway because we need
	// a way to interpolate vertex colors and texture coordinates.

	// The vertex colors weights of a fragment are equal to the barycentric
	// coordinates of a fragment, which is nice because then the fragment
	// shader only needs to add weighted colors.
	
	// The texture coordinates are slightly more difficult and it seems a bit
	// strange. First we convert from xy coordinates to barycentric coordinates
	// in the drawn vertex triangle, and then we convert the barycentric coordinates
	// back to st coordinates in the texture triangle. Seems inefficient but
	// let's just go with it.

	// First we need to find all fragments that should be rasterized, i.e. all
	// fragments whose sample point(s) lie inside of the triangle defined by
	// the given vertices, OR lie on the edge of the triangle, but only if it
	// is either the top or left edge. Otherwise, two triangles sharing an edge
	// would produce duplicate fragments, which we must avoid because that could
	// really screw over alpha blending.

	// Currently we're just considering every pixel in the triangle's bounding
	// box and check if its barycentric coordinates are in the range [0 .. 1].
	// TODO see if it's worth optimizing this. The bounding box also snaps to
	// integer bounds because we're dealing with pixels after all.

	int bounding_box_min_x = (int)floorf(min(x1, min(x2, x3)));
	int bounding_box_max_x = (int)ceilf((x1, max(x2, x3)));
	int bounding_box_min_y = (int)floorf(min(y1, min(y2, y3)));
	int bounding_box_max_y = (int)ceilf(max(y1, max(y2, y3)));

	for (int y = bounding_box_min_y; y <= bounding_box_max_y; ++y)
	{
		for (int x = bounding_box_min_x; x <= bounding_box_max_x; ++x)
		{
			float center_x = (float)x + 0.5f;
			float center_y = (float)y + 0.5f;

			// Dis straight from wikipedia
			float denom_rec = 1.0f / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
			float dx_base = (center_x - x3);
			float dy_base = (center_y - y3);
			float bary1 = ((y2 - y3) * dx_base + (x3 - x2) * dy_base) * denom_rec;
			float bary2 = ((y3 - y1) * dx_base + (x1 - x3) * dy_base) * denom_rec;
			float bary3 = 1.0f - bary1 - bary2;

			// Great !!

			// Point is inside if all bary coordinates are in the range 0 .. 1. If one
			// coordinate is exactly 0 or 1, then it's either exactly on an edge or
			// on a corner.
			
			// TODO: Disambiguate !! Make sure that triangles 2 which share an edge
			// do not produce the same fragments twice. For example we could make it so
			// that if a sample is exactly on the edge of a triangle, it only produces the fragment
			// if that edge is the top or left edge (whatever that means...)

			if (bary1 < 0.0f || bary1 > 1.0f || bary2 < 0.0f || bary2 > 1.0f || bary3 < 0.0f || bary3 > 1.0f)
			{
				// Outside
				continue;
			}

			// TODO weighting Ehhh??
			context->fragment_shader(context->fragment_shader_user_state, x, y, 255);
		}
	}
}
#else
;
#endif
