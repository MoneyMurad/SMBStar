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
    u8 groupID;
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

    u8 getGroupID();
    bool hasStoppedMoving();

    int type;
    int getType();
};

class daPathTileBlockB_c : public dEnPath_c {
    TileRenderer movingTile;
    Physics physics;
    u8 groupID;
    u32 linkedBlockAID;
    bool hasLinkedBlockA;

public:
    static dActor_c* build();

    int onCreate();
    int onDelete();
    int onExecute();

    void setupCollision();
    void updateMovingTile();
    void deleteTileIfCovered();
    daPathTileBlockA_c *findLinkedBlockA();

    bool startCountdown;
    int blocks;
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

u8 daPathTileBlockA_c::getGroupID() {
    return groupID;
}

int daPathTileBlockA_c::getType() {
    return type;
}

bool daPathTileBlockA_c::hasStoppedMoving() {
    return (acState.getCurrentState() == &dEnPath_c::StateID_Done);
}

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

    this->groupID = this->settings >> 0 & 0b11111111;
    this->type = ((this->settings >> 28) & 0xF);
    
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

    if(acState.getCurrentState() == &dEnPath_c::StateID_Done)
    {   
        if(this->type == 1)
        {
            this->currentNodeNum = 0;
            doStateChange(&dEnPath_c::StateID_Init);
        }
    }

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

    this->groupID = this->settings >> 0 & 0b11111111;
    this->linkedBlockAID = 0;
    this->hasLinkedBlockA = false;
    this->startCountdown = false;

    setupCollision();

    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->add(&movingTile);

    updateMovingTile();
    doStateChange(&dEnPath_c::StateID_Init);
    return true;
}

daPathTileBlockA_c *daPathTileBlockB_c::findLinkedBlockA() {
    fBase_c *search = 0;
    while ((search = fBase_c::searchByBaseType(0x2, search))) {
        dStageActor_c *actor = (dStageActor_c*)search;

        if (actor->name != ProfileId::pathTileBlockA)
            continue;

        daPathTileBlockA_c *candidate = (daPathTileBlockA_c*)actor;
        if (candidate->getGroupID() != groupID)
            continue;

        linkedBlockAID = candidate->id;
        hasLinkedBlockA = true;
        return candidate;
    }

    return 0;
}


int daPathTileBlockB_c::onDelete() {
    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->remove(&movingTile);

    physics.removeFromList();

    return true;
}

int daPathTileBlockB_c::onExecute() {
    daPathTileBlockA_c *linkedA = findLinkedBlockA();
    if (linkedA && linkedA->hasStoppedMoving()) {
        if (acState.getCurrentState() != &dEnPath_c::StateID_Done)
        {
            if(linkedA->getType() == 0) // stops when moving
                doStateChange(&dEnPath_c::StateID_Done);
        }
    }

    if(linkedA && acState.getCurrentState() == &dEnPath_c::StateID_Done)
    {
        if(linkedA->getType() == 1)
        {
            this->currentNodeNum = 0;
            doStateChange(&dEnPath_c::StateID_Init);
        }
    }

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
    if (pExistingTile /* && *pExistingTile == kPathTileNumber*/) {
        dBgGm_c::instance->placeTile(worldX, worldY, currentLayerID, 0);
    }

    if(this->startCountdown)
        this->blocks++;  // measures the amount of blocks we are for when we respawn
}
