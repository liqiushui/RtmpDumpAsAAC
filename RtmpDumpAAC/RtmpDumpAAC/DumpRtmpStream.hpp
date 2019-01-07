//
//  DumpRtmpStream.hpp
//  RtmpDumpDemo
//
//  Created by lunli on 2019/1/4.
//  Copyright © 2019年 lunli. All rights reserved.
//

#ifndef DumpRtmpStream_hpp
#define DumpRtmpStream_hpp

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "FLVParse.hpp"

class RtmpStreamDumper {
public:
    RtmpStreamDumper(std::string &url, std::string dump_flv_path = "");
    std::string tempFlvPath();
    bool dumpBytesToFlv(const unsigned char *buffer, unsigned int size);
    void startDump();
    void startDumpAAC();
    
    std::string rtmp_rsource_url;
    std::string dump_flv_path;
    FLVAudioTagHeader *p;
};

#endif /* DumpRtmpStream_hpp */
