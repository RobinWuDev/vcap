#include "vcapaudiocapture.h"
#include "vcapengine.h"
#include "vcapenginefactory.h"
#include "vcapaudioencoder.h"
#include "vcapaudioencoderfactory.h"
#include "vcapmic.h"
#include "vcapmicfactory.h"
#include "vcapfilefilter.h"
#include "vcapfilter.h"
#include "vcapspxencfilter.h"


VCapAudioCapture::VCapAudioCapture()
{	
	m_pEngine = VCapEngineFactory::getInstance();	
	m_pSpxFilter = new VCapSpxEncFilter();

	m_pFileFilter = NULL;
	m_arrMics = VCapMicFactory::enumMics();
	if( m_arrMics.size() > 0 )
		m_pMic = m_arrMics[0];
}

VCapAudioCapture::~VCapAudioCapture()
{
	if( m_pEngine )
		delete m_pEngine;
}

void	VCapAudioCapture::setFileName(const wchar_t* filename)
{
	m_wstrFileName.assign(filename);
}

void	VCapAudioCapture::setAudioFormat(int format)
{	
}

int		VCapAudioCapture::startCapture()
{
	HRESULT hr = S_OK;

	m_pFileFilter = new VCapFileFilter(m_pEngine, L"d:\\video.avi");	
	if( !m_pMic )
		return VCAP_ERROR_NO_CAMERA;
	if( !m_pSpxFilter )
		return VCAP_ERROR_NO_SPEEX_FILTER;

	m_pEngine->getGraphBuilder()->AddFilter( m_pMic->filter()->filter(), L"Micphone");
	m_pEngine->getGraphBuilder()->AddFilter( m_pSpxFilter->filter()->filter(), L"Speex Encoder");
	m_pEngine->getGraphBuilder()->AddFilter( m_pFileFilter->filter()->filter(), L"File Writer");	
	
	hr = m_pEngine->getCaptureBuilder()->RenderStream(&PIN_CATEGORY_CAPTURE, 
		&MEDIATYPE_Audio, 
		m_pMic->filter()->filter(), 
		m_pSpxFilter->filter()->filter(), 						
		m_pFileFilter->filter()->filter());

	m_pEngine->getMediaControl()->Run();

	return VCAP_ERROR_OK;
}

int		VCapAudioCapture::stopCapture()
{
	m_pEngine->getMediaControl()->Stop();
	return VCAP_ERROR_OK;
}