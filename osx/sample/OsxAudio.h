#ifndef HEADER_PIPELINE_OSX_AUDIO
#define HEADER_PIPELINE_OSX_AUDIO

#include <AudioToolbox/AudioToolbox.h>
#include <AudioToolbox/AudioQueue.h>
#include <OpenHome/Types.h>
#include <OpenHome/Thread.h>
#include <OpenHome/Media/Pipeline/Msg.h>
#include <OpenHome/Media/ClockPuller.h>
#include <OpenHome/Media/Utils/ProcessorPcmUtils.h>
#include <OpenHome/Net/Core/DvDevice.h>
#include <OpenHome/Media/Pipeline/Pipeline.h>

#include "PcmHandler.h"

namespace OpenHome {
    class Environment;
namespace Media {

class OsxAudio : public Thread
{
    static const TInt32 kNumDataBuffers = 5;
    
public:
    /* Initialise the OsxAudio thread with a refill semaphore
     * which we signal when a host buffer has been consumed
     */
    OsxAudio();
    ~OsxAudio();
    
    void fillBuffer(AudioQueueBufferRef inBuffer);
    
    void initialise(OsxPcmProcessor *aPcmHandler, AudioStreamBasicDescription *format);
    void finalise();
    void startQueue();
    void stopQueue();
    void setVolume(Float32 volume);
    void Quit();


private: // from Thread
    void Run();
    
private:
    void initAudioQueue();
    void initAudioBuffers();
    void finaliseAudioQueue();
    void finaliseAudioBuffers();
    void primeAudioBuffers();
    
private:
    OsxPcmProcessor *iPcmHandler;
    Semaphore iInitialised;
    Mutex iHostLock;
    bool      iPlaying;
    bool      iQuit;
    
    /* Define the relative audio level of the output stream. Defaults to 1.0f. */
    Float32 iVolume;
    
    /* describe the audio format of the active stream */
    AudioStreamBasicDescription iAudioFormat;
    
    // the audio queue object being used for playback
    AudioQueueRef iAudioQueue;
    
    // the audio queue buffers for the playback audio queue
    AudioQueueBufferRef iAudioQueueBuffers[kNumDataBuffers];
};

} // namespace Media
} // namespace OpenHome

#endif // HEADER_PIPELINE_OSX_AUDIO