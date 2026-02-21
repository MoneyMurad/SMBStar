#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>

const char *LongBlockFileList[] = {"obj_block_long", 0};

class daLongBlock_c : public daEnBlockMain_c {
	Physics::Info physicsInfo;

	int onCreate();
	int onDelete();
	int onExecute();
	int onDraw();
		
	bool isHit;
	int hitCount;
	int timer;
	bool isGroundPound;
	bool spawnedThisBump;
	bool pendingHitState;
	bool pendingItemSpawn;
	bool isInvisible;
	bool forceItemModeB;
	bool forceSinglePowerupInMP;
	u32 pendingItemSettings;
	Vec pendingItemPos;
	int pendingItemSoundId;
	u32 powerup;
	u32 itemSettings;
	u32 coinSettings;
	dStageActor_c *item;
	dStageActor_c *bigCoin;

	float ogPos;
	
	m3d::anmTexSrt_c body;

	Vec coinL;
	Vec coinR;
	
	void calledWhenUpMoveExecutes();
	void calledWhenDownMoveExecutes();

	void spawnContents(bool isDown);
	void spawnConfiguredItemsForPlayers(Vec spawnPos, u32 baseItemSettings);
	void spawnPendingItem();
	void finishHit();

	mHeapAllocator_c allocator;
	nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile usedBlockResFile;
	m3d::mdl_c model;
	m3d::mdl_c usedModel;

	USING_STATES(daLongBlock_c);
	DECLARE_STATE(Wait);
	DECLARE_STATE(Hit);

	public: static dActor_c *build();
};


CREATE_STATE(daLongBlock_c, Wait);
CREATE_STATE(daLongBlock_c, Hit);

const SpriteData LongBlockData = {ProfileId::tripleblock, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile LongBlockProfile(&daLongBlock_c::build, SpriteId::tripleblock, &LongBlockData, ProfileId::tripleblock, ProfileId::tripleblock, "tripleblock", LongBlockFileList);

extern "C" int CheckExistingPowerup(void * Player);

dActor_c  *daLongBlock_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(daLongBlock_c));
	return new(buf) daLongBlock_c;
}

int daLongBlock_c::onCreate() {
	int theme = (this->settings >> 8) & 0xF; // Nybble 10
	const char *blockBrres = "g3d/obj_block_long.brres";
	const char *emptyBlockBrres = "g3d/obj_block_long_empty.brres";
	if (theme == 1)
		blockBrres = "g3d/obj_block_long_blue.brres";
	if (theme == 1)
		emptyBlockBrres = "g3d/obj_block_long_empty_blue.brres";

	allocator.link(-1, GameHeaps[0], 0, 0x20);

	this->resFile.data = getResource("obj_block_long", blockBrres);
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("block_long");
	model.setup(mdl, &allocator, 0x108, 1, 0);
	
	nw4r::g3d::ResAnmTexSrt anmRes = this->resFile.GetResAnmTexSrt("scroll");
	this->body.setup(mdl, anmRes, &this->allocator, 0, 1);
	this->body.bindEntry(&model, anmRes, 0, 0);
	this->model.bindAnim(&this->body, 0.0);

	this->resFile.data = getResource("obj_block_long", emptyBlockBrres);
	nw4r::g3d::ResMdl uMdl = this->resFile.GetResMdl("block_long");
	usedModel.setup(uMdl, &allocator, 0x108, 1, 0);

	//usedBlockResFile.data = getResource("obj_block_long", "g3d/obj_block_long_empty.brres");
	//usedModel.setup(resFile.GetResMdl("block_long"), &allocator, 0, 1, 0);
	
	SetupTextures_MapObj(&model, 0);
	SetupTextures_MapObj(&usedModel, 0);

	allocator.unlink();

	this->ogPos = this->pos.y;

	blockInit(pos.y);

	physicsInfo.x1 = -24;
	physicsInfo.y1 = 16;
	physicsInfo.x2 = 24;
	physicsInfo.y2 = 0;

	physicsInfo.otherCallback1 = &daEnBlockMain_c::OPhysicsCallback1;
	physicsInfo.otherCallback2 = &daEnBlockMain_c::OPhysicsCallback2;
	physicsInfo.otherCallback3 = &daEnBlockMain_c::OPhysicsCallback3;

	physics.setup(this, &physicsInfo, 3, currentLayerID);
	physics.flagsMaybe = 0x260;
	physics.callback1 = &daEnBlockMain_c::PhysicsCallback1;
	physics.callback2 = &daEnBlockMain_c::PhysicsCallback2;
	physics.callback3 = &daEnBlockMain_c::PhysicsCallback3;
	physics.addToList();
	
	this->_68B = 1;
	physics._D8 &= ~0b00101000;

	this->isHit = false;
	this->hitCount = 0;
	this->spawnedThisBump = false;
	this->pendingHitState = false;
	this->pendingItemSpawn = false;
	this->pendingItemSettings = 0;
	this->pendingItemPos = (Vec){0.0f, 0.0f, 0.0f};
	this->pendingItemSoundId = 0;
	this->forceItemModeB = ((this->settings >> 19) & 1); // Nybble 8.1
	this->forceSinglePowerupInMP = ((this->settings >> 18) & 1); // Nybble 8.2

	this->isInvisible = (settings & 0x2000);
	
	this->pos.z = 200.0f;
	
	doStateChange(&daLongBlock_c::StateID_Wait);
	this->onExecute();
	
	return true;
}


