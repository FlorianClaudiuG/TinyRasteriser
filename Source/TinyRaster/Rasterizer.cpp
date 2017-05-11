/*---------------------------------------------------------------------
*
* Copyright © 2016  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#include <algorithm>
#include <math.h>

#include "Rasterizer.h"

Rasterizer::Rasterizer(void)
{
	mFramebuffer = NULL;
	mScanlineLUT = NULL;
}

void Rasterizer::ClearScanlineLUT()
{
	Scanline *pScanline = mScanlineLUT;

	for (int y = 0; y < mHeight; y++)
	{
		(pScanline + y)->clear();
		(pScanline + y)->shrink_to_fit();
	}
}

unsigned int Rasterizer::ComputeOutCode(const Vector2 & p, const ClipRect& clipRect)
{
	unsigned int CENTRE = 0x0;
	unsigned int LEFT = 0x1;
	unsigned int RIGHT = 0x1 << 1;
	unsigned int BOTTOM = 0x1 << 2;
	unsigned int TOP = 0x1 << 3;
	unsigned int outcode = CENTRE;
	
	if (p[0] < clipRect.left)
		outcode |= LEFT;
	else if (p[0] >= clipRect.right)
		outcode |= RIGHT;

	if (p[1] < clipRect.bottom)
		outcode |= BOTTOM;
	else if (p[1] >= clipRect.top)
		outcode |= TOP;

	return outcode;
}

bool Rasterizer::ClipLine(const Vertex2d & v1, const Vertex2d & v2, const ClipRect& clipRect, Vector2 & outP1, Vector2 & outP2)
{
	//TODO: EXTRA This is not directly prescribed as an assignment exercise. 
	//However, if you want to create an efficient and robust rasteriser, clipping is a usefull addition.
	//The following code is the starting point of the Cohen-Sutherland clipping algorithm.
	//If you complete its implementation, you can test it by calling prior to calling any DrawLine2D .

	const Vector2 p1 = v1.position;
	const Vector2 p2 = v2.position;
	unsigned int outcode1 = ComputeOutCode(p1, clipRect);
	unsigned int outcode2 = ComputeOutCode(p2, clipRect);

	outP1 = p1;
	outP2 = p2;
	
	bool draw = false;

	return true;
}

void Rasterizer::WriteRGBAToFramebuffer(int x, int y, const Colour4 & colour)
{
	//Added a check to see that the x and y are between bounds.
	if (((0 <= x) && (x <= (mWidth - 1))) && ((0 <= y) && (y <= (mHeight - 1))))
	{
		PixelRGBA *pixel = mFramebuffer->GetBuffer();

		pixel[y*mWidth + x] = colour;
	}
}

Rasterizer::Rasterizer(int width, int height)
{
	//Initialise the rasterizer to its initial state
	mFramebuffer = new Framebuffer(width, height);
	mScanlineLUT = new Scanline[height];
	mWidth = width;
	mHeight = height;

	mBGColour.SetVector(0.0, 0.0, 0.0, 1.0);	//default bg colour is black
	mFGColour.SetVector(1.0, 1.0, 1.0, 1.0);    //default fg colour is white

	mGeometryMode = LINE;
	mFillMode = UNFILLED;
	mBlendMode = NO_BLEND;

	SetClipRectangle(0, mWidth, 0, mHeight);
}

Rasterizer::~Rasterizer()
{
	delete mFramebuffer;
	delete[] mScanlineLUT;
}

void Rasterizer::Clear(const Colour4& colour)
{
	PixelRGBA *pixel = mFramebuffer->GetBuffer();

	SetBGColour(colour);

	int size = mWidth*mHeight;
	
	for(int i = 0; i < size; i++)
	{
		//fill all pixels in the framebuffer with background colour
		*(pixel + i) = mBGColour;
	}
}

void Rasterizer::DrawPoint2D(const Vector2& pt, int size)
{
	int x = pt[0];
	int y = pt[1];
	
	WriteRGBAToFramebuffer(x, y, mFGColour);
}
 
void Rasterizer::DrawLine2D(const Vertex2d & v1, const Vertex2d & v2, int thickness)
{
	Vector2 pt1 = v1.position;
	Vector2 pt2 = v2.position;

	//Boolean to determine if this is the first pixel to be drawn.
	bool first_pass = true;

	//Variable that holds the previous Y. 
	int prevY = -1;

	//Checks if x2 < x1, a case the algorithm can't handle in its current state.
	bool swap_vertices = (pt1[0] > pt2[0]);

	if (swap_vertices)
	{
		pt1 = v2.position;
		pt2 = v1.position;
	}

	//This is how everything should look like for octant 1 (and the one swap vertices fixes, 5)
	int epsilon = 0;
	int x = pt1[0];
	int y = pt1[1];
	int dx = pt2[0] - pt1[0];
	int dy = pt2[1] - pt1[1];
	int ex = pt2[0];

	//Check if the slope is negative. Since we swap the vertices first, we know dx is positive.
	bool negative_slope = (dy < 0);

	//Check if we need to swap x and y.
	bool swap_xy = (abs(dx) < abs(dy));

	bool octant3 = false;
	bool octant7 = (negative_slope && swap_xy);

	//Check if it's in octant 7
	if (octant7)
	{
		//If swap_vertices is also true, then we need to draw a line in octant 3.
		if (swap_vertices)
		{
			octant3 = true;

			pt1 = v1.position;
			pt2 = v2.position;

			x = pt1[1];
			y = -pt1[0];
			ex = pt2[1];
			dx = abs(pt2[1] - pt1[1]);
			dy = abs(pt2[0] - pt1[0]);
		}
		else 
		{
			//If not, we draw in octant 7.
			x = -pt1[1];
			y = pt1[0];
			ex = -pt2[1];
			dx = abs(pt2[1] - pt1[1]);
			dy = abs(pt2[0] - pt1[0]);
		}
	}
	else 
	{
		//Cover any other octant.
		if (negative_slope)
		{
			y = -y;
			dy = -dy;
		}

		if (swap_xy)
		{
			x = pt1[1];
			y = pt1[0];
			ex = pt2[1];
			dx = abs(pt2[1] - pt1[1]);
			dy = abs(pt2[0] - pt1[0]);
		}
	}

	//Set the colours for the 2 points.
	Colour4 start_colour = v1.colour;
	Colour4 end_colour = v2.colour;

	//Calculate the total distance between the 2 points.
	float total_distance = sqrt(pow(pt2[0] - pt1[0], 2) + pow(pt2[1] - pt1[1], 2));

	//Swap the colours if the vertices have been swapped. Octant 3 is a special case, since we swap the vertices back for it.
	if (swap_vertices && !octant3)
	{
		start_colour = v2.colour;
		end_colour = v1.colour;
	}

	//Calculate the differences in colour between our 2 points.
	float delta_r = end_colour[0] - start_colour[0];
	float delta_g = end_colour[1] - start_colour[1];
	float delta_b = end_colour[2] - start_colour[2];

	//The loop that draws the line starts here.
	while (x <= ex)
	{
		Vector2 temp(x, y);

		//Restore the coordinates before the line is drawn.
		if (octant3)
		{
			temp[1] = -temp[1];
			int aux = temp[0];
			temp[0] = temp[1];
			temp[1] = aux;
		}
		else
		{
			if (swap_xy)
			{
				int aux = temp[0];
				temp[0] = temp[1];
				temp[1] = aux;
			}
			if (negative_slope)
			{
				temp[1] = -temp[1];
			}
		}

		//The linear interpolation algorithm.
		if (mFillMode == INTERPOLATED_FILLED)
		{
			//Calculate our current distance drawn from the first point.
			float distance = sqrt(pow(temp[0] - pt1[0], 2) + pow(temp[1] - pt1[1], 2));
			float t = distance / total_distance;

			//Set the colour according to the formula: C[t] = C0 + t * (C1 - C0).
			mFGColour[0] = start_colour[0] + delta_r * t;
			mFGColour[1] = start_colour[1] + delta_g * t;
			mFGColour[2] = start_colour[2] + delta_b * t;
		}
		else
		{
			//If not, set the colour to start_colour.
			SetFGColour(start_colour);
		}

		//Check if we need to handle alpha blending.
		if (mBlendMode == ALPHA_BLEND && mGeometryMode == POLYGON)
		{
			//Get the framebuffer in order to get the colour of the pixel over which we're drawing.
			PixelRGBA *pixel = mFramebuffer->GetBuffer();
			Colour4 old_colour = pixel[abs(int((temp[1] * mWidth) + temp[0]))];

			//Calculate the colour values after alpha blending and modify the foreground colour.
			mFGColour[0] = mFGColour[3] * mFGColour[0] + (1 - mFGColour[3]) * old_colour[0];
			mFGColour[1] = mFGColour[3] * mFGColour[1] + (1 - mFGColour[3]) * old_colour[1];
			mFGColour[2] = mFGColour[3] * mFGColour[2] + (1 - mFGColour[3]) * old_colour[2];
			mFGColour[3] = mFGColour[3] * mFGColour[3] + (1 - mFGColour[3]) * old_colour[3];
		}

		//Draw the point coresponding to the original line to be drawn.
		DrawPoint2D(temp);

		//Check if we need to draw polygon edges and populate the scanline.
		if (mGeometryMode == POLYGON)
		{
			//Check if the y is the same as the previous one. This is so we don't have too many scanline items for adjacent x's.
			if (!(temp[1] == prevY))
			{
				//The way the scanline is currently implemented is rather rough; vertices are pushed too many times.
				ScanlineLUTItem item = { mFGColour, temp[0]};
				int y = abs(temp[1]);
				mScanlineLUT[y].push_back(item);

				//This fixes filling up the circles.
				if (first_pass)
				{
					mScanlineLUT[y].push_back(item);
					first_pass = false;
				}
			}
		}
		prevY = temp[1];

		/*
			Handling thickness in all edge cases.
			The algorithm draws additional lines next to the original to increase its thickness.
			It alternates drawing points above (even counter) and below (odd counter) the original point (assuming the line to be drawn is horizontal; in any other
			case it is rotated accordingly).
		*/
		if (thickness > 1)
		{
			int even = 1;
			int odd = 1;

			//Draw the lines "above"
			for (int i = 0; i < thickness - 1; i += 2)
			{
				if (swap_xy && !negative_slope)
				{
					Vector2 temp(y + even, x);
					DrawPoint2D(temp);
				}
				else if (!swap_xy && negative_slope)
				{
					Vector2 temp(x, -y - even);
					DrawPoint2D(temp);
				}
				else if (swap_xy && negative_slope)
				{
					Vector2 temp(-y - even, x);
					DrawPoint2D(temp);
				}
				else
				{
					Vector2 temp(x, y + even);
					DrawPoint2D(temp);
				}
				even++;
			}

			//Draw the lines "below"
			for (int i = 1; i < thickness - 1; i += 2)
			{
				if (swap_xy && !negative_slope)
				{
					Vector2 temp(y - odd, x);
					DrawPoint2D(temp);
				}
				else if (!swap_xy && negative_slope)
				{
					Vector2 temp(x, -y + odd);
					DrawPoint2D(temp);
				}
				else if (swap_xy && negative_slope)
				{
					Vector2 temp(-y + odd, x);
					DrawPoint2D(temp);
				}
				else
				{
					Vector2 temp(x, y - odd);
					DrawPoint2D(temp);
				}
				odd++;
			}
		}

		//Bresenham's algorithm for the first octant.
		epsilon += dy;

		if ((epsilon << 1) >= dx)
		{
			y++;

			epsilon -= dx;
		}
		x++;
	}
}

