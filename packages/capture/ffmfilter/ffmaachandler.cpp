#include "ffmaachandler.h"
#include "ffmlog.h"

FfmAacHandler::FfmAacHandler()
{	
	m_pFormatContext = NULL;
	m_pCodecContext = NULL;
	m_pCodec = NULL;
	m_pStream = NULL;
	m_pFrame = NULL;
	m_nSampleRate = 44100;
	m_nBitRate = 64000;
	m_nChannels = 2;
	m_nSamples = m_nSampleRate*20/1000;
}

FfmAacHandler::~FfmAacHandler()
{	
	close();
}

void	FfmAacHandler::open() {
	char* filename = "d:\\out.aac";
	int ret = ::avformat_alloc_output_context2(&m_pFormatContext, NULL, NULL, filename);

	m_pCodec = ::avcodec_find_encoder(AV_CODEC_ID_AAC);

	m_pStream = ::avformat_new_stream(m_pFormatContext, m_pCodec);
	m_pCodecContext = m_pStream->codec;

	m_pCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
	m_pCodecContext->sample_fmt = AV_SAMPLE_FMT_S16;
	m_pCodecContext->bit_rate = m_nBitRate;
	m_pCodecContext->sample_rate = m_nSampleRate;
	m_pCodecContext->channels = m_nChannels;
	m_pCodecContext->gop_size = 50;

	ret = ::avcodec_open2(m_pCodecContext, m_pCodec, NULL);
	if( ret != 0 ) {
		FFMLOG("FfmTransform::open, avcodec_open2 failed with ret=", ret);
		return;
	}

	m_nFrameSize = m_pCodecContext->frame_size*4;
	ret = ::avio_open(&m_pFormatContext->pb, filename, AVIO_FLAG_WRITE);
	if( ret != 0 ) {
		FFMLOG("FfmTransform::open, avio_open failed ret=", ret);
		return;
	}
	::avformat_write_header(m_pFormatContext, NULL);

	m_pFrame = avcodec_alloc_frame();
}

int		FfmAacHandler::onData(LONGLONG time, char* src, int inlen, char* dest, int outlen) {
	int got_packet = 0;
	int ret = 0;
	int num = 0;

	avcodec_get_frame_defaults(m_pFrame);
	m_pFrame->nb_samples = m_nSamples;

	av_init_packet(&m_packet);

	ret = ::avcodec_fill_audio_frame(m_pFrame, 2, AV_SAMPLE_FMT_S16, (uint8_t*)src, inlen, 1);
	if( ret != 0 ) {
		FFMLOG("FfmTransform.onData, avcodec_encode_audio2 failed with ret=", ret);
		return 0;
	}
	ret = ::avcodec_encode_audio2(m_pCodecContext, &m_packet, m_pFrame, &got_packet);
	if( ret == 0 && got_packet )
	{
		ret = ::av_write_frame(m_pFormatContext, &m_packet);
		if( ret < 0 ) {
			FFMLOG("FfmTransform.onData, av_write_frame failed with ret=", ret);
		}
		num++;
	} else {
		//	FFMLOG("FfmTransform.onData, avcodec_encode_audio2 failed with ret=", ret);
	}
	FFMLOG("FfmTransform.onData, inlen/frames=", inlen, num);

	return 0;
}

void	FfmAacHandler::close() {
	::av_write_trailer(m_pFormatContext);
	if( m_pCodecContext ) {
		avcodec_close(m_pCodecContext);
		m_pCodecContext = NULL;
	}
	if( m_pFormatContext ) {
		avio_close(m_pFormatContext->pb);
		avformat_free_context(m_pFormatContext);
		m_pFormatContext = NULL;
	}	
	if( m_pFrame ) {
		avcodec_free_frame(&m_pFrame);
		m_pFrame = NULL;
	}
}