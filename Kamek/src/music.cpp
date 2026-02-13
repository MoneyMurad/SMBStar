#include <game.h>
#include <sfx.h>
#include "music.h"

struct HijackedStream {
	u32 stringOffset;
	u32 stringOffsetFast;
	u32 infoOffset;
	u8 originalID;
	int streamID;

	// NEW: backups for undo
	char originalName[64];
	char originalFastName[64];
	u16 originalChannelCount;
	u16 originalTrackMask;
	bool hasBackup;
};

struct Hijacker {
	HijackedStream stream[2];
	u8 currentStream;
	s8 currentCustomTheme;
};

const char* SongNameList [] = {
	"AIRSHIP","BOSS_TOWER","MENU","UNDERWATER","ATHLETIC","CASTLE","MAIN",
	"MOUNTAIN","TOWER","UNDERGROUND","DESERT","FIRE","FOREST","FREEZEFLAME",
	"JAPAN","PUMPKIN","SEWER","SPACE","BOWSER","BONUS","AMBUSH","BRIDGE_DRUMS",
	"SNOW2","MINIMEGA","CLIFFS","AUTUMN","CRYSTALCAVES","GHOST_HOUSE","GRAVEYARD",
	"JUNGLE","TROPICAL","SKY_CITY","SNOW","STAR_HAVEN","SINGALONG","FACTORY",
	"TANK","TRAIN","YOSHIHOUSE","FACTORYB","CAVERN","SAND","SHYGUY","MINIGAME",
	"BONUS_AREA","CHALLENGE","BOWSER_CASTLE","","","","","","","","","",
	"BOSS_CASTLE","BOSS_AIRSHIP",NULL
};

#define _I(offs) ((offs)-0x212C0)

Hijacker Hijackers[2] = {
	{
		{
			{_I(0x4A8F8), _I(0x4A938), _I(0x476C4), 4, STRM_BGM_ATHLETIC},
			{_I(0x4B2E8), _I(0x4B320), _I(0x48164), 10, STRM_BGM_SHIRO}
		},
		0, -1
	},
	{
		{
			{_I(0x4A83C), _I(0x4A8B4), 0, 1, STRM_BGM_CHIJOU},
			{_I(0x4A878), _I(0x4A780), 0, 2, STRM_BGM_CHIKA},
		},
		0, -1
	}
};

extern void *SoundRelatedClass;
inline char *BrsarInfoOffset(u32 offset) {
	return (char*)(*(u32*)(((u32)SoundRelatedClass) + 0x5CC)) + offset;
}

void FixFilesize(u32 streamNameOffset);

u8 hijackMusicWithSongName(const char *songName, int themeID, bool hasFast,
	int channelCount, int trackCount, int *wantRealStreamID)
{
	Hijacker *hj = &Hijackers[channelCount==4?1:0];

	if ((themeID >= 0) && hj->currentCustomTheme == themeID)
		return hj->stream[hj->currentStream].originalID;

	int toUse = (hj->currentStream + 1) & 1;

	hj->currentStream = toUse;
	hj->currentCustomTheme = themeID;

	HijackedStream *stream = &hj->stream[hj->currentStream];

	// BACKUP ORIGINAL DATA (once)
	if (!stream->hasBackup) {
		strcpy(stream->originalName, BrsarInfoOffset(stream->stringOffset));
		strcpy(stream->originalFastName, BrsarInfoOffset(stream->stringOffsetFast));

		if (stream->infoOffset) {
			u16 *thing = (u16*)(BrsarInfoOffset(stream->infoOffset) + 4);
			stream->originalChannelCount = thing[0];
			stream->originalTrackMask = thing[1];
		}

		stream->hasBackup = true;
	}

	// MODIFY STREAM INFO
	if (stream->infoOffset) {
		u16 *thing = (u16*)(BrsarInfoOffset(stream->infoOffset) + 4);
		thing[0] = channelCount;
		thing[1] = (1 << trackCount) - 1;
	}

	// MODIFY FILENAMES
	sprintf(BrsarInfoOffset(stream->stringOffset),
		"stream/%s.brstm", songName);

	sprintf(BrsarInfoOffset(stream->stringOffsetFast),
		hasFast ? "stream/%s_F.brstm" : "stream/%s.brstm",
		songName);

	FixFilesize(stream->stringOffset);
	FixFilesize(stream->stringOffsetFast);

	if (wantRealStreamID)
		*wantRealStreamID = stream->streamID;

	return stream->originalID;
}

// UNDO FUNCTION
void undoHijackMusic(int channelCount)
{
	Hijacker *hj = &Hijackers[channelCount==4?1:0];
	HijackedStream *stream = &hj->stream[hj->currentStream];

	if (!stream->hasBackup)
		return;

	if (stream->infoOffset) {
		u16 *thing = (u16*)(BrsarInfoOffset(stream->infoOffset) + 4);
		thing[0] = stream->originalChannelCount;
		thing[1] = stream->originalTrackMask;
	}

	sprintf(BrsarInfoOffset(stream->stringOffset), "%s", stream->originalName);
	sprintf(BrsarInfoOffset(stream->stringOffsetFast), "%s", stream->originalFastName);

	FixFilesize(stream->stringOffset);
	FixFilesize(stream->stringOffsetFast);

	hj->currentCustomTheme = -1;
	hj->currentStream = 0;

	nw4r::snd::SndAudioMgr::instance->stopAllBgm(0);
}

//oh for fuck's sake
#include "fileload.h"

void FixFilesize(u32 streamNameOffset) {
	char *streamName = BrsarInfoOffset(streamNameOffset);

	char nameWithSound[80];
	snprintf(nameWithSound, 79, "/Sound/%s", streamName);

	s32 entryNum;
	DVDHandle info;

	if ((entryNum = DVDConvertPathToEntrynum(nameWithSound)) >= 0) {
		if (DVDFastOpen(entryNum, &info)) {
			u32 *lengthPtr = (u32*)(streamName - 0x1C);
			*lengthPtr = info.length;
		}
	} else
		OSReport("What, I couldn't find \"%s\" :(\n", nameWithSound);
}

extern "C" u8 after_course_getMusicForZone(u8 realThemeID) {
	if (realThemeID < 100)
		return realThemeID;

	bool usesDrums = (realThemeID >= 200);
	const char *name = SongNameList[realThemeID - (usesDrums ? 200 : 100)];
	return hijackMusicWithSongName(name, realThemeID, true,
		usesDrums?4:2, usesDrums?2:1, 0);
}
