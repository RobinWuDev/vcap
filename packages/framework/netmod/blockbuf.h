#ifndef _BLOCK_BUFFER_H_
#define _BLOCK_BUFFER_H_

#include "netlog.h"
#include <stdlib.h> //linux need it to identify free function!!!
#include <string.h> //memcpy, memmov
#include <iostream>

#ifndef _WIN32
#include <sys/socket.h>
#include <errno.h>
#ifndef SOCKET
#define SOCKET int
#endif
#else
#include <WinSock2.h>
#endif

template<unsigned BlockSize>
struct Allocator_malloc_free
{
    enum{mBLOCKSIZE = BlockSize};

    static char* ordered_malloc(size_t block)
    { return (char*)::malloc(mBLOCKSIZE * block); }
    static void ordered_free(char* block)
    { ::free(block); }
};

template<unsigned BlockSize>
struct Allocator_new_delete
{
    enum{mBLOCKSIZE = BlockSize};

    static char* ordered_malloc(size_t block)
    { return new char[mBLOCKSIZE * block]; }
    static void ordered_free(char* block)
    { delete[] block;}
};

#ifdef _USE_NEW_DELETE_ALLOCATOR_
    typedef Allocator_new_delete<1 * 1024>  Allocator_Block_1k;
    typedef Allocator_new_delete<2 * 1024>  Allocator_Block_2k;
    typedef Allocator_new_delete<4 * 1024>  Allocator_Block_4k;
    typedef Allocator_new_delete<8 * 1024>  Allocator_Block_8k;
    typedef Allocator_new_delete<16 * 1024> Allocator_Block_16k;
    typedef Allocator_new_delete<32 * 1024> Allocator_Block_32k;
    typedef Allocator_new_delete<64 * 1024> Allocator_Block_64k;
#else
    typedef Allocator_malloc_free<1 * 1024> Allocator_Block_1k;
    typedef Allocator_malloc_free<2 * 1024> Allocator_Block_2k;
    typedef Allocator_malloc_free<4 * 1024> Allocator_Block_4k;
    typedef Allocator_malloc_free<8 * 1024> Allocator_Block_8k;
    typedef Allocator_malloc_free<16 * 1024> Allocator_Block_16k;
    typedef Allocator_malloc_free<32 * 1024> Allocator_Block_32k;
    typedef Allocator_malloc_free<64 * 1024> Allocator_Block_64k;
#endif

template<typename TAllocator = Allocator_Block_4k, unsigned MaxBlockNum = 2>
class BlockBuf
{
    enum{ mMAXBLOCKNUM = MaxBlockNum};
    enum{ mPos = size_t(-1) };//4294967295L
public:
    BlockBuf(): m_size(0), m_blockNum(0), m_data(NULL){}
    virtual ~BlockBuf(){ this->free(); }

public:
    inline char*    data()      { return m_data; }
    inline char*    tail()      { return m_data + m_size; }
    inline size_t   size()      { return m_size; }
    inline size_t   blocksize() { return TAllocator::mBLOCKSIZE; }
    inline size_t   blocknum()  { return m_blockNum; }
    inline size_t   capacity()  { return m_blockNum * TAllocator::mBLOCKSIZE; }
    inline size_t   freespace() { return capacity() - size(); }
    inline bool     empty()     { return size() == 0; }
    inline void     setsize(size_t n)   { m_size = n < capacity() ? n : capacity(); }

    bool     reserve(size_t n);
    bool     append(const char* data, size_t len);
    int      read(SOCKET s, sockaddr_in* pAddr=NULL, int soType=SOCK_STREAM);
    int      write(SOCKET s, const char* data, size_t len, sockaddr_in* pAddr=NULL, int soType=SOCK_STREAM);
    int      flush(SOCKET s, sockaddr_in* pAddr=NULL, int soType=SOCK_STREAM); //flush cached data when onSend event triggered
    void     erase(size_t pos, size_t n, bool hold = false);

protected:
    bool     increase_capacity(size_t increase_size);

private:
    inline void     free()      { TAllocator::ordered_free(m_data); m_data = NULL; m_blockNum = 0; m_size = 0; }

private:
    size_t m_blockNum;
    size_t m_size;
    char*  m_data;
};

template<typename TAllocator, unsigned MaxBlockNum>
bool BlockBuf<TAllocator,MaxBlockNum>::increase_capacity(size_t increate_size)
{
    if ( increate_size == 0 || increate_size <= freespace() )
        return true;

    increate_size -= freespace();
    size_t newBlockNum = m_blockNum;
    newBlockNum += increate_size / TAllocator::mBLOCKSIZE;
    if ( increate_size % TAllocator::mBLOCKSIZE > 0) //still need a block
        newBlockNum++;

    if (newBlockNum > mMAXBLOCKNUM)
        return false; //log here

    char* newData = TAllocator::ordered_malloc(newBlockNum);
    if (newData == NULL)
        return false; //log here
    if ( !empty() )
    {
        //copy old data and free old block
        memcpy(newData, m_data, m_size);
        TAllocator::ordered_free(m_data);
    }
    m_data = newData;
    m_blockNum = newBlockNum;
    return true;
}

