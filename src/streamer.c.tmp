#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <librtmp/rtmp.h>
#include <librtmp/log.h>

#define MAX_TAG_BUF_SIZE 256*256*20
#define MAX_TAG_TMP_BUF_SIZE 256
#define MIN_TAG_BUF_SIZE 256*256

typedef struct {
    unsigned char *tag;
    int tagSize;
    int readBytes;
    char isValid;
}TagInfo;


typedef void (*TagAction)(TagInfo tagInfo);

RTMP* makeConnection(char *url){
    RTMP *rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    RTMP_LogSetLevel(RTMP_LOGALL);
    RTMP_LogSetOutput(stderr);
    if(RTMP_SetupURL(rtmp, url) == FALSE)
        return NULL;
    rtmp->Link.timeout = 3;
    RTMP_EnableWrite(rtmp);
    fprintf(stderr, "connecting...");
    if(RTMP_Connect(rtmp, NULL) == FALSE){
        RTMP_Free(rtmp);
        return NULL;
    }
    if (!RTMP_ConnectStream(rtmp, 0)) {
        RTMP_Free(rtmp);
        return NULL;
    }
    return rtmp;
}

int bytesToInt(const unsigned char *bytes, int n){
    int ret = 0;
    int m = 0;
    for(;n > 0; n--)
        ret += (bytes[m++] << ((n-1)*8));
    return ret;
}


TagInfo searchNextTag(const unsigned char * buf, int bufSize, int skipByte){
    TagInfo tagInfo = {NULL, 0, skipByte, 0};
    int rmnBufSize = bufSize;
    const unsigned char *cpBuf = buf + skipByte;

    // fprintf(stderr, "bufSize : %d,  skipByte : %d\n", bufSize, skipByte);

    while(rmnBufSize > 11){
        cpBuf = buf + tagInfo.readBytes;

        /* check data type */
        unsigned char dataType = *cpBuf;
        if (dataType != 0x08 && dataType != 0x09 && dataType != 0x12){
            tagInfo.readBytes ++;
            rmnBufSize --;
            continue;
        }
        cpBuf ++;
        // fprintf(stderr, "pass to check data type\n");


        /* data size */
        int dataSize = bytesToInt(cpBuf, 3);
        // fprintf(stderr, "data size : %d,  buf size : %d\n", dataSize, bufSize);
        if (dataSize > MAX_TAG_BUF_SIZE){
            tagInfo.readBytes ++;
            rmnBufSize --;
            continue;
        }
        if (rmnBufSize < dataSize + 11)
            break;
        cpBuf += 3;

        /* timestamp */
        cpBuf += 4;

        /* check stream id ( 0 padding )*/
        if (cpBuf[0] != 0 || cpBuf[1] != 0 || cpBuf[2] != 0){
            tagInfo.readBytes ++;
            rmnBufSize --;
            continue;
        }
        cpBuf += 3;

        cpBuf += dataSize;
        int preTagSize = bytesToInt(cpBuf, 4);
        //fprintf(stderr, "pre tag size : %d\n", preTagSize);
        if (preTagSize != dataSize + 11){
            tagInfo.readBytes ++;
            rmnBufSize --;
            continue;
        }

        // fprintf(stderr, "valid tag\n");
        /* if reach here, it is valid tag */
        tagInfo.tag = malloc(dataSize + 11);
        memcpy(tagInfo.tag, buf + tagInfo.readBytes, dataSize + 11);
        tagInfo.tagSize = dataSize + 11;
        tagInfo.isValid = 1;
        return tagInfo;
    }

    /* invalid tag buffer */
    tagInfo.isValid = 0;
    return tagInfo;
}



void TagNotify(const TagInfo tagInfo){
    fprintf(stderr, "find tag!! : size is %d\n", tagInfo.tagSize);
}


void RTMP_WriteTag(RTMP *rtmp, TagInfo tagInfo){
    int res = RTMP_Write(rtmp, (const char *)tagInfo.tag, tagInfo.tagSize);
    fprintf(stderr, "rtmp write result : %d\n\n", res);
}


int workLoop(RTMP *rtmp){
    unsigned char buf[MAX_TAG_BUF_SIZE] = {};
    unsigned char *bufPtr = &buf[0];
    unsigned char tmpBuf[MAX_TAG_TMP_BUF_SIZE];
    int readByte = 0;
    int skipBytes = 0;

    while(1){
        if ((readByte = read(0, tmpBuf, MAX_TAG_TMP_BUF_SIZE)) == -1)
            return -1;
        if ((bufPtr - &buf[0]) + readByte > MAX_TAG_BUF_SIZE){
            fprintf(stderr, "buffer overflow !!!\n");
            exit(1);
        }
        memcpy(bufPtr, tmpBuf, readByte);
        bufPtr += readByte;
        if ((bufPtr - &buf[0]) > MIN_TAG_BUF_SIZE){
            /* search tag */
            TagInfo tagInfo = searchNextTag(buf, bufPtr - &buf[0], skipBytes);
            if (tagInfo.isValid){
                RTMP_WriteTag(rtmp, tagInfo);
                // fprintf(stderr, "read bytes : %d\n", tagInfo.readBytes);
                free(tagInfo.tag);
                int bytesToNextBuf = tagInfo.readBytes + tagInfo.tagSize + 4;
                // fprintf(stderr, "tag size : %d\n", tagInfo.tagSize);
                memmove(buf, buf + bytesToNextBuf, (bufPtr - &buf[0]) - bytesToNextBuf);
                // fprintf(stderr, "new buffer : %d %d %d %d %d %d %d %d\n", (int)buf[0], (int)buf[1], (int)buf[2], (int)buf[3], (int)buf[4], (int)buf[5], (int)buf[6], (int)buf[7]);
                bufPtr -= bytesToNextBuf;
                skipBytes = 0;
            }else{
                skipBytes = tagInfo.readBytes;
            }
        }
        
    }
    return 1;
}


int main(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "please set url\n");
        exit(1);
    }
    char *url = argv[1];
    RTMP *rtmp = makeConnection(url);
    if(!rtmp){
        fprintf(stderr, "fail to setup");
        return 1;
    }
    int isConnected = RTMP_IsConnected(rtmp);
    if (!isConnected){
        fprintf(stderr, "接続失敗\n");
        exit(2);
    }
    fprintf(stderr, "接続成功\n");

    workLoop(rtmp);

    return 0;
}
