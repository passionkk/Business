#ifndef _MP3Muxer_H_
#define _MP3Muxer_H_

#include <stdint.h>
#include <vector>
#include <deque>
#include <Poco/Thread.h>

#include "../Common/RecordBase.h"

class Mp3DataIface
{
public:
	virtual ~Mp3DataIface() {}

    virtual void DataHandle(
        void *pUserData,
        const uint8_t *pData,
        uint32_t iLen,
        uint64_t iPts /*ms*//*, uint64_t iDts*/) = 0;

};

class Mp3Muxer : public Poco::Runnable
{
public:
    typedef void(*DataHandle)(
        void *pUserData,
        const uint8_t *pData,
        uint32_t iLen,
        uint64_t iPts /*ms*//*,uint64_t iDts*/);

    struct AVPacketData
    {
        uint8_t * pData;
        int iDataSize;
        int iOffset;
        int iProbeOffset;
        uint64_t iTimestamp; // us
		uint64_t iTimestampDTS;	// us 
        FormatType eFormatType;
    };

    Mp3Muxer();

    ~Mp3Muxer();

    // ��ӹ�ע������ݵĺ���ָ��
    void Subscribe(DataHandle dataHandle, void *pUserData);

    // ȡ����ע������ݵĺ���ָ��
    void UnSubscribe(DataHandle dataHandle);

    // ��ӹ�ע������ݵ�ʵ������
	void Subscribe(Mp3DataIface *pDataInstance, void *pUserData);

    // ȡ����ע������ݵ�ʵ������
	void UnSubscribe(Mp3DataIface *pDataInstance);

    // ��������Ƶ����
    bool ImportAVPacket(
        MediaType eMediaType,
        FormatType eFormatType,
        uint8_t *pData,
        int iDataSize,
        uint64_t iTimestamp /*us*/,
		uint64_t iTimestampDTS /*us*/);	//��B֡������贫��DTS

    // ��ʼ
    bool Start(
        FormatType eAFormatType,
        FormatType eVFormatType,
        int iAudioReadSize = 32768,
        int iVideoReadSize = 32768);

    // ֹͣ
    void Stop();

    // ��̨�߳����в��л�������
    bool IsBusy();

    // ��̨�߳��Ƿ�����
    bool IsRunning() const;

public:
    // �����ڲ��ص�����ʹ��
    int ReadAVData(uint8_t *pBuf, int iBufSize, MediaType eType);

    int OnDataHandle(uint8_t *pBuf, int iBufSize);

protected:
    void run();

private:
    bool CheckAudioType(FormatType eAType);

    bool CheckVideoType(FormatType eVType);

    void Destroy();

    AVInputFormat *GetAVInputFormat(FormatType eType);

    void ReadAVFrame(
        AVPacket *pkt,
        uint64_t iReferenceTimeVal,
        uint64_t iCurPts,
        std::deque<AVPacketData *> *pAVPacketDataDeque);

    void SetAVPacketFlag(AVPacket *pkt, MediaType eType);

    const char * GetNal(
        const char *sData,
        int iDataSize,
        int *iRelativePos,
        int *iNalSize);

private:
    Poco::Thread m_RoutineThread;

    std::vector<std::pair<DataHandle, void*> > m_DataHandles;
	std::vector<std::pair<Mp3DataIface *, void*> > m_DataInstances;
    
    std::deque<AVPacketData *> m_AudioPacketDatas;
    std::deque<AVPacketData *> m_VideoPacketDatas;

    AVFormatContext * m_pInFormatCtxA;
    AVFormatContext * m_pInFormatCtxV;
    AVFormatContext * m_pOutFormatCtx;
    AVIOContext * m_pAVIOCtxA;
    AVIOContext * m_pAVIOCtxV;
    AVIOContext * m_pAVIOCtxOut;

    FormatType m_eAFormatType;
    FormatType m_eVFormatType;

    bool m_bRoutineStop;
    bool m_bFCtxAOpened;
    bool m_bFCtxVOpened;
    volatile bool m_bTerminateConsume;

    const uint32_t m_iBufferSize;
    uint64_t m_iCurTimeduration; //us

    Poco::Mutex m_CallbackMutex;
    Poco::Mutex m_DataMutex;
    Poco::Event m_ConsumeEvent;

	AVBitStreamFilterContext* m_pVFilterContext;
	AVBitStreamFilterContext* m_pAFilterContext;
};

#endif // _Mp3Muxer_H_