#include "vcapaudiocapture.h"
#include "vcapengine.h"
#include "vcapenginefactory.h"
#include "vcapmic.h"
#include "vcapmicfactory.h"
#include "vcapfilefilter.h"
#include "vcapfilter.h"
#include "ffmencoder.h"

VCapAudioCapture::VCapAudioCapture()
{	
	m_pEngine = VCapEngineFactory::getInstance();	
	m_pFfmEncoder = new FfmEncoder();

	m_pFileFilter = NULL;
	m_arrMics = VCapMicFactory::enumMics();
	if( m_arrMics.size() > 0 )
		m_pMic = m_arrMics[0];
}

VCapAudioCapture::~VCapAudioCapture()
{
	if( m_pEngine )
		delete m_pEngine;
	if( m_pFfmEncoder ) 
		delete m_pFfmEncoder;
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

	if( m_wstrFileName.size() > 0 ) {
		m_pFileFilter = new VCapFileFilter(m_pEngine, L"d:\\video.avi");	
	}
	if( !m_pMic )
		return VCAP_ERROR_NO_CAMERA;

	m_pEngine->getGraphBuilder()->AddFilter( m_pMic->filter()->filter(), L"Microphone");
	m_pEngine->getGraphBuilder()->AddFilter( m_pFfmEncoder->filter()->filter(), L"Ffm Encoder");
	if( m_pFileFilter ) {
		m_pEngine->getGraphBuilder()->AddFilter( m_pFileFilter->filter()->filter(), L"File Writer");	
	}
	
	if( m_pFileFilter ) {
		hr = m_pEngine->getCaptureBuilder()->RenderStream(&PIN_CATEGORY_CAPTURE, 
			&MEDIATYPE_Audio, 
			m_pMic->filter()->filter(), 
			m_pFfmEncoder->filter()->filter(),
			m_pFileFilter->filter()->filter());
	} else {
		hr = m_pEngine->getCaptureBuilder()->RenderStream(&PIN_CATEGORY_CAPTURE, 
			&MEDIATYPE_Audio, 
			m_pMic->filter()->filter(), 
			m_pFfmEncoder->filter()->filter(),
			NULL);
	}

	m_pEngine->getMediaControl()->Run();

	return VCAP_ERROR_OK;
}

int		VCapAudioCapture::stopCapture()
{
	m_pEngine->getMediaControl()->Stop();
	return VCAP_ERROR_OK;
}