#include <common.h>
#include <game.h>
#include <sfx.h>
#include <profile.h>

/************************************************************
    Snake Tile Spawner
    - Spawns tile 0xF6
    - Can also break/remove tile
    - Self deletes after spawning
************************************************************/

const char *SnakeSpawnerFileList[] = { NULL };

class daEnSnakeSpawner : public dStageActor_c {
	public:
		static dActor_c* build();
	
		int onCreate();
		int onExecute();
		int onDelete();
	
		// callable helpers
		void SpawnTile();
		void BreakTile();
};

const SpriteData SnakeSpawnerData = {
    ProfileId::snakespawner,
    8, -8, 0, 0,
    0x100, 0x100,
    0, 0, 0, 0, 0
};

Profile SnakeSpawnerProfile(
    &daEnSnakeSpawner::build,
    SpriteId::snakespawner,
    &SnakeSpawnerData,
    ProfileId::snakespawner,
    ProfileId::snakespawner,
    "snakespawner",
    SnakeSpawnerFileList
);

/************************************************************
    Build
************************************************************/
dActor_c* daEnSnakeSpawner::build() {
    return new(AllocFromGameHeap1(sizeof(daEnSnakeSpawner))) daEnSnakeSpawner;
}

/************************************************************
    Helpers
************************************************************/
static inline void GetTileCoords(Vec &pos, u16 &x, u16 &y) {
    x = ((u16)pos.x) & 0xFFF0;
    y = ((u16)(-pos.y)) & 0xFFF0;
}

/************************************************************
    Tile spawn
************************************************************/
void daEnSnakeSpawner::SpawnTile() {
    u16 x, y;
    GetTileCoords(pos, x, y);

    dBgGm_c::instance->placeTile(
        x,
        y,
        currentLayerID,
        0x00F6   // tile ID
    );

    Vec effectPos = {
        (float)x + 8.0f,
        (float)(-y) - 8.0f,
        pos.z
    };

    //SpawnEffect("Wm_en_burst_ss", 0, &effectPos, 0, 0);
}

/************************************************************
    Tile break
************************************************************/
void daEnSnakeSpawner::BreakTile() {
    u16 x, y;
    GetTileCoords(pos, x, y);

    dBgGm_c::instance->placeTile(
        x,
        y,
        currentLayerID,
        0
    );

    Vec effectPos = {
        (float)x + 8.0f,
        (float)(-y) - 8.0f,
        pos.z
    };

    SpawnEffect("Wm_en_burst_ss", 0, &effectPos, 0, 0);

    Vec2 soundPos;
    ConvertStagePositionToScreenPosition(&soundPos, &effectPos);
    SoundPlayingClass::instance2->PlaySoundAtPosition(
        SE_OBJ_BLOCK_BREAK,
        &soundPos,
        0
    );
}

/************************************************************
    Lifecycle
************************************************************/
int daEnSnakeSpawner::onCreate() {
    return 1;
}

int daEnSnakeSpawner::onExecute() {
    return 1;
}

int daEnSnakeSpawner::onDelete() {
    return 1;
}