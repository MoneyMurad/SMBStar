#include <common.h>
#include <game.h>
#include <profile.h>
#include "path.h"

const char *SnakeBlockNeoFileList[] = {NULL};

class daSnakeBlockNeo_c : public dEnPath_c {
public:
    static dActor_c* build();

    int onCreate();
    int onExecute();
    int onDelete();

private:
    static const int MaxTrailTiles = 1024;
    static const u16 SnakeTileID = 0x00D2;

    struct TrailTile {
        u16 x;
        u16 y;
    };

    TrailTile trail[MaxTrailTiles];
    int trailStart;
    int trailCount;

    int snakeLengthTiles;
    Vec lastPos;
    bool lastPosValid;

    void worldToTile(const Vec &worldPos, u16 &tileX, u16 &tileY);
    bool isSameTile(const Vec &a, const Vec &b);
    void placeTile(const TrailTile &tile, u16 tileID);
    void pushFrontTile(const TrailTile &tile);
    void popRearTile();
};

dActor_c* daSnakeBlockNeo_c::build() {
    void *buf = AllocFromGameHeap1(sizeof(daSnakeBlockNeo_c));
    return new(buf) daSnakeBlockNeo_c;
}

const SpriteData SnakeBlockNeoData = {ProfileId::snakeblockneo, 8, -8, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile SnakeBlockNeoProfile(&daSnakeBlockNeo_c::build, SpriteId::snakeblockneo, &SnakeBlockNeoData, ProfileId::snakeblockneo, ProfileId::snakeblockneo, "snakeblockneo", SnakeBlockNeoFileList);

void daSnakeBlockNeo_c::worldToTile(const Vec &worldPos, u16 &tileX, u16 &tileY) {
    tileX = ((u16)worldPos.x) & 0xFFF0;
    tileY = ((u16)(-worldPos.y)) & 0xFFF0;
}

bool daSnakeBlockNeo_c::isSameTile(const Vec &a, const Vec &b) {
    u16 ax, ay, bx, by;
    worldToTile(a, ax, ay);
    worldToTile(b, bx, by);
    return (ax == bx) && (ay == by);
}

void daSnakeBlockNeo_c::placeTile(const TrailTile &tile, u16 tileID) {
    dBgGm_c::instance->placeTile(tile.x, tile.y, currentLayerID, tileID);
}

void daSnakeBlockNeo_c::pushFrontTile(const TrailTile &tile) {
    int index = (trailStart + trailCount) % MaxTrailTiles;
    trail[index] = tile;
    trailCount++;
    placeTile(tile, SnakeTileID);
}

void daSnakeBlockNeo_c::popRearTile() {
    if (trailCount <= 0)
        return;

    placeTile(trail[trailStart], 0);
    trailStart = (trailStart + 1) % MaxTrailTiles;
    trailCount--;
}


int daSnakeBlockNeo_c::onCreate() {
    waitForPlayer = 0;
    pathID = this->settings >> 0 & 0xFF;
    speed = (float)(this->settings >> 16 & 0xF) / 6.0f;

    if (speed <= 0.0f)
        speed = 1.0f;

    rail = GetRail(pathID);
    if (!rail) {
        OSReport("SNAKEBLOCKNEO: No rail found for pathID %d\n", pathID);
        return false;
    }

    // Bit 24-27: segment count between front and rear. Default to 5 if unset.
    snakeLengthTiles = (settings >> 24) & 0xF;
    if (snakeLengthTiles <= 0)
        snakeLengthTiles = 5;

    trailStart = 0;
    trailCount = 0;
    lastPosValid = false;

    doStateChange(&StateID_Init);
    return true;
}

int daSnakeBlockNeo_c::onExecute() {
    acState.execute();

    updateSnakeBody();

    return true;
}

int daSnakeBlockNeo_c::onDelete() {
    while (trailCount > 0) {
        popRearTile();
    }

    return true;
}

static inline void GetTileCoords(Vec &pos, u16 &x, u16 &y) {
    x = ((u16)pos.x) & 0xFFF0;
    y = ((u16)(-pos.y)) & 0xFFF0;
}

void daSnakeBlockNeo_c::placeSnakeTile(const u16 x, const u16 y) {
    dBgGm_c::instance->placeTile(x, y, currentLayerID, kSnakeTileId);
}

void daSnakeBlockNeo_c::eraseSnakeTile(const u16 x, const u16 y) {
    dBgGm_c::instance->placeTile(x, y, currentLayerID, 0);
}

void daSnakeBlockNeo_c::updateSnakeBody() {
    u16 tileX;
    u16 tileY;
    GetTileCoords(pos, tileX, tileY);

    if (!hasPreviousTile) {
        hasPreviousTile = true;
        previousTileX = tileX;
        previousTileY = tileY;

        placeSnakeTile(tileX, tileY);
        tileQueue[0].x = tileX;
        tileQueue[0].y = tileY;
        queueStart = 0;
        queueCount = 1;
        return;
    }

    while (previousTileX != tileX || previousTileY != tileY) {
        if (previousTileX < tileX) {
            previousTileX += 0x10;
        } else if (previousTileX > tileX) {
            previousTileX -= 0x10;
        } else if (previousTileY < tileY) {
            previousTileY += 0x10;
        } else if (previousTileY > tileY) {
            previousTileY -= 0x10;
        }

        placeSnakeTile(previousTileX, previousTileY);

        const int insertIndex = (queueStart + queueCount) % kMaxSnakeTiles;
        tileQueue[insertIndex].x = previousTileX;
        tileQueue[insertIndex].y = previousTileY;

        if (queueCount < kMaxSnakeTiles) {
            queueCount += 1;
        }

        while (queueCount > length) {
            eraseSnakeTile(tileQueue[queueStart].x, tileQueue[queueStart].y);
            queueStart = (queueStart + 1) % kMaxSnakeTiles;
            queueCount -= 1;
        }
    }
}