template<typename TAllocator, unsigned MaxBlockNum>
bool BlockBuf<TAllocator, MaxBlockNum>::append(const char* data, size_t len)
{
    if (len == 0)
        return true; // no data

    if (increase_capacity(len))
    {
        memmove(tail(), data, len); // append
        m_size += len;
        return true;
    }
    return false;
}

template<typename TAllocator, unsigned MaxBlockNum>
int BlockBuf<TAllocator, MaxBlockNum>::read(SOCKET s, sockaddr_in* pAddr/*=NULL*/, int soType/*=SOCK_STREAM*/)
{
    if (freespace() < (blocksize() >> 1) 
        && blocknum() < mMAXBLOCKNUM)
        // ignore increase_capacity result.
        increase_capacity(blocksize());

    size_t nrecv = freespace() < mPos ? freespace() : mPos;  // min(mPos, freespace());
    if (nrecv == 0) 
        return -1;

    int ret = 0;
    if ( SOCK_STREAM == soType )
        ret = ::recv(s, (char*)tail(), (int)nrecv, 0);
    else if ( SOCK_DGRAM == soType )
    {
#ifdef WIN32
        int addr_len = sizeof(struct sockaddr);
#else
        socklen_t addr_len = sizeof(struct sockaddr);
#endif
        ret = ::recvfrom(s, (char*)tail(), (int)nrecv, 0, (struct sockaddr*)pAddr, &addr_len );
    }
    if (ret > 0)
        m_size += ret;
    return ret;
}

template<typename TAllocator, unsigned MaxBlockNum>
int BlockBuf<TAllocator, MaxBlockNum>::write(SOCKET s, const char* data, size_t len, sockaddr_in* pAddr/*=NULL*/, int soType/*=SOCK_STREAM*/)
{
    if (len == 0) 
        return -1;

    if(blocknum() > mMAXBLOCKNUM) 
        return -1;

    int nsent = 0;
    if (empty()) //call send as no data cached in buffer,otherwise,socket can't send anything and we should cache the data into buffer until onSend event was given
    {
        if ( SOCK_STREAM == soType )
            nsent = ::send(s , data, (int)len, 0);
        else if ( SOCK_DGRAM == soType )
            nsent = ::sendto(s, data, (int)len, 0, (struct sockaddr*)pAddr, sizeof(struct sockaddr));
    }

    if(nsent < 0)
    {
#ifdef WIN32
        if (GetLastError() == WSAEINTR || GetLastError() == WSAEINPROGRESS)
            nsent = 0;
#endif

#ifdef unix
        if(errno == EAGAIN || errno == EINTR || errno == EINPROGRESS)
            nsent = 0;
        //else
            //throw buffer_overflow("send error");
#endif
    }

    if (!append(data + nsent, len - nsent))
    {
        // all or part append error .
//         if (nsent > 0) 
//             throw buffer_overflow("output buffer overflow");
//         else 
//             throw buffer_overflow_all("output buffer overflow [all]");
    }
    return (int)nsent;
}

template<typename TAllocator, unsigned MaxBlockNum>
int BlockBuf<TAllocator, MaxBlockNum>::flush(SOCKET s, sockaddr_in* pAddr/*=NULL*/, int soType/*=SOCK_STREAM*/)
{
    int ret = 0;
    if ( SOCK_STREAM == soType )
        ret = ::send(s, (const char*)data(), (int)size(), 0);
    else if ( SOCK_DGRAM == soType )
        ret = ::sendto(s, (const char*)data(), (int)size(), 0, (struct sockaddr*)pAddr, sizeof(struct sockaddr) );
    erase( 0, ret );
    return ret;
}

template<typename TAllocator, unsigned MaxBlockNum>
bool BlockBuf<TAllocator, MaxBlockNum>::reserve(size_t n)
{
    return (n <= capacity() || increase_capacity(n - capacity()));
}

template<typename TAllocator, unsigned MaxBlockNum>
void BlockBuf<TAllocator, MaxBlockNum>::erase(size_t pos, size_t n, bool hold)
{
    if (pos > size())
        pos = size();

    size_t m = size() - pos; // can erase
    if (n >= m)
        m_size = pos; // all clear after pos
    else
    {
        m_size -= n;
        memmove(m_data + pos, m_data + pos + n, m - n);
    }

    if (empty() && !hold)
    {
        free(); //free all buffer!!!

		//NET_LOG("BlockBuf::erase, free all buffer.");
    }
}

typedef BlockBuf<Allocator_Block_8k, 8>     Buffer8x8k;  //64k
typedef BlockBuf<Allocator_Block_8k, 16>    Buffer8x16k; //128k
typedef BlockBuf<Allocator_Block_8k, 32>    Buffer8x32k; //256k
typedef BlockBuf<Allocator_Block_32k, 16>   Buffer32x16k; //512k
typedef BlockBuf<Allocator_Block_32k, 32>   Buffer32x32k; //1M Wow!!!
typedef BlockBuf<Allocator_Block_64k, 64>   Buffer64x64k; //4M Wow!!!

typedef Buffer64x64k  inputbuf_t;
typedef Buffer32x32k  outputbuf_t;

#endif
