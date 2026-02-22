#include <common.h>
#include <game.h>
#include <profile.h>
#include "path.h"
#include "boss.h"

const char *SnakeBlockNeoFileList[] = {NULL};

class daSnakeBlockNeo_c : public dEnPath_c {
public:
    static dActor_c* build();

    int onCreate();
    int onExecute();
    int onDelete();

    // Spawned noteblock
    dStageActor_c* spawnedBlock;
    u32 spawnedBlockID;
    
    // Settings
    bool isFront;  // true = front, false = rear
    
    // Position tracking for front block
    Vec prevPos;
    bool hasPrevPos;
};

dActor_c* daSnakeBlockNeo_c::build() {
    void *buf = AllocFromGameHeap1(sizeof(daSnakeBlockNeo_c));
    return new(buf) daSnakeBlockNeo_c;
}

const SpriteData SnakeBlockNeoData = {ProfileId::snakeblockneo, 8, -8, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile SnakeBlockNeoProfile(&daSnakeBlockNeo_c::build, SpriteId::snakeblockneo, &SnakeBlockNeoData, ProfileId::snakeblockneo, ProfileId::snakeblockneo, "snakeblockneo", SnakeBlockNeoFileList);

int daSnakeBlockNeo_c::onCreate() {
    // Initialize path system
    waitForPlayer = 0;
    pathID = this->settings >> 0 & 0b11111111;
    speed = (float)(this->settings >> 16 & 0b1111);
    speed /= 6;
    
    if (speed == 0) {
        speed = 1.0f; // Default speed
    }
    
    rail = GetRail(pathID);
    if (!rail) {
        OSReport("SNAKEBLOCKNEO: No rail found for pathID %d\n", pathID);
        return false;
    }
    
    course = dCourseFull_c::instance->get(GetAreaNum());
    loop = rail->flags & 2;
    
    // Determine if this is front or rear block
    // Bit 24-27: 0 = front, non-zero = rear
    isFront = ((settings & 0xF000000) >> 24) == 0;
    
    // Set starting node: rear = 0, front = 1
    currentNodeNum = isFront ? 1 : 0;
    
    // Spawn the noteblock at current position
    spawnedBlock = (dStageActor_c*)CreateActor(773, 0, pos, 0, 0);
    if (spawnedBlock) {
        spawnedBlockID = spawnedBlock->id;
    }

    OSReport("Our ID %d\n", spawnedBlockID);
    
    // Initialize position tracking for front block
    hasPrevPos = false;
    prevPos = pos;
    
    // Start path following
    doStateChange(&StateID_Init);
    
    return true;
}

int daSnakeBlockNeo_c::onExecute() {
    // Execute path following (this updates pos)
    acState.execute();
    
    // Update spawned block position to match our position
    if (spawnedBlock) {
        dStageActor_c* ac = (dStageActor_c*)fBase_c::search(spawnedBlockID);
        if (ac) {
            ac->pos = pos;
        } else {
            // Respawn if deleted
            spawnedBlock = (dStageActor_c*)CreateActor(773, 0, pos, 0, 0);
            if (spawnedBlock) spawnedBlockID = spawnedBlock->id;
        }
    }
    
    // For front block: check if moved exactly one block (16 pixels) and spawn noteblock
    if (isFront) {
        if (abs(pos.x - prevPos.x) >= 16.0f || abs(pos.y - prevPos.y) >= 16.0f) {
            // Spawn a static noteblock at current position
            OSReport("Hi yes we should be spawning!\n");
            CreateActor(773, 0, pos, 0, 0);

            prevPos = pos;
        }
    }
    // If this is the REAR block, delete any tile it passes over
    else {
        // Search for actors near our position
        fBase_c* search = 0;
        while ((search = fBase_c::searchByBaseType(0x2, search))) { // 0x2 = stage actor
            dStageActor_c* ac = (dStageActor_c*)search;

            // Only care about tile actors (ID 773)
            if (ac->name == 773) {
                // Check if we're basically on top of it
                if (ac->pos.x == pos.x && ac->pos.y == pos.y) {

                    ac->Delete(1);
                }
            }
        }
    }
    
    return true;
}

int daSnakeBlockNeo_c::onDelete() {
    // Delete spawned block
    if (spawnedBlock) {
        dStageActor_c* ac = (dStageActor_c*)fBase_c::search(spawnedBlockID);
        if (ac) ac->Delete(1);
    }
    
    return true;
}

