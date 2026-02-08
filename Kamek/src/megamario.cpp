/*
    A literal mega mario
*/

#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

const char* MegaMarioArc[] = {"obj_mega", NULL};

#define MEGA_TOTAL_TIME   (20 * 60)
#define MEGA_FLASH_TIME   (3 * 60)

// apDebug.cpp debug drawer (sensor hitboxes)
extern int APDebugDraw();
	
class dMegaMario_c : public daBoss {
    int onCreate();
    int onExecute();
    int onDelete();
    int onDraw();

    mHeapAllocator_c allocator;

    nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile anmFile;
	m3d::mdl_c bodyModel;
	m3d::anmChr_c chrAnimation;
	nw4r::g3d::ResAnmTexPat anmPat;
	m3d::anmTexPat_c patAnimation;
    
    mEf::es2 glow;

	lineSensor_s belowSensor;
	lineSensor_s adjacentSensor;
	lineSensor_s aboveSensor;

    int timer;
	int flashTimer;
    int Baseline;
	public: int texState;
	int walkTimer;
    u32 cmgr_returnValue;
	bool weAreReadyForAction;
	float originalX;
	int speedFrameIncrease;
	bool wasOnGround;
	bool canBreakThisLanding;
	bool weJumped;
    int fazcheck;
	bool markInvisible;
	bool finishingUp;
	int animTimer;

	Vec originalPos;

	bool calculateTileCollisions();
	void collectNearbyPickups();

	void texPat_bindAnimChr_and_setUpdateRate(const char* name);
    void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	float nearestPlayerDistance();
	void calculateRemainingTime();

	void playerCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	
	bool collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther);

    public: static dActor_c *build();
    public: dAcPy_c *daPlayer;
    public: void setdaPlayer(dAcPy_c *player);
	
	USING_STATES(dMegaMario_c);
	DECLARE_STATE(Idle);
	DECLARE_STATE(Walk);
	DECLARE_STATE(SpawnScale);
	DECLARE_STATE(MegaOutro);
};

CREATE_STATE(dMegaMario_c, Idle);
CREATE_STATE(dMegaMario_c, Walk);
CREATE_STATE(dMegaMario_c, SpawnScale);
CREATE_STATE(dMegaMario_c, MegaOutro);

static inline bool isMegaPickup(u16 name) {
	return (
		name == EN_COIN || name == EN_EATCOIN || name == AC_BLOCK_COIN || name == EN_COIN_JUGEM || name == EN_COIN_ANGLE ||
		name == EN_COIN_JUMP || name == EN_COIN_FLOOR || name == EN_COIN_VOLT || name == EN_COIN_WIND ||
		name == EN_BLUE_COIN || name == EN_COIN_WATER || name == EN_REDCOIN || name == EN_GREENCOIN ||
		name == EN_JUMPDAI || name == EN_ITEM || name == EN_STAR_COIN
	);
}

// Add state to the player
CREATE_STATE(daPlBase_c, MegaMario);

const SpriteData MegaMarioData = {ProfileId::megamario, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile MegaMarioProfile(&dMegaMario_c::build, SpriteId::megamario, &MegaMarioData, ProfileId::megamario, ProfileId::megamario, "megamario", MegaMarioArc);

extern "C" void *ShakeScreen(void *, unsigned int, unsigned int, unsigned int, unsigned int);
extern "C" void* ScreenPositionClass;
extern int MaxCoins;

static inline void awardCoins(int playerIndex, int amount) {
	if (playerIndex < 0 || playerIndex > 3)
		return;

	Player_Coins[playerIndex] += amount;

	nw4r::snd::SoundHandle handle;
	if (Player_Coins[playerIndex] > MaxCoins) {
		PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_SYS_100COIN_ONE_UP, 1);

		if (Player_Lives[playerIndex] >= 99)
			Player_Lives[playerIndex] = 99;
		else
			Player_Lives[playerIndex] += 1;

		int difference = Player_Coins[playerIndex] - MaxCoins;
		Player_Coins[playerIndex] = (difference - 1);
	}

	PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_GET_COIN, 1);
}

