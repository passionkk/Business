#ifndef _TSMuxer_H_
#define _TSMuxer_H_

#include <stdint.h>
#include <vector>
#include <deque>
#include <Poco/Thread.h>

#include "../Common/RecordBase.h"

class TSDataIface
{
public:
    virtual ~TSDataIface() {}

    virtual void DataHandle(
        void *pUserData,
        const uint8_t *pData,
        uint32_t iLen,
        uint64_t iPts /*ms*/) = 0;

};

class TSMuxer : public Poco::Runnable
{
public:
    typedef void(*DataHandle)(
        void *pUserData,
        const uint8_t *pData,
        uint32_t iLen,
        uint64_t iPts /*ms*/);

    struct AVPacketData
    {
        uint8_t * pData;
        int iDataSize;
        int iOffset;
        int iProbeOffset;
        uint64_t iTimestamp; // us
        FormatType eFormatType;
    };

    TSMuxer();

    ~TSMuxer();

    // 添加关注输出数据的函数指针
    void Subscribe(DataHandle dataHandle, void *pUserData);

    // 取消关注输出数据的函数指针
    void UnSubscribe(DataHandle dataHandle);

    // 添加关注输出数据的实例对象
    void Subscribe(TSDataIface *pDataInstance, void *pUserData);

    // 取消关注输出数据的实例对象
    void UnSubscribe(TSDataIface *pDataInstance);

    // 导入音视频数据
    bool ImportAVPacket(
        MediaType eMediaType,
        FormatType eFormatType,
        uint8_t *pData,
        int iDataSize,
        uint64_t iTimestamp /*us*/);

    // 开始
    bool Start(
        FormatType eAFormatType,
        FormatType eVFormatType,
        int iAudioReadSize = 32768,
        int iVideoReadSize = 32768);

    // 停止
    void Stop();

    // 后台线程运行并有缓存数据
    bool IsBusy();

    // 后台线程是否运行
    bool IsRunning() const;

public:
    // 仅供内部回调函数使用
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
    std::vector<std::pair<TSDataIface *, void*> > m_DataInstances;
    
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
};

#endif // _TSMuxer_H_