void Rasterizer::DrawUnfilledPolygon2D(const Vertex2d * vertices, int count)
{
	//Iterate through the vertices array, drawing a line between each two vertices. (i + 1) % count will unite the first and last vertex.
	for (int i = 0; i < count; i++)
	{
		DrawLine2D(vertices[i], vertices[(i + 1) % count], 1);
	}
}

//Method for the sort function to order the elements in the LUT.
bool WayToSort(const ScanlineLUTItem lhs, const ScanlineLUTItem rhs)
{
	return lhs.pos_x > rhs.pos_x;
}

void Rasterizer::ScanlineFillPolygon2D(const Vertex2d * vertices, int count)
{

	ClearScanlineLUT();

	//Draw the outline of the polygon we want to fill.
	DrawUnfilledPolygon2D(vertices, count);

	//Iterate through the scanline's y's.
	for(int y = 0; y < mHeight; y++)
	{
		int size = mScanlineLUT[y].size();

		if (!mScanlineLUT[y].empty() && size > 1)
		{
			//Sort the elements for each y by their x_pos.
			std::sort(&mScanlineLUT[y][0], &mScanlineLUT[y][size], WayToSort);

			//Iterate through the elements for each y that passes the above check.
			for (int j = 0; j < size; j += 2)
			{
				Vertex2d temp1;
				Vertex2d temp2;
				temp1.position[0] = mScanlineLUT[y][j].pos_x;
				temp1.position[1] = y;
				temp2.position[1] = y;
				temp1.colour = mScanlineLUT[y][j].colour;

				//A very rough fix for an issue with filling the triangle and pentagon. It would not work for a non-convex polygon with 4 vertices.
				//Unites the current point with the last one in the scanline. The code in the else portion should have sufficed.
				if (count <= 5)
				{
					temp2.position[0] = mScanlineLUT[y][size - 1].pos_x;
					temp2.colour = mScanlineLUT[y][size - 1].colour;
				}
				else
				{
					temp2.position[0] = mScanlineLUT[y][j + 1].pos_x;
					temp2.colour = mScanlineLUT[y][j + 1].colour;
				}
				DrawLine2D(temp1, temp2);
			}
		}
	}
}