dActor_c* dMegaMario_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dMegaMario_c));
	return new(buf) dMegaMario_c;
}

void dMegaMario_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	/*daPlBase_c *player = (daPlBase_c*)apOther->owner;

	if(this->scale.x == 1.4f)
	{
		player->states2.setState(&daPlBase_c::StateID_MegaMario);
		doStateChange(&StateID_SpawnScale);
	}*/

	return;
}
void dMegaMario_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	playerCollision(apThis, apOther);
}
void dMegaMario_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) 
{
    u16 name = ((dActor_c*)apOther->owner)->name;

	OSReport("COLLISION with %d\n", ((dActor_c*)apOther->owner)->name);

	if(name == mega || name == bird) return;

	if (isMegaPickup(name)) {
			dEn_c* pickup = (dEn_c*)apOther->owner;
			if (pickup && daPlayer) {
				pickup->playerCollision(apOther, &daPlayer->aPhysics);
			}
			return;
		}

	// Get our enemy
	dEn_c* enemy = (dEn_c*)apOther->owner;
	if (!enemy) return;

	// Pretend megamario hit them with mario's star-man
	enemy->collisionCat3_StarPower(apOther, &daPlayer->aPhysics);
	return;

	return;
}
bool dMegaMario_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dMegaMario_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dMegaMario_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dMegaMario_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dMegaMario_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dMegaMario_c::collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dMegaMario_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dMegaMario_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dMegaMario_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dMegaMario_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dMegaMario_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}

void dMegaMario_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

void dMegaMario_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

void dMegaMario_c::texPat_bindAnimChr_and_setUpdateRate(const char* name) {
	this->anmPat = this->resFile.GetResAnmTexPat(name);
	this->patAnimation.bindEntry(&this->bodyModel, &anmPat, 0, 1);
	this->bodyModel.bindAnim(&this->patAnimation);
}

void dMegaMario_c::setdaPlayer(dAcPy_c *player) {
	if (!player)
		return;

	daPlayer = player;
	this->which_player = player->which_player;
}

int dMegaMario_c::onCreate()
{
	this->texState = 2;
	this->direction = 1;
	this->speedFrameIncrease = 7;
	this->markInvisible = false;
	this->animTimer = 0;

	originalPos = this->pos;

    allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("obj_mega", "g3d/obj_mario.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("obj_mario");
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);

	this->anmPat = this->resFile.GetResAnmTexPat("mega_action");
	this->patAnimation.setup(mdl, anmPat, &this->allocator, 0, 1);
	this->patAnimation.bindEntry(&this->bodyModel, &anmPat, 0, 1);
	this->patAnimation.setFrameForEntry(1, 0);
	this->patAnimation.setUpdateRateForEntry(0.0f, 0);
	this->bodyModel.bindAnim(&this->patAnimation);

	allocator.unlink();
    
	this->scale = (Vec){1.4f, 1.4f, 1.4f};

    ActivePhysics::Info HitMeBaby;
	
	// Dist from center
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 55.0;
	
	// Size
	HitMeBaby.xDistToEdge = 24.0;
	HitMeBaby.yDistToEdge = 60.0;
	
	HitMeBaby.category1 = 0x3;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0xFFBAFFFE;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();

	wasOnGround = false;
	weJumped = false;
	canBreakThisLanding = false;

	// STOLEN
	
	spriteSomeRectX = 28.0f;
	spriteSomeRectY = 32.0f;
	_320 = 0.0f;
	_324 = 16.0f;

	static const lineSensor_s below(-5<<12, 5<<12, 0<<12);
	static const pointSensor_s above(0<<12, 12<<12);
	static const lineSensor_s adjacent(6<<12, 9<<12, 6<<12);

	collMgr.init(this, &below, &above, &adjacent);
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
	int size = 20; // MegaMario width in tiles

	this->fazcheck = 0;

	// BELOW SENSOR
	belowSensor.flags =
		SENSOR_LINE |
		SENSOR_BREAK_BLOCK |
		SENSOR_BREAK_BRICK |
		SENSOR_10000000;

	belowSensor.lineA = -size << 12;
	belowSensor.lineB =  size << 12;
	belowSensor.distanceFromCenter = -15;

	int halfHeight = 60; // match ActivePhysics yDistToEdge

	// SIDE SENSOR
	adjacentSensor.flags =
		SENSOR_LINE |
		SENSOR_BREAK_BLOCK |
		SENSOR_BREAK_BRICK |
		SENSOR_10000000;

	// Vertical line spanning full body height
	adjacentSensor.lineA =  0 << 12;
	adjacentSensor.lineB =  100 << 12;

	// Horizontal offset from center (how far to the side)
	adjacentSensor.distanceFromCenter = 22 << 12; // match xDistToEdge

	// ABOVE SENSOR
	aboveSensor.flags =
		SENSOR_LINE |
		SENSOR_BREAK_BLOCK |
		SENSOR_ACTIVATE_QUESTION |
		SENSOR_BREAK_BRICK |
		SENSOR_10000000;
	
	aboveSensor.lineA = -size << 12;
	aboveSensor.lineB =  size << 12;
	aboveSensor.distanceFromCenter = int(ceil(halfHeight * 1.75)) << 12;

	// Register sensors
	collMgr.init(this, &belowSensor, &aboveSensor, &adjacentSensor);

	daPlayer = dAcPy_c::findByID(0);
	if (daPlayer) {
		this->which_player = daPlayer->which_player;
	}
	//cmgr_returnValue = collMgr.isOnTopOfTile();
	
	this->finishingUp = false;
	this->timer = 0;

	OSReport("It's time to get BIG");
	this->onExecute();
	return true;
}

