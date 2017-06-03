#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/BitmapSurface.h>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include "openvr.h"


using Awesomium::WSLit;

void CopySurfaceToTexture( GLuint & iTexture, Awesomium::BitmapSurface* surface )
{
	if ( iTexture == 0 )
	{
		glGenTextures( 1, &iTexture );

		glBindTexture( GL_TEXTURE_2D, iTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, surface->width(), surface->height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->buffer() );

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
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, surface->width(), surface->height(), GL_BGRA, GL_UNSIGNED_BYTE, surface->buffer() );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
	glFlush();
	glFinish();
}

Awesomium::MouseButton VRMouseButtonToAwesomiumMouseButton( vr::EVRMouseButton eVRButton )
{
	switch ( eVRButton )
	{
	default:
	case vr::VRMouseButton_Left: return Awesomium::kMouseButton_Left;
	case vr::VRMouseButton_Middle: return Awesomium::kMouseButton_Middle;
	case vr::VRMouseButton_Right: return Awesomium::kMouseButton_Right;
	}
}


void CopyWebViewToOverlay( Awesomium::WebView* my_web_view, GLuint & iTexture, vr::VROverlayHandle_t mainHandle )
{
	// Get the WebView's rendering Surface. The default Surface is of
	// type 'BitmapSurface', we must cast it before we can use it.
	Awesomium::BitmapSurface* surface = (Awesomium::BitmapSurface*)my_web_view->surface();
	if ( !surface )
	{
		printf( "surface is null\n" );
	}

	CopySurfaceToTexture( iTexture, surface );

	// set the overlay texture
	vr::Texture_t overlayTexture = { (void*)(uintptr_t)iTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto };
	vr::VROverlay()->SetOverlayTexture( mainHandle, &overlayTexture );
}


int main( int _Argc, char ** _Argv )
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

	const int nWidth = 1024;
	const int nHeight = 1024;

	// Create a new WebView
	Awesomium::WebView* my_web_view = web_core->CreateWebView( nWidth, nHeight );
	Awesomium::WebURL url( Awesomium::WSLit( "http://google.com" ) );
	my_web_view->LoadURL( url );

	while ( my_web_view->IsLoading() )
	{
		web_core->Update();
		Sleep( 0 );
	}

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

	glEnable( GL_TEXTURE_2D );
	CopyWebViewToOverlay( my_web_view, iTexture, mainHandle );

	// might as well set the same texture for the thumbnail. This should probably be an icon instead.
	CopyWebViewToOverlay( my_web_view, iTexture, thumbnailHandle );

	// The default overlay size is 1m wide. We want ours to be bigger
	vr::VROverlay()->SetOverlayWidthInMeters( mainHandle, 2.5f );

	// OpenGL has its origin in the lower left, so we'll flip the vertical part of the bounds to
	// compensate
	vr::VRTextureBounds_t overlayTextureBounds = { 0, 1.f, 1.f, 0 };
	vr::VROverlay()->SetOverlayTextureBounds( mainHandle, &overlayTextureBounds );

	// Tell OpenVR that we want to use the mouse and how many "pixels" we want to scale the mouse by
	vr::VROverlay()->SetOverlayInputMethod( mainHandle, vr::VROverlayInputMethod_Mouse );
	vr::HmdVector2_t mouseScale = { (float)nWidth, (float)nHeight };
	vr::VROverlay()->SetOverlayMouseScale( mainHandle, &mouseScale );

	// Just for debugging, show our new dashboard overlay
	vr::VROverlay()->ShowDashboard( "sample.myoverlay" );

	bool bQuit = false;
	while ( !bQuit )
	{
		// process all events to our overlay
		vr::VREvent_t vrEvent;
		while ( vr::VROverlay()->PollNextOverlayEvent( mainHandle, &vrEvent, sizeof( vrEvent ) ) )
		{
			switch ( vrEvent.eventType )
			{
			case vr::VREvent_Quit:
				bQuit = true;
				break;

			case vr::VREvent_MouseButtonUp:
				my_web_view->InjectMouseUp( VRMouseButtonToAwesomiumMouseButton( (vr::EVRMouseButton)vrEvent.data.mouse.button ) );
				break;

			case vr::VREvent_MouseButtonDown:
				my_web_view->InjectMouseDown( VRMouseButtonToAwesomiumMouseButton( (vr::EVRMouseButton)vrEvent.data.mouse.button ) );
				break;

			case vr::VREvent_MouseMove:
				my_web_view->InjectMouseMove( (int)vrEvent.data.mouse.x, nHeight - (int)vrEvent.data.mouse.y );
				break;

			case vr::VREvent_Scroll:
				my_web_view->InjectMouseWheel( (int)vrEvent.data.scroll.ydelta, (int)vrEvent.data.scroll.xdelta );
				break;
			}
		}

		// let Awesomium do its thing
		web_core->Update();

		// send the new texture to OpenVR
		CopyWebViewToOverlay( my_web_view, iTexture, mainHandle );
	}
	

	my_web_view->Destroy();

	//exit( 0 ); // this avoids the Awesomium shutdown crash. Everything gets cleaned up when the process exits anyway.

	web_core->Shutdown();

	return 0;
}

 int __stdcall WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
	return main( 1, &lpCmdLine );
}