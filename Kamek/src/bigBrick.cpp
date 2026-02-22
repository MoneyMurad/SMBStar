#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>

const char *BigBrickArc[] = {"obj_brick_big", 0};

class daBigBrick_c : public daEnBlockMain_c {
	Physics::Info physicsInfo;

	int onCreate();
	int onDelete();
	int onExecute();
	int onDraw();

	int hitCount;
	bool pendingBreak;
	bool isBroken;
	u32 configuredPowerup;
	u32 configuredCoinCount;
	u32 configuredActorId;
	bool forceItemModeB;
	bool forceSinglePowerupInMP;

	void calledWhenUpMoveExecutes();
	void calledWhenDownMoveExecutes();

	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	void finishBump();
	void spawnRewards(bool isDown);
	void spawnConfiguredCoins(bool isDown);

	mHeapAllocator_c allocator;
	nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile anmFile;
	m3d::anmChr_c chrAnimation;
	m3d::mdl_c model;

	USING_STATES(daBigBrick_c);
	DECLARE_STATE(Wait);

	public: static dActor_c *build();
};

CREATE_STATE(daBigBrick_c, Wait);

const SpriteData BigBrickData = {ProfileId::bigbrick, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile BigBrickProfile(&daBigBrick_c::build, SpriteId::bigbrick, &BigBrickData, ProfileId::bigbrick, ProfileId::bigbrick, "bigbrick", BigBrickArc);

extern "C" int CheckExistingPowerup(void * Player);

dActor_c *daBigBrick_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(daBigBrick_c));
	return new(buf) daBigBrick_c;
}

void daBigBrick_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->model, anmChr, unk);
	this->model.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

