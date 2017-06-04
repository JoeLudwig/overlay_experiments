#include "weboverlay.h"

#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/BitmapSurface.h>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include "openvr.h"


// --------------------------------------------------------------------------------------------------
// Purpose: Converts an OpenVR mouse button enum value to an Awesomium mouse button enum value
// --------------------------------------------------------------------------------------------------
static Awesomium::MouseButton VRMouseButtonToAwesomiumMouseButton( vr::EVRMouseButton eVRButton )
{
	switch ( eVRButton )
	{
	default:
	case vr::VRMouseButton_Left: return Awesomium::kMouseButton_Left;
	case vr::VRMouseButton_Middle: return Awesomium::kMouseButton_Middle;
	case vr::VRMouseButton_Right: return Awesomium::kMouseButton_Right;
	}
}


// --------------------------------------------------------------------------------------------------
// Purpose: Constructor. Makes an overlay, a web view, and a GL texture to bridge them
// --------------------------------------------------------------------------------------------------
CWebOverlay::CWebOverlay( Awesomium::WebCore *pWebCore, const std::string & sOverlayKey, const std::string & sOverlayName, uint32_t unWidth, uint32_t unHeight, float fWidthInMeters, const std::string & sUrl )
{
	m_unWidth = unWidth;
	m_unHeight = unHeight;

	// Create a new WebView
	m_pWebView = pWebCore->CreateWebView( (int)unWidth, (int)unHeight );
	Awesomium::WebURL url( Awesomium::ToWebString( sUrl ) );
	m_pWebView->LoadURL( url );

	vr::EVROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay( sOverlayKey.c_str(), sOverlayName.c_str(), &m_ulMainHandle, &m_ulThumbnailHandle);
	if ( overlayError != vr::VROverlayError_None )
	{
		printf( "Unable to create overlay with error %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum( overlayError ) );
		return;
	}

	// The default overlay size is 1m wide. We want ours to be bigger
	vr::VROverlay()->SetOverlayWidthInMeters( m_ulMainHandle, fWidthInMeters );

	// OpenGL has its origin in the lower left, so we'll flip the vertical part of the bounds to
	// compensate
	vr::VRTextureBounds_t overlayTextureBounds = { 0, 1.f, 1.f, 0 };
	vr::VROverlay()->SetOverlayTextureBounds( m_ulMainHandle, &overlayTextureBounds );

	// Tell OpenVR that we want to use the mouse and how many "pixels" we want to scale the mouse by
	vr::VROverlay()->SetOverlayInputMethod( m_ulMainHandle, vr::VROverlayInputMethod_Mouse );
	vr::HmdVector2_t mouseScale = { (float)unWidth, (float)unHeight };
	vr::VROverlay()->SetOverlayMouseScale( m_ulMainHandle, &mouseScale );
}


// --------------------------------------------------------------------------------------------------
// Purpose: Destructor
// --------------------------------------------------------------------------------------------------
CWebOverlay::~CWebOverlay()
{
	vr::VROverlay()->DestroyOverlay( m_ulMainHandle );
	m_pWebView->Destroy();
	if ( m_unGLTexture )
	{
		glDeleteTextures( 1, &m_unGLTexture );
	}
}


// --------------------------------------------------------------------------------------------------
// Purpose: Processes events and updates the texture
// --------------------------------------------------------------------------------------------------
void CWebOverlay::RunFrame()
{
	// process all events to our overlay
	vr::VREvent_t vrEvent;
	while ( vr::VROverlay()->PollNextOverlayEvent( m_ulMainHandle, &vrEvent, sizeof( vrEvent ) ) )
	{
		switch ( vrEvent.eventType )
		{
		case vr::VREvent_MouseButtonUp:
			m_pWebView->InjectMouseUp( VRMouseButtonToAwesomiumMouseButton( (vr::EVRMouseButton)vrEvent.data.mouse.button ) );
			break;

		case vr::VREvent_MouseButtonDown:
			m_pWebView->InjectMouseDown( VRMouseButtonToAwesomiumMouseButton( (vr::EVRMouseButton)vrEvent.data.mouse.button ) );
			break;

		case vr::VREvent_MouseMove:
			m_pWebView->InjectMouseMove( (int)vrEvent.data.mouse.x, m_unHeight - (int)vrEvent.data.mouse.y );
			break;

		case vr::VREvent_Scroll:
			m_pWebView->InjectMouseWheel( (int)vrEvent.data.scroll.ydelta, (int)vrEvent.data.scroll.xdelta );
			break;
		}
	}

	// send the new texture to OpenVR
	CopyWebViewToOverlay();
}


// --------------------------------------------------------------------------------------------------
// Purpose: Updates the GL texture and overlay from the web view
// --------------------------------------------------------------------------------------------------
void CWebOverlay::CopyWebViewToOverlay()
{
	// Get the WebView's rendering Surface. The default Surface is of
	// type 'BitmapSurface', we must cast it before we can use it.
	Awesomium::BitmapSurface* surface = (Awesomium::BitmapSurface*)m_pWebView->surface();
	if ( !surface )
	{
		printf( "surface is null\n" );
		return;
	}

	if ( m_unGLTexture == 0 )
	{
		glGenTextures( 1, &m_unGLTexture );

		glBindTexture( GL_TEXTURE_2D, m_unGLTexture );
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
		glBindTexture( GL_TEXTURE_2D, m_unGLTexture );
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, surface->width(), surface->height(), GL_BGRA, GL_UNSIGNED_BYTE, surface->buffer() );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
	glFlush();
	glFinish();

	// set the overlay texture
	vr::Texture_t overlayTexture = { (void*)(uintptr_t)m_unGLTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto };
	vr::VROverlay()->SetOverlayTexture( m_ulMainHandle, &overlayTexture );
}