int dMegaMario_c::onDraw()
{
    if(!markInvisible) bodyModel.scheduleForDrawing();
    return true;
}

int dMegaMario_c::onExecute() {
	dAcPy_c *player = (dAcPy_c*)GetSpecificPlayerActor(this->which_player);
	if (player && player != daPlayer)
		setdaPlayer(player);
	if (!daPlayer)
		return true;

	acState.execute();
	updateModelMatrices();
	bodyModel._vf1C();
	
	if(this->scale.x == 0.2f) 
	{
		daPlayer->states2.setState(&daPlBase_c::StateID_MegaMario);
		doStateChange(&StateID_SpawnScale);
	}
	
	if(this->scale.x != 1.5f) return;
	APDebugDraw();

	if (strcmp(daPlayer->states2.getCurrentState()->getName(),
           "daPlBase_c::MegaMario") != 0) {
			daPlayer->states2.setState(&daPlBase_c::StateID_MegaMario);
	}

	Vec bindPos = this->pos;
	bindPos.y += 50.0f;
	daPlayer->pos = bindPos;
	
	bool onGround = collMgr.isOnTopOfTile();

	if(this->speed.y >= 8.3f)
		this->weJumped = true;

	// Detect landing from a jump
	if (!this->wasOnGround && onGround) {
		canBreakThisLanding = true;

		if(this->weJumped) {
			ShakeScreen(ScreenPositionClass, 0, 1, 0, 0); // add screenshake effect
			static nw4r::snd::StrmSoundHandle megaStompHandle;
			PlaySoundWithFunctionB4(SoundRelatedClass, &megaStompHandle, SFX_MEGA_STOMP, 1);
		}

		this->weJumped = false;
	}
	else
		canBreakThisLanding = false;

	wasOnGround = onGround;

	// Enable brick breaking ONLY for the landing frame
	if (canBreakThisLanding) {
		belowSensor.flags |= SENSOR_BREAK_BLOCK | SENSOR_BREAK_BRICK;
	}
	else {
		belowSensor.flags &= ~(SENSOR_BREAK_BLOCK | SENSOR_BREAK_BRICK);
	}

	// Texture anims

	if(this->texState == 0)
	{
		this->patAnimation.setFrameForEntry(1, 0);
	}
	else if(this->texState == 1)
	{
		if(this->animTimer >= speedFrameIncrease)
		{
			if(this->walkTimer < 5) {
				this->patAnimation.setFrameForEntry(this->walkTimer, 0);
				this->walkTimer++;
				this->animTimer = 0;
			} else {
				this->animTimer = 0;
				this->patAnimation.setFrameForEntry(3, 0);
				this->walkTimer = 2;
			}

			if(this->speed.x >= 3.0f || this->speed.x <= -3.0f)
				speedFrameIncrease = 3;
			else if(this->speed.x >= 1.5f || this->speed.x <= -1.5f)
				speedFrameIncrease = 5;
			else
				speedFrameIncrease = 7;
		}
		this->animTimer++;
	}
	else if(this->texState == 3)
		this->patAnimation.setFrameForEntry(5, 0);
	else
		this->patAnimation.setFrameForEntry(0, 0);

	// Above collision is handled in calculateTileCollisions after movement updates.
	collectNearbyPickups();

	return true;
}