void Rasterizer::ScanlineInterpolatedFillPolygon2D(const Vertex2d * vertices, int count)
{
	//Clear the scanline LUT, then draw and fill the polygons.
	ClearScanlineLUT();
	DrawUnfilledPolygon2D(vertices, count);
	ScanlineFillPolygon2D(vertices, count);
}

void Rasterizer::DrawCircle2D(const Circle2D & inCircle, bool filled)
{
	//This implementation uses a variant of the parametric ecuation for circles.
	double PI = 3.1415926535897;
	int radius = inCircle.radius;
	Vector2 centre = inCircle.centre;
	
	//Array used for filling up the circles.
	Vertex2d segments[30];
	int i = 0;

	//Decides how big the step is in radians. The denominator is the number of segments used.
	float dt = 2 * PI / 30;
	
	//Set the first point of the circle.
	Vector2 start_pos(centre[0] + radius, centre[1]);
	Vertex2d start;
	start.position = start_pos;

	//Initialise the 2 vertices being drawn. The first vertex is the starting one.
	Vertex2d current;
	current.colour = inCircle.colour;

	//Loop that calculates the points that need to be drawn.
	for (float t = dt; t < 2 * PI; t += dt)
	{
		//Formulae for calculating the next point to be drawn.
		float x = centre[0] + radius * cos(t);
		float y = centre[1] + radius * sin(t);

		current.position[0] = x;
		current.position[1] = y;
		
		//Add elements to the array of vertices.
		segments[i++] = current;
	}

	//Reuse the DrawUnfilledPolygon2D to draw the circles.
	DrawUnfilledPolygon2D(segments, 30);

	//Fill the required circles.
	if (filled == true)
	{
		mGeometryMode = POLYGON;
		mFillMode = SOLID_FILLED;
		ScanlineFillPolygon2D(segments,30);
	}
}

Framebuffer *Rasterizer::GetFrameBuffer() const
{
	return mFramebuffer;
}
