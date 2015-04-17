#include "PcmHandler.h"
#include <OpenHome/Printer.h>

using namespace OpenHome;
using namespace OpenHome::Media;


OsxPcmProcessor::OsxPcmProcessor() : IPcmProcessor()
, iSampleBufferLock("SBLK")
, iSemHostReady("HRDY", 0)
{
    iReadIndex = iWriteIndex = iBytesToRead = 0;
}

void OsxPcmProcessor::enqueue(MsgPlayable *msg)
{
    Log::Print("waiting to enqueue msg=%p\n", msg);
    iSemHostReady.Wait();
    iSemHostReady.Clear();
    Log::Print("enqueuing msg=%p\n", msg);
    queue.Enqueue(msg);
    Log::Print("enqueued msg=%p\n", msg);
}

MsgPlayable * OsxPcmProcessor::dequeue()
{
    Log::Print("waiting to dequeue msg\n");
    MsgPlayable *msg = static_cast<MsgPlayable *>(queue.Dequeue());
    Log::Print("dequeued msg=%p\n", msg);
    
    return msg;
}

/**
 * Set the buffer to be used for packet reading
 */
void OsxPcmProcessor::setBuffer(AudioQueueBufferRef buf)
{
    Log::Print("setBuffer()\n");
    iBuff = buf;
    iBuffsize = buf->mAudioDataBytesCapacity;
    buf->mAudioDataByteSize = 0;
    iReadIndex = iWriteIndex = 0;
    iBytesToRead = iBuffsize;
}

void OsxPcmProcessor::fillBuffer(AudioQueueBufferRef inBuffer)
{
    
    if(queue.NumMsgs() == 0)
    {
        Log::Print("fillBuffer - signalling host ready\n");
        iSemHostReady.Signal();
    }
    else
    {
        Log::Print("fillBuffer - %u queued entries available\n", queue.NumMsgs());
    }
    
    MsgPlayable *msg = dequeue();
    
    Msg *remains = nil;
    /* read the packet, release and remove */
    if(msg->Bytes() > iBytesToRead)
        remains = msg->Split(iBytesToRead);
    msg->Read(*this);
    msg->RemoveRef();

    /* requeue the remaining bytes */
    if(remains != nil)
        queue.EnqueueAtHead(remains);
    
    Log::Print("fillBuffer - size of host buffer %u\n", size());
    inBuffer->mAudioDataByteSize = size();
}

/**
 * Gives the processor a chance to copy memory in a single block.
 *
 * Is not guaranteed to be called so all processors must implement ProcessSample.
 * Bit depth is indicated in function name; number of channels is passed as a parameter.
 *
 * @param aData         Packed big endian pcm data.  Will always be a complete number of samples.
 * @param aNumChannels  Number of channels.
 *
 * @return  true if the fragment was processed (meaning that ProcessSample will not be called for aData);
 *          false otherwise (meaning that ProcessSample will be called for each sample in aData).
 */
TBool OsxPcmProcessor::ProcessFragment(const Brx& aData, TByte aSampleSize, TUint aNumChannels)
{
    AutoMutex _(iSampleBufferLock);
    
    iFrameSize = aSampleSize * aNumChannels;
    
    /* figure out how much data we can copy between the current start point and the end of the buffer */
    TUint32 blocksize = fmin(aData.Bytes(), iBuffsize - iWriteIndex);
    Log::Print("ProcessFragment - data buffer has : %d bytes\n", aData.Bytes());
    Log::Print("Processing : %d bytes starting with %.2x%.2x%.2x%.2x\n", blocksize,
               aData.Ptr()[0],aData.Ptr()[1],aData.Ptr()[2],aData.Ptr()[3]);
    
    memcpy(&(((char *)iBuff->mAudioData)[iWriteIndex]), aData.Ptr(), blocksize);
    iBytesToRead -= blocksize;
    iWriteIndex += blocksize;
    
    return true;
}

/**
 * Optional function.  Gives the processor a chance to copy memory in a single block.
 *
 * Is not guaranteed to be called so all processors must implement ProcessSample.
 * Bit depth is indicated in function name; number of channels is passed as a parameter.
 *
 * @param aData         Packed big endian pcm data.  Will always be a complete number of samples.
 * @param aNumChannels  Number of channels.
 *
 * @return  true if the fragment was processed (meaning that ProcessSample will not be called for aData);
 *          false otherwise (meaning that ProcessSample will be called for each sample in aData).
 */
TBool OsxPcmProcessor::ProcessFragment8(const Brx& aData, TUint aNumChannels)
{
    return ProcessFragment(aData, 1, aNumChannels);
}

TBool OsxPcmProcessor::ProcessFragment16(const Brx& aData, TUint aNumChannels)
{
    return ProcessFragment(aData, 2, aNumChannels);
}
TBool OsxPcmProcessor::ProcessFragment24(const Brx& aData, TUint aNumChannels)
{
    return ProcessFragment(aData, 3, aNumChannels);
}


/**
 * Process a single sample of audio.
 *
 * Data is packed and big endian.
 * Bit depth is indicated in function name; number of channels is passed as a parameter.
 *
 * @param aSample  Pcm data for a single sample.  Length will be (bitDepth * numChannels).
 */
void OsxPcmProcessor::ProcessSample(const TByte* aSample, const TUint8 aSampleSize, TUint aNumChannels)
{
    AutoMutex _(iSampleBufferLock);
    
    iFrameSize = aSampleSize * aNumChannels;
    
    /* figure out how much data we can copy between the current start point and the end of the buffer */
    TUint32 dataSize = iFrameSize;
    Log::Print("Processing sample: %d bytes\n", dataSize);
    
    TUint32 blocksize = fmin(dataSize, iBuffsize - iWriteIndex);
    memcpy(&(((char *)iBuff->mAudioData)[iWriteIndex]), aSample, blocksize);
    iBytesToRead -= blocksize;
    iWriteIndex += blocksize;
}


void OsxPcmProcessor::ProcessSample8(const TByte* aSample, TUint aNumChannels)
{
    ProcessSample(aSample, 1, aNumChannels);
}
void OsxPcmProcessor::ProcessSample16(const TByte* aSample, TUint aNumChannels)
{
    ProcessSample(aSample, 2, aNumChannels);
}
void OsxPcmProcessor::ProcessSample24(const TByte* aSample, TUint aNumChannels)
{
    ProcessSample(aSample, 3, aNumChannels);
}