int dMegaMario_c::onDelete()
{
    return true;
}

bool dMegaMario_c::calculateTileCollisions()
{
	HandleXSpeed();
	HandleYSpeed();
	doSpriteMovement();
	
	cmgr_returnValue = collMgr.isOnTopOfTile();
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
	//if (speed.y > 0.0f)
		aboveSensor.flags |= SENSOR_BREAK_BLOCK | SENSOR_ACTIVATE_QUESTION | SENSOR_HIT_OR_BREAK_BRICK;
	//else
	//	aboveSensor.flags &= ~(SENSOR_BREAK_BLOCK | SENSOR_ACTIVATE_QUESTION | SENSOR_HIT_OR_BREAK_BRICK);

	if (collMgr.calculateAboveCollision(collMgr.outputMaybe) && speed.y > 0.0f)
		speed.y = 0.0f;

	float xDelta = pos.x - last_pos.x;
	if(xDelta == 0 && !collMgr.isOnTopOfTile())
	{	this->speed.x = 0.0f;
		this->max_speed.x = 0.0f;}
	
	if (xDelta >= 0.0f)
		direction = 0;
	else
		direction = 1;
	
	if(collMgr.isOnTopOfTile())
	{
		speed.y = 0.0f;
		max_speed.x = (direction == 1) ? -speed.x : speed.x;
	}
	else
		x_speed_inc = 0.0f;
	
	collMgr.calculateAdjacentCollision(0);
	
	if (collMgr.outputMaybe & (0x15 << direction)) {
		/*if (collMgr.isOnTopOfTile()) {
			isBouncing = true;
		}*/
		return true;
	}
	return false;
}

