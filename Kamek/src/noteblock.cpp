#include <common.h>
#include <game.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

/**
    Note Block

    Actually should be called "Face Block"

    Blocks with faces on them that the player can walk on, and when the player jumps on them, they launch the player into the air.
    The block will slightly dip down when the player is on it, and spring up when the player jumps off of it.
    
    Uses different tileset tiles to show different states and colors:
    - Sleep: Default face (tiles 0xf6, 0xe6, 0xe9)
    - Wake: Different face when player is on it (tiles 0xf8, 0xe8, 0xeb) 
    - Bounce: Special face when player jumps (tiles 0xf7, 0xe7, 0xea)
    
    Colors: Yellow (0xf6-0xf8), Red (0xe6-0xe8), Purple (0xe9-0xeb)
 */

const char *NoteBlockFileList[] = {NULL};

class daEnNoteBlock_c : public daEnBlockMain_c {
    TileRenderer tile;

public:
    static dActor_c* build();

    Physics::Info physicsInfo;

    int onCreate();
    int onDelete();
    int onExecute();

    float originalY;
    bool jumpable;
    bool wasSteppedOn;
    bool playerJumped;
    int timer;
    bool allowVisual;
    
    // Tile animation states
    enum TileState {
        TILE_SLEEP = 0,
        TILE_WAKE = 1,
        TILE_BOUNCE = 2
    };
    TileState currentTileState;
    int bounceTimer; // Timer for bounce state
    bool bounceAnimActive;
    int bounceAnimTimer;
    float bounceBaseY;
    bool stepOffAnimActive;
    int stepOffAnimTimer;
    float stepOffBaseY;
    bool prevOnBlockState; // Track previous frame's onBlock state for step-on detection
    
    // Color variants
    enum Color {
        COLOR_YELLOW = 0,
        COLOR_RED = 1,
        COLOR_PURPLE = 2,
        COLOR_GREEN = 3
    };
    Color selectedColor;

    void blockWasHit(bool isDown);
    void calledWhenUpMoveExecutes();
	void calledWhenDownMoveExecutes();

    float nearestPlayerDistance();

    bool playersGoUp[4];
    bool prevJump[4];
    bool prevOnBlock[4];

    int updatePlayersOnBlock();
    bool playerIsGoUp(int playerID);
    void bouncePlayerWhenJumpedOn(void *player);
    void updateTileState(TileState newState);
    bool applyBounceAnim();
    bool applyStepOffAnim();

    USING_STATES(daEnNoteBlock_c);
    DECLARE_STATE(Spawn);
    DECLARE_STATE(Wait);
    DECLARE_STATE(GoUp);
    DECLARE_STATE(GoDown);
};

CREATE_STATE(daEnNoteBlock_c, Spawn);
CREATE_STATE_E(daEnNoteBlock_c, Wait);
CREATE_STATE_E(daEnNoteBlock_c, GoUp);
CREATE_STATE_E(daEnNoteBlock_c, GoDown);

