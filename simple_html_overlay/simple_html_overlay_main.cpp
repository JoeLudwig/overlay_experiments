#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/BitmapSurface.h>

using Awesomium::WSLit;

int main( _In_ int _Argc, _In_reads_( _Argc ) _Pre_z_ char ** _Argv, _In_z_ char ** _Env )
{
	Awesomium::WebConfig config;
	config.log_path = Awesomium::WSLit( "C:\\My Documents\\My Custom Log Path" );
	config.log_level = Awesomium::kLogLevel_Normal;

	Awesomium::WebCore* web_core = Awesomium::WebCore::Initialize( config );

	Awesomium::WebSession* my_session = web_core->CreateWebSession(
		Awesomium::WSLit( "C:\\Session Data Path" ), Awesomium::WebPreferences() );

	// Create a new WebView with a width and height of 500px
	Awesomium::WebView* my_web_view = web_core->CreateWebView( 500, 500 );
	Awesomium::WebURL url( Awesomium::WSLit( "http://steamvr.com" ) );
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

	// Get the WebView's rendering Surface. The default Surface is of
	// type 'BitmapSurface', we must cast it before we can use it.
	Awesomium::BitmapSurface* surface = (Awesomium::BitmapSurface*)my_web_view->surface();

	// Make sure our surface is not NULL-- it may be NULL if the WebView 
	// process has crashed.
	if ( surface != 0 ) 
	{
		// Save our BitmapSurface to a JPEG image in the current
		// working directory.
		surface->SaveToJPEG( Awesomium::WSLit( "./result.jpg" ) );
	}


	my_web_view->Destroy();

	exit( 0 );

	Awesomium::WebCore::Shutdown();

	return 0;
}