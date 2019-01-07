//
//  DumpRtmpStream.cpp
//  RtmpDumpDemo
//
//  Created by lunli on 2019/1/4.
//  Copyright © 2019年 lunli. All rights reserved.
//

#include "DumpRtmpStream.hpp"
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#include <librtmp/rtmp.h>
#include <librtmp/log.h>
#include "FLVParse.hpp"

RtmpStreamDumper::RtmpStreamDumper(std::string &url, std::string dump_flv_path)
{
    this->rtmp_rsource_url = url;
    if(dump_flv_path.length() == 0) {
        dump_flv_path = this->tempFlvPath();
    }
    this->dump_flv_path = dump_flv_path;
    this->p = NULL;
}


std::string RtmpStreamDumper::tempFlvPath()
{
    std::string result = "";
    char cCurrentPath[FILENAME_MAX] = {0};
    if(GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
    {
        
        time_t t = time(0);   // get time now
        srand((int)t);
        struct tm * now = localtime(&t);
        char buffer [256] = {0};
        strftime (buffer,80,"%Y_%m_%d_%H_%M_%S",now);
        sprintf(buffer, "%s_%d.aac", buffer, rand());
        result = std::string(cCurrentPath) + "//" + std::string(buffer);
    }
    return result;
}


bool RtmpStreamDumper::dumpBytesToFlv(const unsigned char *buffer, unsigned int size)
{
    
    FILE *fp = fopen(this->dump_flv_path.c_str(), "ab");
    if(fp != NULL)
    {
        fwrite(buffer, size, 1, fp);
        fflush(fp);
        fclose(fp);
        return true;
    }
    
    return false;
}


void RtmpStreamDumper::startDump()
{
    int readBytes = 0;
    bool bLiveStream = true;
    int bufsize = 1024*1024*10;
    long countbufsize = 0;
    char *buf  = (char*)calloc(sizeof(char), bufsize);
    char *path = (char*)calloc(sizeof(char), this->rtmp_rsource_url.size() + 1);
    strcpy(path, this->rtmp_rsource_url.c_str());
    
    RTMP_LogPrintf("Start Dump To %s", this->dump_flv_path.c_str());

    
    RTMP *rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    rtmp->Link.timeout=10;
    
    if(!RTMP_SetupURL(rtmp, path))
    {
        RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
        RTMP_Free(rtmp);
        return;
    }
    
    if (bLiveStream){
        rtmp->Link.lFlags|=RTMP_LF_LIVE;
    }

    RTMP_SetBufferMS(rtmp, 3600*1000);

    if(!RTMP_Connect(rtmp,NULL)){
        RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
        RTMP_Free(rtmp);
        return ;
    }
    
    if(!RTMP_ConnectStream(rtmp,0)){
        RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        return ;
    }
    
    while((readBytes = RTMP_Read(rtmp,buf,bufsize))){
        this->dumpBytesToFlv((const unsigned char *)buf, readBytes);
        countbufsize += readBytes;
        RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n",readBytes,countbufsize*1.0/1024);
    }

    if(buf){
        free(buf);
    }
    
    if(rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp=NULL;
    }

}


void RtmpStreamDumper::startDumpAAC()
{
    int readBytes = 0;
    bool bLiveStream = true;
    int bufsize = 1024*1024*10;
    long countbufsize = 0;
    char *buf  = (char*)calloc(sizeof(char), bufsize);
    char *path = (char*)calloc(sizeof(char), this->rtmp_rsource_url.size() + 1);
    strcpy(path, this->rtmp_rsource_url.c_str());
    
//    RTMP_LogSetLevel(RTMP_LOGALL);
    RTMP_LogPrintf("Start Dump To %s", this->dump_flv_path.c_str());
    
    
    RTMP *rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    rtmp->Link.timeout=10;
    
    if(!RTMP_SetupURL(rtmp, path))
    {
        RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
        RTMP_Free(rtmp);
        return;
    }
    
    if (bLiveStream){
        rtmp->Link.lFlags|=RTMP_LF_LIVE;
    }
    
    RTMP_SetBufferMS(rtmp, 3600*1000);
    
    if(!RTMP_Connect(rtmp,NULL)){
        RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
        RTMP_Free(rtmp);
        return ;
    }
    
    if(!RTMP_ConnectStream(rtmp,0)){
        RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        return ;
    }
    
    RTMPPacket packet = {0};
    int failCount = 0;
    while (RTMP_IsConnected(rtmp)) {
        
        readBytes = RTMP_ReadPacket(rtmp, &packet);
        if (packet.m_body == NULL ||
            packet.m_nBodySize == 0 ||
            readBytes <= 0 ) {
            usleep(50);
            
            if(failCount++ < 20 * 5)
                continue;
            else
                break;
        }
        failCount = 0;
        
        RTMP_LogPrintf("Receive Packet: %5d Packet, Total: %5.2fkB\n",readBytes, packet.m_nBodySize*1.0/1024);
        
        if(packet.m_packetType == RTMP_PACKET_TYPE_AUDIO)
        {
            //Audio Packet
            //FLV Audio Tag 原始数据，包含Tag Header， 音频Tag Header一般由一个字节定义（AAC用两个字节）
            //第一个字节的定义如下：音频格式 4bits | 采样率 2bits | 采样精度 1bits | 声道数 1bits|
            /*
             看第2个字节，如果音频格式AAC（0x0A），AudioTagHeader中会多出1个字节的数据AACPacketType，这个字段来表示AACAUDIODATA的类型：
             0x00 = AAC sequence header，类似h.264的sps,pps，在FLV的文件头部出现一次。
             0x01 = AAC raw，AAC数据
             */
            
            //FLV Audio Tag, 完整格式，结构为：【0x08, 3字节包长度，4字节时间戳，00 00 00】，AF 01 N字节AAC数据 | 前包长度
            //其中编码后AAC纯数据长度为N，3字节包长度 = N + 2
            //前包长度 = 11 + 3字节包长度 = 11 + N + 2 = 13 + N。
            //如果要保存AAC为流数据，需要 【ADTS头 + AACRaw数据】【ADTS头 + AACRaw数据】【ADTS头 + AACRaw数据】 写入文件
            
            if(packet.m_nBodySize >= 2 && packet.m_body[1] == 0x00)
            {
                //AAC sequence header
                this->p = new FLVAudioTagHeader((const unsigned char *)(packet.m_body));
                this->p->parse();
                this->p->dumpHeaderInfo();
                //FFMpeg 解析参考
                //https://github.com/herocodemaster/rtmp-cpp/blob/3ec35590675560ac4fa9557ca7a5917c617d9999/RTMP/projects/ffmpeg/src_completo/libavcodec/mpeg4audio.c
                //用bit操作类https://blog.csdn.net/qll125596718/article/details/6901935
                this->p->parseAudioConfig((const char *)(packet.m_body + 2), packet.m_nBodySize-2);
            }
            
            if(packet.m_nBodySize >= 2 && packet.m_body[1] == 0x01)
            {
                //AAC Raw Data
                unsigned char adts[7] = {0};
                this->p->aac_set_adts_head(adts, packet.m_nBodySize - 2);
                this->dumpBytesToFlv(adts, 7);
                this->dumpBytesToFlv((const unsigned char *)(packet.m_body+2), packet.m_nBodySize-2);
            }
            
            RTMPPacket_Free(&packet);
        }
        
        RTMP_ClientPacket(rtmp, &packet);
    }

    if(buf){
        free(buf);
    }
    
    if(rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp=NULL;
    }
}