int daLongBlock_c::onDelete() {
	physics.removeFromList();

	return true;
}


int daLongBlock_c::onExecute() {
	model._vf1C();
	acState.execute();
	physics.update();
	this->body.process();

	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);
	
	if(!this->isHit)
	{
		model.setDrawMatrix(matrix);
		model.setScale(&scale);
		model.calcWorld(false);
	}
	else
	{
		//this->pos.z = 9900;
		usedModel.setDrawMatrix(matrix);
		usedModel.setScale(&scale);
		usedModel.calcWorld(false);
	}
	
	this->blockUpdate();

	//if (acState.getCurrentState()->isEqual(&StateID_Wait)) {
	//	checkZoneBoundaries(0);
	//}

	return true;
}


int daLongBlock_c::onDraw() {
	if(!this->isHit)
	{
		if(!this->isInvisible)
			model.scheduleForDrawing();
	}
	else
	{
		usedModel.scheduleForDrawing();
	}

	return true;
}

// AVERT YOUR EYES
void daLongBlock_c::spawnContents(bool isDown) {
	pos.y = initialY;
	
	// Get our powerup from reggie
	this->powerup = (this->settings >> 28 & 0xF);
	
	// Big coin custom stuffs
	if(powerup > 10)
	{
		//				   type														                   gravity 
		u32 bcSettings = ((powerup - 10) | (0 << 2) | (0 << 4) | (0 << 6) | (0 << 8) | (0 << 10) | (2 << 12) | (0 << 14));
		Vec bcPos = (Vec){this->pos.x, ((isDown) ? this->pos.y - 5 : this->pos.y + 25), this->pos.z};
			
		bigCoin = dStageActor_c::create(bigcoin, bcSettings, &bcPos, 0, 0);
		
		if(!isDown) { 
			bigCoin->speed.y = 6.0f; 
			bigCoin->speed.x = 2.5f; 
		}
		
		// We are OUT of here after the bump finishes
		pendingHitState = true;
		return;
	}
	
	// Powerups: 0 = small 1 = big 2 = fire 3 = mini 4 = prop 5 = peng 6 = ice
	int p = CheckExistingPowerup(dAcPy_c::findByID(this->playerID));
	
	/*	Powerup Settings	*/
	static u32 ingamePowerupIDs[] = {0x0,0x1,0x2,0x7,0x9,0xE,0x11,0x15,0x19,0x6,0x2}; //Mushroom, Star, Coin, 1UP, Fire Flower, Ice Flower, Penguin, Propeller, Mini Shroom, Hammer, 10 Coin
	this->powerup = ingamePowerupIDs[this->powerup];
	
	/*	Sound logic	*/
	nw4r::snd::SoundHandle handle;
	PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_GET_COIN, 1);
	
	pendingItemSoundId = 0;
	if((powerup != 0x2 && powerup != 0x15) || p == 0 || p == 3) 
		pendingItemSoundId = SE_OBJ_ITEM_APPEAR; //Item sound

	if(powerup == 0x15 && p != 0) 
		pendingItemSoundId = SE_OBJ_ITEM_PRPL_APPEAR; //Propeller sound
	
	/*	Coin/powerup positions and settings logic	*/
	u32 itemMode = this->forceItemModeB ? 0xB : 0x4;
	this->itemSettings = (itemMode << 24) | (powerup << 0) | (this->playerID + 8 << 16);
	this->coinSettings = 0 | (0x2 << 0) | (((isDown) ? 3 : 2) << 18) | (4 << 9) | (2 << 10) | (this->playerID + 8 << 16);
		
	this->coinL = (Vec) {this->pos.x - 16, this->pos.y + ((isDown) ? 1 : -2), this->pos.z};
	this->coinR = (Vec) {this->pos.x + 16, this->pos.y + ((isDown) ? 1 : -2), this->pos.z};
	Vec itemPos = (Vec){this->pos.x, this->pos.y + ((isDown) ? 1 : -2), this->pos.z};
	
	/*	Create our actors	*/
	if (powerup == 0x2) {
		spawnConfiguredItemsForPlayers(itemPos, itemSettings);
	} else {
		pendingItemSpawn = true;
		pendingItemSettings = itemSettings;
		pendingItemPos = itemPos;
	}
	dStageActor_c::create(EN_ITEM, coinSettings, &coinL, 0, 0);
	dStageActor_c::create(EN_ITEM, coinSettings, &coinR, 0, 0);
	// non-coin items are spawned after the bump ends
	
	// 10 coin
	if(powerup == 0x2 && ((this->settings >> 28 & 0xF) == 10))
		this->hitCount++;
	else
		this->hitCount = 10;
	
	if(this->hitCount >= 10)
		pendingHitState = true;
}