int daBigBrick_c::onCreate() {
	allocator.link(-1, GameHeaps[0], 0, 0x20);

	this->resFile.data = getResource("obj_brick_big", "g3d/obj_brick_big.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("obj_brick_big");
	model.setup(mdl, &allocator, 0x108, 1, 0);
	SetupTextures_MapObj(&model, 0);

	this->anmFile.data = getResource("obj_brick_big", "g3d/obj_brick_big.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("damage1");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);

	allocator.unlink();

	blockInit(pos.y);

	physicsInfo.x1 = -8;
	physicsInfo.y1 = 32;
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

	this->hitCount = 0;
	this->pendingBreak = false;
	this->isBroken = false;
	this->scale = (Vec){1.0f, 1.0f, 1.0f};
	this->configuredPowerup = ((this->settings >> 28) & 0xF); // Nybble 5
	this->configuredCoinCount = ((this->settings >> 20) & 0xFF); // Nybbles 6-7
	this->configuredActorId = ((this->settings >> 4) & 0xFFF); // Nybbles 9-11
	this->forceItemModeB = ((this->settings >> 19) & 1); // Nybble 8.1
	this->forceSinglePowerupInMP = ((this->settings >> 18) & 1); // Nybble 8.2

	doStateChange(&daBigBrick_c::StateID_Wait);
	this->onExecute();

	return true;
}

int daBigBrick_c::onDelete() {
	physics.removeFromList();
	return true;
}

int daBigBrick_c::onExecute() {
	model._vf1C();
	acState.execute();
	physics.update();

	matrix.translation(pos.x + 8.0f, pos.y + 16.0f, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	this->scale = (Vec){1.57f, 1.57f, 1.57f};
	model.setDrawMatrix(matrix);
	model.setScale(&scale);
	model.calcWorld(false);

	if (this->isBroken && this->chrAnimation.isAnimationDone())
		this->Delete(1);

	this->blockUpdate();
	return true;
}

int daBigBrick_c::onDraw() {
	model.scheduleForDrawing();
	return true;
}

void daBigBrick_c::spawnConfiguredCoins(bool isDown) {
	for (u32 i = 0; i < this->configuredCoinCount; i++) {
		Vec coinPos = (Vec){this->pos.x, this->pos.y + (isDown ? -8.0f : 20.0f), this->pos.z};
		dStageActor_c *coin = (dStageActor_c*)CreateActor(403, 0, coinPos, 0, 0);

		if (coin != 0) {
			float sx = ((float)GenerateRandomNumber(33) - 16.0f) * 0.08f;
			float sy;

			// Favor upward momentum while still allowing occasional downward shots.
			if (GenerateRandomNumber(100) < 80)
				sy = 1.6f + ((float)GenerateRandomNumber(24) * 0.18f);
			else
				sy = -0.8f - ((float)GenerateRandomNumber(9) * 0.2f);

			coin->speed.x = sx;
			coin->speed.y = sy;
		}
	}
}

void daBigBrick_c::spawnRewards(bool isDown) {
	this->spawnConfiguredCoins(isDown);

	Vec itemPos = (Vec){this->pos.x, this->pos.y + (isDown ? -8.0f : 20.0f), this->pos.z};

	// Value 11 is a special "Actor" mode: spawn configured actor ID instead of EN_ITEM.
	if (this->configuredPowerup == 11) {
		if (this->configuredActorId > 0 && this->configuredActorId < ProfileId::Num) {
			dStageActor_c *spawnedActor = dStageActor_c::create((Actors)this->configuredActorId, 0, &itemPos, 0, 0);
			if (spawnedActor != 0)
				spawnedActor->pos.z = 100.0f;
		}
		return;
	}

	if (this->configuredPowerup > 10)
		return;

	static u32 ingamePowerupIDs[] = {0x0, 0x1, 0x2, 0x7, 0x9, 0xE, 0x11, 0x15, 0x19, 0x6, 0x2};
	u32 powerup = ingamePowerupIDs[this->configuredPowerup];
	u32 itemMode = this->forceItemModeB ? 0xB : 0x4;
	u32 itemSettings = (itemMode << 24) | powerup;
	u8 activePlayerIDs[4];
	int activePlayerCount = 0;

	for (int i = 0; i < 4; i++) {
		if (dAcPy_c::findByID(i) != 0)
			activePlayerIDs[activePlayerCount++] = i;
	}

	bool useMultiPowerupSpread = !this->forceSinglePowerupInMP && (activePlayerCount > 1);
	int spawnCount = useMultiPowerupSpread ? activePlayerCount : 1;

	for (int i = 0; i < spawnCount; i++) {
		u32 itemSettingsForSpawn = itemSettings;

		if (useMultiPowerupSpread)
			itemSettingsForSpawn |= ((activePlayerIDs[i] + 8) << 16);

		dStageActor_c *item = dStageActor_c::create(EN_ITEM, itemSettingsForSpawn, &itemPos, 0, 0);
		if (item == 0)
			continue;

		float sx, sy;

		if (!useMultiPowerupSpread) {
			// Single-item fling matches coin range/distribution.
			sx = ((float)GenerateRandomNumber(33) - 16.0f) * 0.08f;
			if (GenerateRandomNumber(100) < 80)
				sy = 1.6f + ((float)GenerateRandomNumber(24) * 0.18f);
			else
				sy = -0.8f - ((float)GenerateRandomNumber(9) * 0.2f);
		} else {
			// Multiplayer: wide angle separation so each player's item launches distinctly.
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

			// Small variance while preserving separation.
			sx += ((float)GenerateRandomNumber(11) - 5.0f) * 0.04f;
			sy += ((float)GenerateRandomNumber(7)) * 0.05f;
		}

		item->pos.z = 100.0f;
		item->speed.x = sx;
		item->speed.y = sy;
	}

	dAcPy_c *player = dAcPy_c::findByID(this->playerID);
	int currentPowerup = CheckExistingPowerup(player);

	nw4r::snd::SoundHandle handle;
	if ((powerup != 0x2 && powerup != 0x15) || currentPowerup == 0 || currentPowerup == 3)
		PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_ITEM_APPEAR, 1);
	else if (powerup == 0x15 && currentPowerup != 0)
		PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_ITEM_PRPL_APPEAR, 1);
}

void daBigBrick_c::finishBump() {
	pos.y = initialY;

	if (this->pendingBreak) {
		this->pendingBreak = false;
		this->isBroken = true;
	}

	doStateChange(&StateID_Wait);
}

void daBigBrick_c::calledWhenUpMoveExecutes() {
	if (initialY >= pos.y)
		finishBump();
}

void daBigBrick_c::calledWhenDownMoveExecutes() {
	if (initialY <= pos.y)
		finishBump();
}

void daBigBrick_c::beginState_Wait() {
}

void daBigBrick_c::endState_Wait() {
}

void daBigBrick_c::executeState_Wait() {
	int result = blockResult();
	if (result == 0 || this->isBroken)
		return;

	if (this->hitCount == 0) {
		bindAnimChr_and_setUpdateRate("damage1", 1, 0.0f, 1.0f);
	} else if (this->hitCount == 1) {
		bindAnimChr_and_setUpdateRate("damage2", 1, 0.0f, 1.0f);
	} else {
		bindAnimChr_and_setUpdateRate("break", 1, 0.0f, 1.0f);
		this->pendingBreak = true;
		physics.removeFromList();
		this->spawnRewards(result != 1);
	}

	this->hitCount++;
	nw4r::snd::SoundHandle handle;
	PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_BLOCK_BREAK, 1);

	if (result == 1) {
		doStateChange(&daEnBlockMain_c::StateID_UpMove);
		anotherFlag = 2;
		isGroundPound = false;
	} else {
		doStateChange(&daEnBlockMain_c::StateID_DownMove);
		anotherFlag = 1;
		isGroundPound = true;
	}
}
