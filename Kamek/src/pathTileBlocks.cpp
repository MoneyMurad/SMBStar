#include <common.h>
#include <game.h>
#include <profile.h>
#include "path.h"

const char *PathTileBlockAFileList[] = {NULL};
const char *PathTileBlockBFileList[] = {NULL};

static const u16 kPathTileNumber = 0xD2;

class daPathTileBlockA_c;
class daPathTileBlockB_c;

static void TileBlockPhysCB1(void *one, dStageActor_c *two) { }
static void TileBlockPhysCB2(void *one, dStageActor_c *two) { }
static void TileBlockPhysCB3(void *one, dStageActor_c *two, bool unk) { }

class daPathTileBlockA_c : public dEnPath_c {
    TileRenderer movingTile;
    Physics physics;
    u16 lastWorldX;
    u16 lastWorldY;
    bool hasLastTile;

public:
    static dActor_c* build();

    int onCreate();
    int onDelete();
    int onExecute();

    void setupCollision();
    void updateMovingTile();
    void layStaticTile();
};

class daPathTileBlockB_c : public dEnPath_c {
    TileRenderer movingTile;
    Physics physics;

public:
    static dActor_c* build();

    int onCreate();
    int onDelete();
    int onExecute();

    void setupCollision();
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

void daPathTileBlockA_c::setupCollision() {
    physics.setup(this, -8.0f, 8.0f, 8.0f, -8.0f,
        (void*)&TileBlockPhysCB1, (void*)&TileBlockPhysCB2, (void*)&TileBlockPhysCB3,
        1, currentLayerID, 0);

    physics.flagsMaybe = 0x260;
    physics.addToList();
}

void daPathTileBlockB_c::setupCollision() {
    physics.setup(this, -8.0f, 8.0f, 8.0f, -8.0f,
        (void*)&TileBlockPhysCB1, (void*)&TileBlockPhysCB2, (void*)&TileBlockPhysCB3,
        1, currentLayerID, 0);

    physics.flagsMaybe = 0x260;
    physics.addToList();
}

int daPathTileBlockA_c::onCreate() {
    this->currentNodeNum = (this->settings >> 8 & 0b11111111);

    this->movingTile.tileNumber = kPathTileNumber;
    this->scale = (Vec){1.0f, 1.0f, 1.0f};
    this->hasLastTile = false;

    setupCollision();

    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->add(&movingTile);

    updateMovingTile();
    layStaticTile();

    doStateChange(&dEnPath_c::StateID_Init);
    return true;
}

int daPathTileBlockA_c::onDelete() {
    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->remove(&movingTile);

    physics.removeFromList();

    return true;
}

int daPathTileBlockA_c::onExecute() {
    acState.execute();
    physics.update();

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

    setupCollision();

    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->add(&movingTile);

    updateMovingTile();
    doStateChange(&dEnPath_c::StateID_Init);
    return true;
}

int daPathTileBlockB_c::onDelete() {
    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->remove(&movingTile);

    physics.removeFromList();

    return true;
}

int daPathTileBlockB_c::onExecute() {
    acState.execute();
    physics.update();

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