const SpriteData NoteBlockData = {ProfileId::noteblock, 8, -8, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile NoteBlockProfile(&daEnNoteBlock_c::build, SpriteId::noteblock, &NoteBlockData, ProfileId::noteblock, ProfileId::noteblock, "noteblock", NoteBlockFileList);

// Tile numbers for different states and colors
// Each color has 3 tiles: [SLEEP, BOUNCE, WAKE]
static const u16 TileNumbers[4][3] = {
    // Yellow (COLOR_YELLOW)
    {0xf6, 0xf7, 0xf8},
    
    // Red (COLOR_RED) 
    {0xe6, 0xe7, 0xe8},
    
    // Purple (COLOR_PURPLE)
    {0xe9, 0xea, 0xeb},

    // Green (COLOR_GREEN)
    {0xec, 0xed, 0xee}
};


// checks to see if a player is on the block
int daEnNoteBlock_c::updatePlayersOnBlock() {
    bool anyPlayersGoUp = false;
    
    for (int i = 0; i < 4; i++) {
        this->playersGoUp[i] = false;
        if (dAcPy_c *player = dAcPy_c::findByID(i)) {
            // Slightly wider X range to catch straddling across adjacent tiles
            if(player->pos.x >= pos.x - 14 && player->pos.x <= pos.x + 14) {
                if(player->pos.y >= pos.y - 5 && player->pos.y <= pos.y + 12) {
                    this->playersGoUp[i] = true;
                    anyPlayersGoUp = true;
                    wasSteppedOn = false;
                }
            }
        }
    }
    
    return anyPlayersGoUp ? 1 : 0;
}

bool daEnNoteBlock_c::playerIsGoUp(int playerID) {
    if(this->playersGoUp[playerID]) {
        return true;
    }
    return false;
}

void daEnNoteBlock_c::updateTileState(TileState newState) {
    if (currentTileState != newState) {
        currentTileState = newState;
        tile.tileNumber = TileNumbers[selectedColor][newState];
    }
}

int daEnNoteBlock_c::onCreate() {
    this->allowVisual = false;

    blockInit(pos.y);

    physicsInfo.x1 = -8;
    physicsInfo.y1 = 8;
    physicsInfo.x2 = 8;
    physicsInfo.y2 = -8;

    physicsInfo.otherCallback1 = &daEnBlockMain_c::OPhysicsCallback1;
    physicsInfo.otherCallback2 = &daEnBlockMain_c::OPhysicsCallback2;
    physicsInfo.otherCallback3 = &daEnBlockMain_c::OPhysicsCallback3;

    physics.setup(this, &physicsInfo, 3, currentLayerID);
    physics.flagsMaybe = 0x260;
    physics.callback1 = &daEnBlockMain_c::PhysicsCallback1;
    physics.callback2 = &daEnBlockMain_c::PhysicsCallback2;
    physics.callback3 = &daEnBlockMain_c::PhysicsCallback3;
    physics.addToList();

    // Setup TileRenderer
    //TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    //list->add(&tile);

    // Randomly select a color variant
    int ourColor = (settings & 0xF00) >> 8;
    selectedColor = (Color)ourColor;

    tile.x = pos.x - 8;
    tile.y = -(8+pos.y);
    this->scale = (Vec){0.0, 0.0, 0.0};
    tile.tileNumber = TileNumbers[selectedColor][TILE_SLEEP]; // Start with sleep state

    originalY = pos.y;
    currentTileState = TILE_SLEEP;
    bounceTimer = 0;
    bounceAnimActive = false;
    bounceAnimTimer = 0;
    bounceBaseY = originalY;
    stepOffAnimActive = false;
    stepOffAnimTimer = 0;
    stepOffBaseY = originalY;
    prevOnBlockState = false;

    for(int i = 0; i < 4; i++) {
        this->playersGoUp[i] = false;
        this->prevJump[i] = false;
        this->prevOnBlock[i] = false;
    }

    wasSteppedOn = true;
    playerJumped = false;
    
    
    if(!((this->settings >> 16) & 0xFFF))
    {
        this->scale = (Vec){1.0, 1.0, 1.0};

        TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
        list->add(&tile);

        doStateChange(&daEnNoteBlock_c::StateID_Wait);
    }
    else
        this->scale = (Vec){0.0, 0.0, 0.0};


    return true;
}

int daEnNoteBlock_c::onDelete() {
    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->remove(&tile);
    
    physics.removeFromList();
    return true;
}

int daEnNoteBlock_c::onExecute() {
    physics.update();
    blockUpdate();
    checkZoneBoundaries(0);

    // Update tile position
    tile.setPosition(pos.x-8, -(8+pos.y), pos.z);
    tile.setVars(scale.x);

    acState.execute();

    if(this->scale.x == 0.0)
        doStateChange(&daEnNoteBlock_c::StateID_Spawn);
    
    dStateBase_c* currentState = this->acState.getCurrentState();

    // Update tile state based on block state
    if(bounceAnimActive) {
        updateTileState(TILE_BOUNCE);
    } else if(this->pos.y == originalY) {
        updateTileState(TILE_SLEEP);
    }
    
    int onBlock = updatePlayersOnBlock();
    
    // Detect initial step-on: transition from no players to players on block
    if(onBlock && !prevOnBlockState) {
        // Play sound when player initially steps on
        static nw4r::snd::StrmSoundHandle stepOnHandle;
        PlaySoundWithFunctionB4(SoundRelatedClass, &stepOnHandle, SFX_FACEBLOCK_DOWN, 1);
    }
    prevOnBlockState = (onBlock != 0);
    
    if(onBlock) 
    {
        this->pos.y = originalY - 0.75f;
        if(!bounceAnimActive && currentTileState != TILE_BOUNCE) {
            updateTileState(TILE_WAKE);
        }
    }
    
    // Handle bounce timer
    if(currentTileState == TILE_BOUNCE) {
        bounceTimer++;
        if(bounceTimer > 120) { // Bounce state lasts for 10 frames
            bounceTimer = 0;
            updateTileState(TILE_SLEEP);
        }
    }

    bool bouncedThisFrame = false;
    for (int i = 0; i < 4; i++) {
        if (dAcPy_c *player = dAcPy_c::findByID(i)) {
            //OSReport("Player %d is on block: %d\n", i, playerIsGoUp(i));
            //OSReport("player current state: %s\n", player->states2.getCurrentState()->getName());
            bool curJump = (strcmp(player->states2.getCurrentState()->getName(), "daPlBase_c::StateID_Jump") == 0);
            bool onBlockNow = playerIsGoUp(i);
            // Allow jump to override step-off/other animations if we were on the block last frame
            if((onBlockNow || prevOnBlock[i]) && curJump && player->speed.y >= 0.0f && (!prevJump[i] || !prevOnBlock[i])) {
                updateTileState(TILE_BOUNCE);
                bounceTimer = 0; // Reset bounce timer
                bouncePlayer(player, 4.5f);
                playerJumped = true;
                bounceAnimActive = true;
                bounceAnimTimer = 0;
                bounceBaseY = originalY;
                doStateChange(&StateID_Wait);
                bouncedThisFrame = true;
                wasSteppedOn = true;

                static nw4r::snd::StrmSoundHandle bounceHandler;
                PlaySoundWithFunctionB4(SoundRelatedClass, &bounceHandler, SFX_FACEBLOCK_BOUNCE, 1);
            }
            prevJump[i] = curJump;
            prevOnBlock[i] = onBlockNow;
        } else {
            prevJump[i] = false;
            prevOnBlock[i] = false;
        }
    }

    if(bouncedThisFrame) {
        stepOffAnimActive = false;
        stepOffAnimTimer = 0;
    }
    else if(!onBlock && !wasSteppedOn && !bounceAnimActive && !stepOffAnimActive) {
        wasSteppedOn = true;
        if(!(strcmp(currentState->getName(), "daEnNoteBlock_c::StateID_Wait"))) {
            // Playsound
            static nw4r::snd::StrmSoundHandle stepOffHandle;
            PlaySoundWithFunctionB4(SoundRelatedClass, &stepOffHandle, SFX_FACEBLOCK_UP, 1);
            
            stepOffAnimActive = true;
            stepOffAnimTimer = 0;
            stepOffBaseY = originalY;
            this->pos.y = originalY;
            doStateChange(&StateID_Wait);
        }
    }

    applyBounceAnim();
    applyStepOffAnim();

    return true;
}


dActor_c *daEnNoteBlock_c::build() {
    void *buf = AllocFromGameHeap1(sizeof(daEnNoteBlock_c));
	return new(buf) daEnNoteBlock_c;
}

void daEnNoteBlock_c::blockWasHit(bool isDown) {
	pos.y = initialY;

    doStateChange(&StateID_GoUp);
}

void daEnNoteBlock_c::calledWhenUpMoveExecutes() {
	if (initialY >= pos.y)
		blockWasHit(false);
}

void daEnNoteBlock_c::calledWhenDownMoveExecutes() {
	if (initialY <= pos.y)
		blockWasHit(true);
}

/* Wait State */
void daEnNoteBlock_c::executeState_Wait() {
    int result = blockResult();

	if (result == 0)
		return;

	if (result == 1) {
		doStateChange(&daEnBlockMain_c::StateID_UpMove);
		anotherFlag = 2;
    }
}

/* Going up after going down */
void daEnNoteBlock_c::executeState_GoUp() {
    this->pos.y += playerJumped ? 1.25f : 0.50f;

    float targetHeight = playerJumped ? originalY + 6.0f : originalY + 2.0f;
    if(this->pos.y >= targetHeight) {
        playerJumped = false;
        doStateChange(&StateID_GoDown);
    }
}


/* Going down after going up */
void daEnNoteBlock_c::executeState_GoDown() {
    this->pos.y -= 0.50f;

    if(this->pos.y <= originalY) {
        this->pos.y = originalY;
        doStateChange(&StateID_Wait);
    }
}

void daEnNoteBlock_c::beginState_Spawn() {
    TileRenderer::List *list = dBgGm_c::instance->getTileRendererList(0);
    list->add(&tile);

    this->scale = (Vec){0.0, 0.0, 0.0};
}
void daEnNoteBlock_c::executeState_Spawn()
{
    this->scale.x += 0.2;
    this->scale.y += 0.2;
    this->scale.z += 0.2;

    if(this->scale.x >= 1.0)
    {
        this->scale = (Vec){1.0, 1.0, 1.0};
        doStateChange(&StateID_Wait);
    }
}
void daEnNoteBlock_c::endState_Spawn() {}

bool daEnNoteBlock_c::applyBounceAnim() {
    if (!bounceAnimActive)
        return false;

    struct BounceKey {
        int frame;
        float y;
        float scale;
    };

    static const BounceKey keys[] = {
        {0, 0.0f, 1.0f},
        {4, -3.0f, 1.0f},
        {10, 6.0f, 1.1f},
        {18, -2.0f, 1.0f},
        {23, 1.0f, 1.0f},
        {28, -0.5f, 1.0f},
        {30, 0.0f, 1.0f}
    };

    const int keyCount = sizeof(keys) / sizeof(keys[0]);
    const int lastFrame = keys[keyCount - 1].frame;

    if (bounceAnimTimer >= lastFrame) {
        this->pos.y = bounceBaseY + keys[keyCount - 1].y;
        float s = keys[keyCount - 1].scale;
        this->scale = (Vec){s, s, s};
        bounceAnimActive = false;
        bounceAnimTimer = 0;
        playerJumped = false;
        doStateChange(&StateID_Wait);
        return true;
    }

    const BounceKey *k0 = &keys[0];
    const BounceKey *k1 = &keys[1];
    for (int i = 1; i < keyCount; i++) {
        if (bounceAnimTimer <= keys[i].frame) {
            k0 = &keys[i - 1];
            k1 = &keys[i];
            break;
        }
    }

    float span = (float)(k1->frame - k0->frame);
    float t = span > 0.0f ? (bounceAnimTimer - k0->frame) / span : 0.0f;
    float y = k0->y + (k1->y - k0->y) * t;
    float s = k0->scale + (k1->scale - k0->scale) * t;

    this->pos.y = bounceBaseY + y;
    this->scale = (Vec){s, s, s};
    bounceAnimTimer++;
    return true;
}

bool daEnNoteBlock_c::applyStepOffAnim() {
    if (!stepOffAnimActive)
        return false;

    struct BounceKey {
        int frame;
        float y;
        float scale;
    };

    static const BounceKey keys[] = {
        {0, 0.0f, 1.0f},
        {3, -1.05f, 1.0f},
        {8, 2.10f, 1.0f},
        {14, -0.70f, 1.0f},
        {20, 0.35f, 1.0f},
        {24, -0.175f, 1.0f},
        {26, 0.0f, 1.0f}
    };

    const int keyCount = sizeof(keys) / sizeof(keys[0]);
    const int lastFrame = keys[keyCount - 1].frame;

    if (stepOffAnimTimer >= lastFrame) {
        this->pos.y = stepOffBaseY + keys[keyCount - 1].y;
        float s = keys[keyCount - 1].scale;
        this->scale = (Vec){s, s, s};
        stepOffAnimActive = false;
        stepOffAnimTimer = 0;
        return true;
    }

    const BounceKey *k0 = &keys[0];
    const BounceKey *k1 = &keys[1];
    for (int i = 1; i < keyCount; i++) {
        if (stepOffAnimTimer <= keys[i].frame) {
            k0 = &keys[i - 1];
            k1 = &keys[i];
            break;
        }
    }

    float span = (float)(k1->frame - k0->frame);
    float t = span > 0.0f ? (stepOffAnimTimer - k0->frame) / span : 0.0f;
    float y = k0->y + (k1->y - k0->y) * t;
    float s = k0->scale + (k1->scale - k0->scale) * t;

    this->pos.y = stepOffBaseY + y;
    this->scale = (Vec){s, s, s};
    stepOffAnimTimer++;
    return true;
}

float daEnNoteBlock_c::nearestPlayerDistance() {
	float bestSoFar = 10000.0f;

	for (int i = 0; i < 4; i++) {
		if (dAcPy_c *player = dAcPy_c::findByID(i)) {
			if (strcmp(player->states2.getCurrentState()->getName(), "dAcPy_c::StateID_Balloon")) {
				float thisDist = abs(player->pos.x - pos.x);
				if (thisDist < bestSoFar)
					bestSoFar = thisDist;
			}
		}
	}

	return bestSoFar;
}
