#include <game.h>
#include <sfx.h>
#include "music.h"
#include "fileload.h"

const char* SFXNameList [] = {
	"original",				// 1999, DON'T USE THIS ONE
	
	"bird_chirp_1",
	"bird_chirp_2",
	"bird_chirp_3",
	"bird_fly",			// 2003

	"mega_jump",		// 2004
	"mega_stomp",		// 2005
	"mega_powerup",		// 2006
	"mega_powerdown",	// 2007

	"faceblock_down",	// 2008
	"faceblock_up",		// 2009
	"faceblock_bounce", // 2010
	
	NULL
};

int currentSFX = -1;
u32 *currentPtr = 0;

extern void loadFileAtIndex(u32 *filePtr, u32 fileLength, u32* whereToPatch);

// static FileHandle handle;
extern u32* GetCurrentPC();


extern "C" u32 NewSFXTable[];
extern "C" u32 NewSFXIndexes;

void loadAllSFXs() {
    u32 currentIdx = (u32)&NewSFXIndexes;

    for(int sfxIndex = 0; SFXNameList[sfxIndex] != NULL; sfxIndex++) {
        FileHandle handle;

        char nameWithSound[80];
        snprintf(nameWithSound, 79, "/Sound/stream/sfx/%s.rwav", SFXNameList[sfxIndex]);

        u32 filePtr = (u32)LoadFile(&handle, nameWithSound);

		if (!filePtr) {
			OSReport("FAILED TO LOAD: %s\n", nameWithSound);
			continue;
		}
		
        NewSFXTable[sfxIndex] = currentIdx;
        loadFileAtIndex((u32*)filePtr, handle.length, (u32*)currentIdx);

        currentIdx += handle.length;
        currentIdx += (currentIdx % 0x10);

        FreeFile(&handle);
    }
}


int hijackSFX(int SFXNum) {
	int nameIndex = SFXNum - 1999;
	if(currentSFX == nameIndex) {
		return 189;
	}

	currentPtr = (u32*)NewSFXTable[nameIndex];
	currentSFX = nameIndex;

	return 189;
}

static nw4r::snd::StrmSoundHandle yoshiHandle;

void fuckingYoshiStuff() {
	PlaySoundWithFunctionB4(SoundRelatedClass, &yoshiHandle, 189, 1);
}
