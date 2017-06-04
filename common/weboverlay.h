#pragma once

#include <stdint.h>
#include <string>

namespace Awesomium
{
	class WebCore;
	class WebView;
};

namespace vr
{
	typedef uint64_t VROverlayHandle_t;
};

class CWebOverlay
{
public:
	CWebOverlay( Awesomium::WebCore *pWebCore, const std::string & sOverlayKey, const std::string & sOverlayName, uint32_t unWidth, uint32_t unHeight, float fWidthInMeters, const std::string & sUrl, const std::string & sIconFilePath = "" );
	~CWebOverlay();

	void RunFrame();
private:
	void CopyWebViewToOverlay();

	vr::VROverlayHandle_t m_ulMainHandle = 0;
	vr::VROverlayHandle_t m_ulThumbnailHandle = 0;
	uint32_t m_unWidth = 0, m_unHeight = 0;
	uint32_t m_unGLTexture = 0;
	Awesomium::WebView *m_pWebView = nullptr;
};
