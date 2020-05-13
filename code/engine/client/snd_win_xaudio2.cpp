// XAudio2

#include "client.h"
#include "snd_local.h"

#include "../shared/winquake.h"
#include <xaudio2.h>

#define SECONDARY_BUFFER_SIZE	0x10000

IXAudio2*				pXA2;
IXAudio2MasteringVoice* pMV;
IXAudio2SourceVoice*	pSV;

/*
==================
SNDDMA_InitXA2

Direct-Sound support
Returns false if failed
==================
*/
static qboolean SNDDMA_InitXA2(void)
{
	HRESULT hr;

	Com_Print("Initializing XAudio2\n");

	if (FAILED(hr = XAudio2Create(&pXA2, 0, XAUDIO2_USE_DEFAULT_PROCESSOR)))
	{
		return false;
	}

	if (FAILED(hr = pXA2->CreateMasteringVoice(&pMV)))
	{
		return false;
	}

	dma.channels = 2;
	dma.samplebits = 16;
	dma.speed = 44100;

	dma.samples = SECONDARY_BUFFER_SIZE / (dma.samplebits / 8);
	dma.submission_chunk = 1;
	dma.buffer = NULL;			// must be locked first

	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	if (FAILED(hr = pXA2->CreateSourceVoice(&pSV, &format)))
	{
		return false;
	}

	dma.buffer = (byte*)malloc(SECONDARY_BUFFER_SIZE);

	XAUDIO2_BUFFER pBuf;
	pBuf.Flags = 0;
	pBuf.AudioBytes = SECONDARY_BUFFER_SIZE;
	pBuf.pAudioData = dma.buffer;
	pBuf.PlayBegin = 0;
	pBuf.PlayLength = 0;
	pBuf.LoopBegin = 0;
	pBuf.LoopLength = 0;
	pBuf.LoopCount = XAUDIO2_LOOP_INFINITE;
	pBuf.pContext = nullptr;

	if (FAILED(hr = pSV->SubmitSourceBuffer(&pBuf)))
	{
		return false;
	}

	if (FAILED(hr = pSV->Start()))
	{
		return false;
	}

	return true;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos(void)
{
	XAUDIO2_VOICE_STATE state;
	pSV->GetState(&state);

	return (int)state.SamplesPlayed;
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
void SNDDMA_BeginPainting(void)
{
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit(void)
{
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/
qboolean SNDDMA_Init(void)
{
	memset((void*)&dma, 0, sizeof(dma));

	if (!SNDDMA_InitXA2()) {
		return false;
	}

	Com_DPrint("Completed successfully\n");

	return true;
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown(void)
{
	Com_DPrint("Shutting down sound system\n");

	free(dma.buffer);

	memset((void*)&dma, 0, sizeof(dma));
}

/*
===========
SNDDMA_Activate

Called when the main window gains or loses focus.
The window have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void SNDDMA_Activate(qboolean active)
{
}