void dMegaMario_c::collectNearbyPickups()
{
	if (!daPlayer)
		return;

	const ActivePhysics::Info &megaInfo = this->aPhysics.info;
	float megaLeft = this->pos.x + megaInfo.xDistToCenter - megaInfo.xDistToEdge;
	float megaRight = this->pos.x + megaInfo.xDistToCenter + megaInfo.xDistToEdge;
	float megaTop = this->pos.y + megaInfo.yDistToCenter + megaInfo.yDistToEdge;
	float megaBottom = this->pos.y + megaInfo.yDistToCenter - megaInfo.yDistToEdge;

	ActivePhysics *ap = ActivePhysics::globalListHead;
	while (ap) {
		dStageActor_c *ac = ap->owner;
		if (!ac || ac == this) {
			ap = ap->listPrev;
			continue;
		}

		if (!isMegaPickup(ac->name)) {
			ap = ap->listPrev;
			continue;
		}

		const ActivePhysics::Info &info = ap->info;
		float otherLeft = ac->pos.x + info.xDistToCenter - info.xDistToEdge;
		float otherRight = ac->pos.x + info.xDistToCenter + info.xDistToEdge;
		float otherTop = ac->pos.y + info.yDistToCenter + info.yDistToEdge;
		float otherBottom = ac->pos.y + info.yDistToCenter - info.yDistToEdge;

		if (megaRight < otherLeft || megaLeft > otherRight) {
			ap = ap->listPrev;
			continue;
		}
		if (megaTop < otherBottom || megaBottom > otherTop) {
			ap = ap->listPrev;
			continue;
		}

		dEn_c *pickup = (dEn_c*)ac;
		pickup->playerCollision(ap, &daPlayer->aPhysics);

		ap = ap->listPrev;
	}

	int playerIndex = Player_ID[daPlayer->which_player];

	static const u16 manualCoinNames[] = {
		EN_COIN, EN_COIN_JUGEM, EN_COIN_JUMP, EN_COIN_FLOOR,
		EN_COIN_VOLT, EN_COIN_WIND, EN_COIN_WATER, EN_COIN_ANGLE
	};

	for (int i = 0; i < (int)(sizeof(manualCoinNames) / sizeof(manualCoinNames[0])); i++) {
		fBase_c *fb = 0;
		while ((fb = fBase_c::search((Actors)manualCoinNames[i], fb))) {
			dStageActor_c *ac = (dStageActor_c*)fb;
			if (!ac || ac == this)
				continue;

			if (ac->aPhysics.isLinkedIntoList)
				continue;

			float otherHalfX = (ac->aPhysics.info.xDistToEdge != 0.0f) ? ac->aPhysics.info.xDistToEdge : 8.0f;
			float otherHalfY = (ac->aPhysics.info.yDistToEdge != 0.0f) ? ac->aPhysics.info.yDistToEdge : 8.0f;
			otherHalfX *= ac->scale.x;
			otherHalfY *= ac->scale.y;

			float otherLeft = ac->pos.x - otherHalfX;
			float otherRight = ac->pos.x + otherHalfX;
			float otherTop = ac->pos.y + otherHalfY;
			float otherBottom = ac->pos.y - otherHalfY;

			if (megaRight < otherLeft || megaLeft > otherRight)
				continue;
			if (megaTop < otherBottom || megaBottom > otherTop)
				continue;

			awardCoins(playerIndex, 1);
			ac->Delete(1);
		}
	}
}

/* 		Idle 		*/
void dMegaMario_c::beginState_Idle() 
{}
void dMegaMario_c::executeState_Idle() 
{}
void dMegaMario_c::endState_Idle() 
{}

/* 		Walk 		*/
void dMegaMario_c::beginState_Walk() 
{
	this->timer = 0;
	
	this->max_speed.x = (direction) ? -0.5f : 0.5f;
	this->speed.x = (direction) ? -0.5f : 0.5f;
	
	this->max_speed.y = -6.0;
	this->speed.y = -6.0;
	this->y_speed_inc = -0.375;
	
	this->texState = 0;
}

void dMegaMario_c::executeState_Walk() 
{
	this->timer++;

	bool turn = calculateTileCollisions();

    int timeLeft = MEGA_TOTAL_TIME - this->timer;

    // --- FLASHING when 15 seconds remain ---
    if (timeLeft <= MEGA_FLASH_TIME) {
        this->flashTimer++;

        // flash frames
        if ((this->flashTimer % 10) == 0) {
            this->markInvisible = !this->markInvisible;
        }
    }

    // times up!
    if (this->timer >= MEGA_TOTAL_TIME) {
        this->markInvisible = false;
		this->finishingUp = true;
        this->doStateChange(&StateID_MegaOutro);
    }
}
void dMegaMario_c::endState_Walk() 
{}

/* 		Spawn Anim 		*/
void dMegaMario_c::beginState_SpawnScale() {
	timer = 0;
	scale = (Vec){0.4f, 0.4f, 0.4f};
	speed.x = speed.y = 0.0f;
	max_speed.x = max_speed.y = 0.0f;

	static nw4r::snd::StrmSoundHandle megaPowerupHandle;
	PlaySoundWithFunctionB4(SoundRelatedClass, &megaPowerupHandle, SFX_MEGA_POWERUP, 1);
}