void daLongBlock_c::spawnConfiguredItemsForPlayers(Vec spawnPos, u32 baseItemSettings) {
	u8 activePlayerIDs[4];
	int activePlayerCount = 0;
	u32 powerupType = (baseItemSettings & 0xFF);

	for (int i = 0; i < 4; i++) {
		if (dAcPy_c::findByID(i) != 0)
			activePlayerIDs[activePlayerCount++] = i;
	}

	bool useMultiPowerupSpread = !this->forceSinglePowerupInMP && (activePlayerCount > 1);
	int spawnCount = useMultiPowerupSpread ? activePlayerCount : 1;

	for (int i = 0; i < spawnCount; i++) {
		u32 itemSettingsForSpawn = baseItemSettings;

		if (useMultiPowerupSpread) {
			// In MP-per-player mode, force EN_ITEM mode B like Big Brick does.
			itemSettingsForSpawn = (0xB << 24) | powerupType;
			itemSettingsForSpawn |= ((activePlayerIDs[i] + 8) << 16);
		}

		dStageActor_c *spawnedItem = dStageActor_c::create(EN_ITEM, itemSettingsForSpawn, &spawnPos, 0, 0);
		if (spawnedItem == 0)
			continue;

		spawnedItem->pos.z = 100.0f;

		if (useMultiPowerupSpread) {
			float sx, sy;

			if (activePlayerCount == 2) {
				sx = (i == 0) ? -1.9f : 1.9f;
				sy = 2.9f;
			} else if (activePlayerCount == 3) {
				static const float spreadX[3] = {-1.9f, 0.0f, 1.9f};
				static const float spreadY[3] = {2.9f, 3.6f, 2.9f};
				sx = spreadX[i];
				sy = spreadY[i];
			} else {
				static const float spreadX[4] = {-1.9f, -0.65f, 0.65f, 1.9f};
				static const float spreadY[4] = {2.8f, 3.4f, 3.4f, 2.8f};
				int idx = (i < 4) ? i : 3;
				sx = spreadX[idx];
				sy = spreadY[idx];
			}

			sx += ((float)GenerateRandomNumber(11) - 5.0f) * 0.04f;
			sy += ((float)GenerateRandomNumber(7)) * 0.05f;

			// Keep MP-spawned items from clipping into the block on their first frame.
			spawnedItem->pos.x += (sx * 1.2f);
			spawnedItem->pos.y += 10.0f;

			spawnedItem->speed.x = sx;
			spawnedItem->speed.y = sy;
		}

		this->item = spawnedItem;
	}
}

void daLongBlock_c::finishHit() {
	this->pos.y = this->ogPos;
	
	if (pendingHitState)
		doStateChange(&StateID_Hit);
	else
		doStateChange(&StateID_Wait);

	pendingHitState = false;
}

void daLongBlock_c::spawnPendingItem() {
	if (!pendingItemSpawn)
		return;

	spawnConfiguredItemsForPlayers(pendingItemPos, pendingItemSettings);

	if (pendingItemSoundId != 0) {
		nw4r::snd::SoundHandle handle;
		PlaySoundWithFunctionB4(SoundRelatedClass, &handle, pendingItemSoundId, 1);
	}

	pendingItemSpawn = false;
	pendingItemSettings = 0;
	pendingItemSoundId = 0;
}

void daLongBlock_c::calledWhenUpMoveExecutes() {
	if (initialY >= pos.y)
	{
		if (spawnedThisBump)
			spawnPendingItem();
		finishHit();
	}
}

void daLongBlock_c::calledWhenDownMoveExecutes() {
	if (initialY <= pos.y)
	{
		if (spawnedThisBump)
			spawnPendingItem();
		finishHit();
	}
}

void daLongBlock_c::beginState_Wait() {
	spawnedThisBump = false;
	pendingHitState = false;
	pendingItemSpawn = false;
	pendingItemSettings = 0;
	pendingItemSoundId = 0;
}

void daLongBlock_c::endState_Wait() {
}

void daLongBlock_c::executeState_Wait() {
	int result = blockResult();

	if (result == 0)
		return;

	this->isInvisible = false; // make it so we can see the block after hitting it

	if (result == 1) {
		spawnContents(false);
		spawnedThisBump = true;
		doStateChange(&daEnBlockMain_c::StateID_UpMove);
		anotherFlag = 2;
		isGroundPound = false;
	} else {
		spawnContents(true);
		spawnedThisBump = true;
		doStateChange(&daEnBlockMain_c::StateID_DownMove);
		anotherFlag = 1;
		isGroundPound = true;
	}
}


void daLongBlock_c::beginState_Hit() {
	this->isHit = true;
}
void daLongBlock_c::executeState_Hit() {
}
void daLongBlock_c::endState_Hit() {
}
