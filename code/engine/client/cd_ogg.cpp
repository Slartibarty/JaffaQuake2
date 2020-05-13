
#include "client.h"

extern "C" {

int CDAudio_Init(void)
{
	return 0;
}

void CDAudio_Shutdown(void)
{

}

void CDAudio_Play(int track, qboolean looping)
{
	char path[MAX_QPATH];

	Com_sprintf(path, sizeof(path), "music/Track%02d.wav", track);

	S_StartLocalSound(path);
}

void CDAudio_Stop(void)
{

}

void CDAudio_Update(void)
{
	// Nothing to do
}

void CDAudio_Activate(qboolean active)
{
	// Do nothing right now
}

}