void dMegaMario_c::executeState_SpawnScale() {
	timer++;
	this->patAnimation.setFrameForEntry(0, 0);

	Vec bindPos = this->pos;
	bindPos.y += 50.0f;
	daPlayer->pos = bindPos;

	if (timer == 7) {
		scale = (Vec){0.7f, 0.7f, 0.7f};
	}
	else if (timer == 14) {
		scale = (Vec){0.5f, 0.5f, 0.5f};
	}
	else if (timer == 21) {
		scale = (Vec){1.0f, 1.0f, 1.0f};
	}
	else if (timer == 28) {
		scale = (Vec){0.7f, 0.7f, 0.7f};
	}
	else if (timer == 35) {
		scale = (Vec){1.49f, 1.49f, 1.49f};
	}
	else if (timer == 42) {
		scale = (Vec){1.5f, 1.5f, 1.5f};
		doStateChange(&StateID_Walk); // transition to normal behavior
	}
}

void dMegaMario_c::endState_SpawnScale() {
}

/* 		Spawn Anim 		*/
void dMegaMario_c::beginState_MegaOutro() {
	timer = 0;

	speed.x = speed.y = 0.0f;
	max_speed.x = max_speed.y = 0.0f;

	static nw4r::snd::StrmSoundHandle megaPowerdownHandle;
	PlaySoundWithFunctionB4(SoundRelatedClass, &megaPowerdownHandle, SFX_MEGA_POWERDOWN, 1);
}

void dMegaMario_c::executeState_MegaOutro() {
	timer++;

	if(this->timer < 49)
		daPlayer->pos = this->pos;

	if (timer == 7) {
		scale = (Vec){1.49f, 1.49f, 1.49f};
	}
	else if (timer == 14) {
		scale = (Vec){0.7f, 0.7f, 0.7f};
	}
	else if (timer == 21) {
		scale = (Vec){1.0f, 1.0f, 1.0f};
	}
	else if (timer == 28) {
		scale = (Vec){0.5f, 0.5f, 0.5f};
	}
	else if (timer == 35) {
		scale = (Vec){0.7f, 0.7f, 0.7f};
	}
	else if(timer == 49)
	{
		this->pos = this->originalPos;
		//SpawnEffect("Wm_ob_starcoinget", 0, &daPlayer->pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});
		
		daPlayer->speed.x = 0.0f;
		daPlayer->max_speed.x = 0.0f;
		daPlayer->states2.setState(&dPlayer::StateID_Walk);
	}

	if(this->timer == 53)
		daPlayer->clearFlag(0xBB); // make mario visible again
}

void dMegaMario_c::endState_MegaOutro() {
}


////////////////////////////////////////////////////////////////////////////////////
// PLAYER STATES

#define MAX_MEGA_SPEED 3.0f
#define SPD_INCREMENT 1.2f
#define SPD_TURN 0.1f

Vec oldPos;
bool jump;
bool turning;
int turnHoldFrames;
bool jumpCutApplied;

