#pragma once
#include <Windows.h>

class CPacketReader
{
public:
    CPacketReader(void* pBuffer, unsigned int nBufferLength)
    {
        ReceiveLength = nBufferLength;
        ReceivePos = 0;
        ReceiveBuffer = (BYTE*)pBuffer;
    }
    ~CPacketReader()
    {
        
    }

    template<typename T>
    T Read()
    {
        if (ReceiveLength <= ReceivePos)
            return T{};
        if (sizeof(T) > (ReceiveLength - ReceivePos))
            return T{};
        T Value = *(T*)(ReceiveBuffer + ReceivePos);
        ReceivePos = ReceivePos + sizeof(T);
        return Value;
    }
    
    bool Skip(unsigned int num)
    {
        if (ReceivePos + num <= ReceiveLength)
        {
            ReceivePos = ReceivePos + num;
            return true;
        }
        return false;
    }
private:
    unsigned int ReceiveLength = 0;
    unsigned int ReceivePos = 0;
    BYTE* ReceiveBuffer = nullptr;
};
