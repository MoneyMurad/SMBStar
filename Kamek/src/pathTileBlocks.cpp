#include <common.h>
#include <game.h>
#include <profile.h>
#include "path.h"

const char *PathTileBlockAFileList[] = {NULL};
const char *PathTileBlockBFileList[] = {NULL};

static const u16 kPathTileNumber = 0xD2;

class daPathTileBlockA_c : public dEnPath_c {
    TileRenderer movingTile;
    u16 lastWorldX;
    u16 lastWorldY;
    bool hasLastTile;

public:
    static dActor_c* build();

    int onCreate();
    int onDelete();
    int onExecute();

    void updateMovingTile();
    void layStaticTile();
};

class daPathTileBlockB_c : public dEnPath_c {
    TileRenderer movingTile;

public:
    static dActor_c* build();

    int onCreate();
    int onDelete();
    int onExecute();

    void updateMovingTile();
    void deleteTileIfCovered();
};

dActor_c* daPathTileBlockA_c::build() {
    void *buf = AllocFromGameHeap1(sizeof(daPathTileBlockA_c));
    return new(buf) daPathTileBlockA_c;
}

dActor_c* daPathTileBlockB_c::build() {
    void *buf = AllocFromGameHeap1(sizeof(daPathTileBlockB_c));
    return new(buf) daPathTileBlockB_c;
}

const SpriteData PathTileBlockAData = {ProfileId::pathTileBlockA, 8, -8, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
const SpriteData PathTileBlockBData = {ProfileId::pathTileBlockB, 8, -8, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile PathTileBlockAProfile(&daPathTileBlockA_c::build, SpriteId::pathTileBlockA, &PathTileBlockAData, ProfileId::pathTileBlockA, ProfileId::pathTileBlockA, "pathTileBlockA", PathTileBlockAFileList);
Profile PathTileBlockBProfile(&daPathTileBlockB_c::build, SpriteId::pathTileBlockB, &PathTileBlockBData, ProfileId::pathTileBlockB, ProfileId::pathTileBlockB, "pathTileBlockB", PathTileBlockBFileList);

int daPathTileBlockA_c::onCreate() {
    this->movingTile.tileNumber = kPathTileNumber;
    this->scale = (Vec){1.0f, 1.0f, 1.0f};
    this->hasLastTile = false;

    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->add(&movingTile);

    this->currentNodeNum = (this->settings >> 8 & 0b11111111);

    updateMovingTile();
    layStaticTile();

    doStateChange(&dEnPath_c::StateID_Init);
    return true;
}

int daPathTileBlockA_c::onDelete() {
    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->remove(&movingTile);
    return true;
}

int daPathTileBlockA_c::onExecute() {
    acState.execute();

    updateMovingTile();
    layStaticTile();

    return true;
}

void daPathTileBlockA_c::updateMovingTile() {
    movingTile.setPosition(pos.x - 8.0f, -(8.0f + pos.y), pos.z);
    movingTile.setVars(scale.x);
}

void daPathTileBlockA_c::layStaticTile() {
    u16 worldX = ((u16)pos.x) & 0xFFF0;
    u16 worldY = ((u16)(-pos.y)) & 0xFFF0;

    float tileCenterX = (float)worldX + 8.0f;
    float tileCenterY = -((float)worldY) - 8.0f;

    // Wait until the actor is centered on the tile
    if (abs(pos.x - tileCenterX) > 0.5f || abs(pos.y - tileCenterY) > 0.5f)
        return;

    if (hasLastTile && worldX == lastWorldX && worldY == lastWorldY)
        return;

    u16 *pExistingTile = dBgGm_c::instance->getPointerToTile(worldX, worldY, currentLayerID);
    if (!pExistingTile || *pExistingTile != kPathTileNumber) {
        dBgGm_c::instance->placeTile(worldX, worldY, currentLayerID, 0x80D2);
    }

    lastWorldX = worldX;
    lastWorldY = worldY;
    hasLastTile = true;
}

int daPathTileBlockB_c::onCreate() {
    this->currentNodeNum = (this->settings >> 8 & 0b11111111);

    this->movingTile.tileNumber = kPathTileNumber;
    this->scale = (Vec){1.0f, 1.0f, 1.0f};

    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->add(&movingTile);

    updateMovingTile();
    doStateChange(&dEnPath_c::StateID_Init);
    return true;
}

int daPathTileBlockB_c::onDelete() {
    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->remove(&movingTile);
    return true;
}

int daPathTileBlockB_c::onExecute() {
    acState.execute();

    updateMovingTile();
    deleteTileIfCovered();

    return true;
}

void daPathTileBlockB_c::updateMovingTile() {
    movingTile.setPosition(pos.x - 8.0f, -(8.0f + pos.y), pos.z);
    movingTile.setVars(scale.x);
}

void daPathTileBlockB_c::deleteTileIfCovered() {
    u16 worldX = ((u16)pos.x) & 0xFFF0;
    u16 worldY = ((u16)(-pos.y)) & 0xFFF0;

    float tileCenterX = (float)worldX + 8.0f;
    float tileCenterY = -((float)worldY) - 8.0f;

    if (abs(pos.x - tileCenterX) > 0.5f || abs(pos.y - tileCenterY) > 0.5f)
        return;

    u16 *pExistingTile = dBgGm_c::instance->getPointerToTile(worldX, worldY, currentLayerID);
    if (pExistingTile && *pExistingTile == kPathTileNumber) {
        dBgGm_c::instance->placeTile(worldX, worldY, currentLayerID, 0);
    }
}