void daPlBase_c::beginState_MegaMario() {
	this->setFlag(0xBB); // invis
	//this->useDemoControl();
	jump = false;
	turning = false;
	turnHoldFrames = 0;
	jumpCutApplied = false;
}
void daPlBase_c::executeState_MegaMario() {
	dMegaMario_c* megaMario = (dMegaMario_c*)FindActorByType(mega, 0);

	if(!megaMario)
		return;

	if(megaMario->scale.x != 1.5f)
		return;

	if(megaMario->finishingUp) return;
	
	someFlightRelatedFunction(this); // Handles x movement apparently
	Remocon* con = GetRemoconMng()->controllers[this->settings % 4];
	
	bool onGround = megaMario->collMgr.isOnTopOfTile();
	float accelScaleBase = onGround ? 1.0f : 0.15f;
	float baseMax = MAX_MEGA_SPEED;
	float boostMax = MAX_MEGA_SPEED * 1.5f;
	float targetMax = (con->heldButtons & WPAD_ONE) ? boostMax : baseMax;
	float currentCap = (megaMario->max_speed.x < 0.0f) ? -megaMario->max_speed.x : megaMario->max_speed.x;
	if (currentCap == 0.0f)
		currentCap = baseMax;
	float speedCap = currentCap + (targetMax - currentCap) * 0.01f;
	
	if(con->heldButtons & WPAD_LEFT) {
		float accelScale = accelScaleBase;

		if(megaMario->speed.x >= MAX_MEGA_SPEED) {
			if (!turning)
				turnHoldFrames = 8;
			turning = true;
		}

		if (turning) {
			if (turnHoldFrames > 0)
				turnHoldFrames--;
			else if (megaMario->speed.x <= 0.0f)
				turning = false;
			float turnLerp = 0.02f;
			megaMario->speed.x = megaMario->speed.x + (0.0f - megaMario->speed.x) * turnLerp;
		} else {
			float accel = (turning) ? SPD_TURN : SPD_INCREMENT;
			megaMario->speed.x -= accel * accelScale;
		}
		if (!turning) {
			if (megaMario->speed.x <= -speedCap)
				megaMario->speed.x = -speedCap;
			megaMario->max_speed.x = -speedCap;
		}

		megaMario->rot.y = 0x8000;
	}

	if(con->heldButtons & WPAD_RIGHT) {
		float accelScale = accelScaleBase;

		if(megaMario->speed.x <= -MAX_MEGA_SPEED) {
			if (!turning)
				turnHoldFrames = 8;
			turning = true;
		}

		if (turning) {
			if (turnHoldFrames > 0)
				turnHoldFrames--;
			else if (megaMario->speed.x >= 0.0f)
				turning = false;
			float turnLerp = 0.02f;
			megaMario->speed.x = megaMario->speed.x + (0.0f - megaMario->speed.x) * turnLerp;
		} else {
			float accel = (turning) ? SPD_TURN : SPD_INCREMENT;
			megaMario->speed.x += accel * accelScale;
		}
		if (!turning) {
			if (megaMario->speed.x >= speedCap)
				megaMario->speed.x = speedCap;
			megaMario->max_speed.x = speedCap;
		}

		megaMario->rot.y = 0;
	}

	if(con->nowPressed & WPAD_TWO && megaMario->collMgr.isOnTopOfTile())
	{
		megaMario->speed.y = 8.5f;
		megaMario->max_speed.y = 8.5f;
		megaMario->texState = 0; // set frame to jump
		jumpCutApplied = false;

		static nw4r::snd::StrmSoundHandle megaJumpHandle;
		PlaySoundWithFunctionB4(SoundRelatedClass, &megaJumpHandle, SFX_MEGA_JUMP, 1);
	}

	if(!megaMario->collMgr.isOnTopOfTile())
	{
		bool holdingJump = (con->heldButtons & WPAD_TWO);
		if (megaMario->speed.y > 0.0f && !holdingJump && !jumpCutApplied) {
			if (megaMario->speed.y > 3.5f)
				megaMario->speed.y = 3.5f;
			if (megaMario->max_speed.y > 3.5f)
				megaMario->max_speed.y = 3.5f;
			jumpCutApplied = true;
		}

		megaMario->speed.y -= 0.15f;
		megaMario->max_speed.y -= 0.35f;
		if (megaMario->max_speed.y < -7.0f)
		{
			megaMario->max_speed.y = -7.5f;
		}
	}

	if(megaMario->collMgr.outputMaybe & (0x15 << direction))
	{
		megaMario->speed.x = 0.0f;
		megaMario->max_speed.x = 0.0f;
	}

	if (!(con->heldButtons & (WPAD_LEFT | WPAD_RIGHT))) {
		float lerpFactor = onGround ? 0.08f : 0.03f;
		float target = 0.0f;
		megaMario->speed.x = megaMario->speed.x + (target - megaMario->speed.x) * lerpFactor;
		if (megaMario->speed.x > -0.1f && megaMario->speed.x < 0.1f)
			megaMario->speed.x = 0.0f;
	}

	// Movement is handled by the Mega Mario actor's state to avoid double-updating physics.

	// TEXTURE ANIMS FOR MEGA MARIO SPRITE
	if(megaMario->weJumped)
		megaMario->texState = 0;
	else if(turning)
		megaMario->texState = 3;
	else if(megaMario->speed.x != 0)
		megaMario->texState = 1;
	else
		megaMario->texState = 2;

	if(megaMario->speed.y > 0)
		megaMario->texState = 0;
}
void daPlBase_c::endState_MegaMario() {
}
