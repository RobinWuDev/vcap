#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include "vcapconfig.h"

class Engine;
class Camera;
class VMRRender;
class FfmEncoder;
class FileFilter;
class VideoCapture : public IVCapVideoCapture
{
public:
	VideoCapture();
	~VideoCapture();

public:
	virtual void	setFileName(const wchar_t* filename);
	virtual void	setServer(char* ip, int port, char* app, char* stream);
	virtual int		startCapture(int hWnd);
	virtual int		stopCapture();
	virtual void	paint();

private:
	Engine*		m_pEngine;
	std::wstring	m_wstrFileName;	
	Camera*		m_pCamera;
	FfmEncoder*	m_pEncoder;		
	FileFilter*	m_pFileFilter;
	VMRRender*	m_pVMRRender;

	std::vector<Camera*>	m_arrCameras;
	std::string	m_strIp;
	int			m_nPort;
	std::string m_strApp;
	std::string m_strStream;
};


#endif