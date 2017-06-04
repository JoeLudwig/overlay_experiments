#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/BitmapSurface.h>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include "openvr.h"

#include <common/weboverlay.h>

using Awesomium::WSLit;

int main( int _Argc, char ** _Argv )
{
	if ( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
	{
		return -1;
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	//SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );

	SDL_Window *window = SDL_CreateWindow( "weboffscreen", 0, 0, 100, 100, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );
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
	glEnable( GL_TEXTURE_2D );

	Awesomium::WebConfig config;
	config.log_path = Awesomium::WSLit( "C:\\My Documents\\My Custom Log Path" );
	config.log_level = Awesomium::kLogLevel_Normal;

	Awesomium::WebCore* web_core = Awesomium::WebCore::Initialize( config );

	Awesomium::WebSession* my_session = web_core->CreateWebSession(
		Awesomium::WSLit( "C:\\Session Data Path" ), Awesomium::WebPreferences() );

	vr::EVRInitError vrInitError;
	if ( !vr::VR_Init( &vrInitError, vr::VRApplication_Overlay ) )
	{
		printf( "VR_Init failed with %s\n", vr::VR_GetVRInitErrorAsSymbol( vrInitError ) );
		return -3;
	}

	CWebOverlay *pGoogle = new CWebOverlay( web_core, "sample.google", "Google Search", 1024, 512, 2.5f, "http://google.com" );
	CWebOverlay *pBing = new CWebOverlay( web_core, "sample.bing", "Bing Search", 1024, 512, 2.5f, "http://bing.com" );

	// Just for debugging, show our new dashboard overlay
	vr::VROverlay()->ShowDashboard( "sample.google" );

	bool bQuit = false;
	while ( !bQuit )
	{
		vr::VREvent_t vrEvent;
		while ( vr::VRSystem()->PollNextEvent( &vrEvent, sizeof( vrEvent ) ) )
		{
			switch ( vrEvent.eventType )
			{
			case vr::VREvent_Quit:
				bQuit = true;
				break;
			}
		}

		web_core->Update();
		pGoogle->RunFrame();
		pBing->RunFrame();
	}

	delete pGoogle;
	delete pBing;

	web_core->Shutdown();

	return 0;
}

 int __stdcall WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
	return main( 1, &lpCmdLine );
}