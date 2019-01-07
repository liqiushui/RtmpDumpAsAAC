//
//  FLVParse.hpp
//  RtmpDumpAAC
//
//  Created by lunli on 2019/1/4.
//  Copyright © 2019年 lunli. All rights reserved.
//

#ifndef FLVParse_hpp
#define FLVParse_hpp

#include <stdio.h>

#define ADTS_HEADER_SIZE    7

const int ff_mpeg4audio_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};


const unsigned char ff_mpeg4audio_channels[8] = {
    0, 1, 2, 3, 4, 5, 6, 8
};


typedef struct
{
    int objecttype;
    int sample_rate_index;
    int channel_conf;
    
}ADTSContext;



class FLVAudioTagHeader {
    
public:
    FLVAudioTagHeader(const unsigned char *pHeaderData)
    {
        this->rawHeaderData[0] = pHeaderData[0];
        this->rawHeaderData[1] = pHeaderData[1];
    }
    
    void parse()
    {
        this->formate = (this->rawHeaderData[0] & 0b11110000)>>4;
        this->sampleRate = (this->rawHeaderData[0] & 0b00001100)>>2;
        this->precision = (this->rawHeaderData[0] & 0b00000010)>>1;
        this->channel = this->rawHeaderData[0] & 0b00000001;
        this->aacType = this->rawHeaderData[1];
    }
    
    int aac_decode_extradata(ADTSContext *adts, unsigned char *pbuf, int bufsize)
    {
        int aot, aotext, samfreindex;
        int i, channelconfig;
        unsigned char *p = pbuf;
        if (!adts || !pbuf || bufsize<2)
        {
            return -1;
        }
        aot = (p[0]>>3)&0x1f;
        if (aot == 31)
        {
            aotext = (p[0]<<3 | (p[1]>>5)) & 0x3f;
            aot = 32 + aotext;
            samfreindex = (p[1]>>1) & 0x0f;
            if (samfreindex == 0x0f)
            {
                channelconfig = ((p[4]<<3) | (p[5]>>5)) & 0x0f;
            }
            else
            {
                channelconfig = ((p[1]<<3)|(p[2]>>5)) & 0x0f;
            }
        }
        else
        {
            samfreindex = ((p[0]<<1)|p[1]>>7) & 0x0f;
            if (samfreindex == 0x0f)
            {
                channelconfig = (p[4]>>3) & 0x0f;
            }
            else
            {
                channelconfig = (p[1]>>3) & 0x0f;
            }
        }
#ifdef AOT_PROFILE_CTRL
        if (aot < 2) aot = 2;
#endif
        adts->objecttype = aot-1;
        adts->sample_rate_index = samfreindex;
        adts->channel_conf = channelconfig;
        return 0;
    }
    
    int aac_set_adts_head(unsigned char *buf, int size)
    {
        ADTSContext *acfg = &this->ctx;
        unsigned char byte;
        if (size < ADTS_HEADER_SIZE)
        {
            return -1;
        }
        buf[0] = 0xff;
        buf[1] = 0xf1;
        byte = 0;
        byte |= (acfg->objecttype & 0x03) << 6;
        byte |= (acfg->sample_rate_index & 0x0f) << 2;
        byte |= (acfg->channel_conf & 0x07) >> 2;
        buf[2] = byte;
        byte = 0;
        byte |= (acfg->channel_conf & 0x07) << 6;
        byte |= (ADTS_HEADER_SIZE + size) >> 11;
        buf[3] = byte;
        byte = 0;
        byte |= (ADTS_HEADER_SIZE + size) >> 3;
        buf[4] = byte;
        byte = 0;
        byte |= ((ADTS_HEADER_SIZE + size) & 0x7) << 5;
        byte |= (0x7ff >> 6) & 0x1f;
        buf[5] = byte;
        byte = 0;
        byte |= (0x7ff & 0x3f) << 2;
        buf[6] = byte;
        
        return 0;
        
    }

    
    void parseAudioConfig(const char *configData, unsigned int size)
    {
        if(size > 2)
        {
            /*
             this->profile = ((configData[2]&0xc0)>>6)+1;
             this->sample_rate_index = (configData[2]&0x3c)>>2;
             this->channel = ((configData[2]&0x1)<<2)|((configData[3]&0xc0)>>6);
             */
            //感谢 https://blog.csdn.net/lichen18848950451/article/details/78266054
            this->aac_decode_extradata(&this->ctx, (unsigned char *)configData, size);
        }
    }
    
    void dumpHeaderInfo()
    {
        const char *formates[] = {
            "Linear PCM, platform endian",
            "ADPCM",
            "MP3",
            "Linear PCM, little endian",
            "Nellymoser 16-kHz mono",
            "Nellymoser 8-kHz mono",
            "Nellymoser",
            "G.711 A-law logarithmic PCM",
            "G.711 mu-law logarithmic PCM",
            "reserved",
            "AAC",
            "Speex",
            "MP3 8-Khz",
            "Device-specific sound",
        };
        
        const char *rates[] = {
            "5.5-kHz",
            "11-kHz",
            "22-kHz",
            "44-kHz",
        };
        
        if(this->formate < 14)
        {
            printf("formate: %s \n", formates[this->formate]);
        }
        else
        {
            printf("formate: unknown \n");
        }
        
        if(this->sampleRate < 4)
        {
            printf("sampleRate: %s \n", rates[this->sampleRate]);
        }
        else
        {
            printf("sampleRate: unknown \n");
        }
        
        if(this->precision > 0)
        {
             printf("precision: %s \n", "snd16Bit");
        }
        else
        {
            printf("precision: %s \n", "snd8Bit");
        }
        
        if(this->channel > 0)
        {
            printf("channel: %s \n", "sndStereo");
        }
        else
        {
            printf("channel: %s \n", "sndMono");
        }
        
        if(this->aacType > 0)
        {
            printf("channel: %s \n", "AAC raw");
        }
        else
        {
            printf("channel: %s \n", "AAC sequence header");
        }
    }
    
public:
    //音频格式 4bits
    unsigned char formate;
    //采样率 2bits
    unsigned char sampleRate;
    //采样精度 1bits
    unsigned char precision;
    //声道数 1bits
    unsigned char channel;
    //AACPacketType 表示AACAUDIODATA的类型 8 bits
    //0x00 = AAC sequence header. 0x01 = AAC raw，AAC数据
    unsigned char aacType;
    
private:
    //header 原始数据,16bits
    unsigned char rawHeaderData[2];
    ADTSContext ctx;
};

#endif /* FLVParse_hpp */
