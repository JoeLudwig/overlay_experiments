#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/BitmapSurface.h>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include "openvr.h"


using Awesomium::WSLit;

int main(int _Argc, char ** _Argv )
{
	if ( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
	{
		return -1;
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );

	SDL_Window *window = SDL_CreateWindow( "weboffscreen", 0, 0, 100, 100, SDL_WINDOW_OPENGL );
	if ( !window )
	{
		printf( "%s - window could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError() );
		return -2;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext( window );
	if ( glContext == 0 )
	{
		printf( "%s - OpenGL context could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError() );
		return false;
	}
	SDL_GL_MakeCurrent( window, glContext );

	GLuint iTexture = 0;

	Awesomium::WebConfig config;
	config.log_path = Awesomium::WSLit( "C:\\My Documents\\My Custom Log Path" );
	config.log_level = Awesomium::kLogLevel_Normal;

	Awesomium::WebCore* web_core = Awesomium::WebCore::Initialize( config );

	Awesomium::WebSession* my_session = web_core->CreateWebSession(
		Awesomium::WSLit( "C:\\Session Data Path" ), Awesomium::WebPreferences() );

	// Create a new WebView with a width and height of 500px
	Awesomium::WebView* my_web_view = web_core->CreateWebView( 1024, 1024 );
	Awesomium::WebURL url( Awesomium::WSLit( "http://google.com" ) );
	my_web_view->LoadURL( url );

	bool bQuit = false;
	while ( !bQuit )
	{
		web_core->Update();

		if ( !my_web_view->IsLoading() )
		{
			bQuit = true;
		}
	}

	Sleep( 300 );

	vr::EVRInitError vrInitError;
	if ( !vr::VR_Init( &vrInitError, vr::VRApplication_Overlay ) )
	{
		printf( "VR_Init failed with %s\n", vr::VR_GetVRInitErrorAsSymbol( vrInitError ) );
		return -3;
	}

	vr::VROverlayHandle_t mainHandle, thumbnailHandle;
	vr::EVROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay( "sample.myoverlay", "My Sample Overlay", &mainHandle, &thumbnailHandle );
	if ( overlayError != vr::VROverlayError_None )
	{
		printf( "Unable to create overlay with error %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum( overlayError ) );
		return -4;
	}

	// Get the WebView's rendering Surface. The default Surface is of
	// type 'BitmapSurface', we must cast it before we can use it.
	Awesomium::BitmapSurface* surface = (Awesomium::BitmapSurface*)my_web_view->surface();
	if ( !surface )
	{
		printf( "surface is null\n" );
		return -5;
	}

	glEnable( GL_TEXTURE_2D );
	if ( iTexture == 0 )
	{
		glGenTextures( 1, &iTexture );

		glBindTexture( GL_TEXTURE_2D, iTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, surface->width(), surface->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->buffer() );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

		GLfloat fLargest;
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest );

		glBindTexture( GL_TEXTURE_2D, 0 );
	}
	else
	{
		glBindTexture( GL_TEXTURE_2D, iTexture );
		glBufferData( GL_TEXTURE_2D, surface->row_span() * surface->height(), surface->buffer(), GL_DYNAMIC_COPY );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
	glFlush();
	glFinish();

	// The default overlay size is 1m wide. We want ours to be bigger
	vr::VROverlay()->SetOverlayWidthInMeters( mainHandle, 3.f );

	// OpenGL has its origin in the lower left, so we'll flip the vertical part of the bounds to
	// compensate
	vr::VRTextureBounds_t overlayTextureBounds = { 0, 1.f, 1.f, 0 };
	vr::VROverlay()->SetOverlayTextureBounds( mainHandle, &overlayTextureBounds );

	// set the overlay texture
	vr::Texture_t overlayTexture = { (void*)(uintptr_t)iTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto };
	overlayError = vr::VROverlay()->SetOverlayTexture( mainHandle, &overlayTexture );


	// might as well set the same texture for the thumbnail. This should probably be an icon instead.
	vr::VROverlay()->SetOverlayTexture( thumbnailHandle, &overlayTexture );

	vr::VROverlay()->ShowDashboard( "sample.myoverlay" );
	Sleep( 10000 );


	my_web_view->Destroy();

	exit( 0 ); // this avoids the Awesomium shutdown crash. Everything gets cleaned up when the process exits anyway.

	Awesomium::WebCore::Shutdown();

	return 0;
}

 int __stdcall WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
	return main( 1, &lpCmdLine );
}