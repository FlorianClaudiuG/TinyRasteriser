/*---------------------------------------------------------------------
*
* Copyright © 2015  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#pragma once

#include <Windows.h>
#include "Rasterizer.h"
#include "Framebuffer.h"

class AppWindow
{
	private:
		enum ETEST {
			TEST1 = 0,
			TEST2,
			TEST3,
			TEST4,
			TEST5,
			TEST6,
			TEST7,
			TEST8
		};

	private:
		Vector2		mMousePt;			//current mouse location;
		HWND		m_hwnd;				//handle to a window
		HDC			m_hdc;				//handle to a device context
		HGLRC		m_hglrc;			//handle to a gl rendering context

		int			m_width;
		int			m_height;

		Rasterizer	*mRasterizer;		//an instance of rasterizer
		ETEST		mCurrentTest;

		void SetCurrentTestCase(ETEST test);

	protected:

		HGLRC CreateOGLContext (HDC hdc);
		BOOL DestroyOGLContext();
		void InitOGLState();

	public:
		AppWindow();
		AppWindow(HINSTANCE hInstance, int width, int height);
		~AppWindow();

		BOOL InitWindow(HINSTANCE hInstance, int width, int height);

		void		Render();
		void		Resize( int width, int height );
		void		SetVisible( BOOL visible );
		void		DestroyOGLWindow();

		BOOL		MouseLBDown ( int x, int y );
		BOOL		MouseLBUp ( int x, int y );
		BOOL		MouseMove ( int x, int y );
		BOOL		KeyUp(WPARAM key);
};
