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

#define MEGA_TOTAL_TIME   (17 * 60)
#define MEGA_FLASH_TIME   (3 * 60)
// -5500 is the standard "behind layer 2" depth in this codebase.
static const float kMegaDrawZOffset = -3000.0f;

extern void StopSoundRelated(int);

// apDebug.cpp debug drawer (sensor hitboxes)
extern int APDebugDraw();

extern "C" void StartStarBGM(void);
extern "C" void StopStarBGM(void);

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
	matrix.translation(pos.x, pos.y, pos.z + kMegaDrawZOffset);
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
	HitMeBaby.yDistToCenter = 45.0f;
	
	// Size
	HitMeBaby.xDistToEdge = 24.0;
	HitMeBaby.yDistToEdge = 50.0f;
	
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

	float halfHeight = 50.0f; // match ActivePhysics yDistToEdge
	float hitboxTop = HitMeBaby.yDistToCenter + HitMeBaby.yDistToEdge;

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
	// Keep above sensor aligned to the hitbox top to avoid clipping into solid tiles.
	aboveSensor.distanceFromCenter = int(ceil(hitboxTop)) << 12;

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

		if(this->weJumped) 
		{
			if(this->speed.y <= 0.0f)
			{
				ShakeScreen(ScreenPositionClass, 0, 1, 0, 0); // add screenshake effect
				static nw4r::snd::StrmSoundHandle megaStompHandle;
				PlaySoundWithFunctionB4(SoundRelatedClass, &megaStompHandle, SFX_MEGA_STOMP, 1);
			}
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
			SpawnEffect("Wm_ob_coinkira", 0, &ac->pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});
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

nw4r::snd::SndAudioMgr* mgr = nw4r::snd::SndAudioMgr::instance;

/* 		Spawn Anim 		*/
void dMegaMario_c::beginState_SpawnScale() {
	timer = 0;
	scale = (Vec){0.4f, 0.4f, 0.4f};
	speed.x = speed.y = 0.0f;
	max_speed.x = max_speed.y = 0.0f;

	static nw4r::snd::StrmSoundHandle megaPowerupHandle;
	PlaySoundWithFunctionB4(SoundRelatedClass, &megaPowerupHandle, SFX_MEGA_POWERUP, 1);
}

static nw4r::snd::StrmSoundHandle s_handle;

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
		nw4r::snd::SndAudioMgr::instance->stopAllBgm(0xf);

		int sID;
		hijackMusicWithSongName("BGM_MEGA", -1, false, 2, 1, &sID);
		PlaySoundWithFunctionB4(SoundRelatedClass, &s_handle, sID, 1);
	
		scale = (Vec){1.5f, 1.5f, 1.5f};
		doStateChange(&StateID_Walk);
	}
}

void dMegaMario_c::endState_SpawnScale() {
}

/* 		Spawn Anim 		*/
void dMegaMario_c::beginState_MegaOutro() {
	timer = 0;

	speed.x = speed.y = 0.0f;
	max_speed.x = max_speed.y = 0.0f;

	s_handle.Stop(30);

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
	{
		daPlayer->clearFlag(0xBB); // make mario visible again
		StartBGMMusic();
	}
}

void dMegaMario_c::endState_MegaOutro() {
}


////////////////////////////////////////////////////////////////////////////////////
// PLAYER STATES

static const float SMB1_MEGA_MAX_RUN_SPEED = 3.6f;
static const float SMB1_MEGA_MAX_WALK_SPEED = 2.25f;
static const float SMB1_MEGA_ACCEL_RUN = 0.080f;
static const float SMB1_MEGA_ACCEL_WALK = 0.055f;
static const float SMB1_MEGA_ACCEL_FAST = 0.072f;
static const float SMB1_MEGA_ACCEL_REVERSE = 0.160f;
static const float SMB1_MEGA_AIR_CONTROL_SCALE = 0.85f;
static const float SMB1_MEGA_FAST_FRICTION_THRESHOLD = 2.6f;
static const float SMB1_MEGA_FAST_JUMP_THRESHOLD = 2.25f;
static const float SMB1_MEGA_JUMP_SPEED_NORMAL = 9.0f;
static const float SMB1_MEGA_JUMP_SPEED_FAST = 9.8f;
static const float SMB1_MEGA_GRAVITY_HELD = 0.18f;
static const float SMB1_MEGA_GRAVITY_RELEASED = 0.62f;
static const float SMB1_MEGA_MAX_FALL_SPEED = -8.2f;
static const int SMB1_MEGA_RUN_TIMER_FRAMES = 10;
static const int SMB1_MEGA_JUMP_HOLD_FRAMES = 24;
static const float SMB1_MEGA_TAP_JUMP_CUT_SPEED = 3.2f;

static inline float megaAbs(float value) {
	return (value < 0.0f) ? -value : value;
}

static inline float megaClamp(float value, float minValue, float maxValue) {
	if (value < minValue)
		return minValue;
	if (value > maxValue)
		return maxValue;
	return value;
}

static bool turning;
static int megaRunTimer;
static int megaJumpHoldTimer;

void daPlBase_c::beginState_MegaMario() {
	this->setFlag(0xBB); // invis
	//this->useDemoControl();
	turning = false;
	megaRunTimer = 0;
	megaJumpHoldTimer = 0;
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
	int inputDir = 0;
	u32 lrHeld = con->heldButtons & (WPAD_LEFT | WPAD_RIGHT);
	if (lrHeld == WPAD_LEFT)
		inputDir = -1;
	else if (lrHeld == WPAD_RIGHT)
		inputDir = 1;

	float absSpeed = megaAbs(megaMario->speed.x);
	float accel = SMB1_MEGA_ACCEL_WALK;
	float speedCap = SMB1_MEGA_MAX_WALK_SPEED;
	bool startedJumpThisFrame = false;
	bool movingWithMomentum = (inputDir != 0) && ((megaMario->speed.x * inputDir) >= 0.0f);
	bool runHeld = (con->heldButtons & WPAD_ONE);

	if (onGround && runHeld && movingWithMomentum) {
		megaRunTimer = SMB1_MEGA_RUN_TIMER_FRAMES;
	}
	else if (megaRunTimer > 0) {
		megaRunTimer--;
	}

	if ((onGround && runHeld && movingWithMomentum) || (megaRunTimer > 0 && movingWithMomentum)) {
		accel = SMB1_MEGA_ACCEL_RUN;
		speedCap = SMB1_MEGA_MAX_RUN_SPEED;
	}

	// Preserve horizontal momentum in air; don't clamp to ground caps mid-jump.
	if (!onGround && absSpeed > speedCap) {
		speedCap = absSpeed;
	}

	if (absSpeed >= SMB1_MEGA_FAST_FRICTION_THRESHOLD) {
		accel = SMB1_MEGA_ACCEL_FAST;
	}

	if (inputDir != 0) {
		float applyAccel = accel;
		bool reversing = ((megaMario->speed.x * inputDir) < 0.0f);

		if (reversing)
			applyAccel = SMB1_MEGA_ACCEL_REVERSE;
		if (!onGround)
			applyAccel *= SMB1_MEGA_AIR_CONTROL_SCALE;

		megaMario->speed.x += applyAccel * (float)inputDir;
		megaMario->speed.x = megaClamp(megaMario->speed.x, -speedCap, speedCap);
		turning = onGround && reversing && (megaAbs(megaMario->speed.x) > 0.08f);

		megaMario->rot.y = (inputDir > 0) ? 0 : 0x8000;
	}
	else {
		float decel = accel;
		if (!onGround)
			decel = 0.0f;

		if (megaMario->speed.x > 0.0f) {
			megaMario->speed.x -= decel;
			if (megaMario->speed.x < 0.0f)
				megaMario->speed.x = 0.0f;
		}
		else if (megaMario->speed.x < 0.0f) {
			megaMario->speed.x += decel;
			if (megaMario->speed.x > 0.0f)
				megaMario->speed.x = 0.0f;
		}

		turning = false;
	}

	if (megaMario->speed.x > 0.0f)
		megaMario->max_speed.x = speedCap;
	else if (megaMario->speed.x < 0.0f)
		megaMario->max_speed.x = -speedCap;
	else
		megaMario->max_speed.x = 0.0f;

	if ((con->nowPressed & WPAD_TWO) && onGround) {
		float jumpSpeed = (absSpeed >= SMB1_MEGA_FAST_JUMP_THRESHOLD) ? SMB1_MEGA_JUMP_SPEED_FAST : SMB1_MEGA_JUMP_SPEED_NORMAL;
		megaMario->speed.y = jumpSpeed;
		megaMario->max_speed.y = jumpSpeed;
		megaMario->texState = 0; // set frame to jump
		megaJumpHoldTimer = SMB1_MEGA_JUMP_HOLD_FRAMES;
		startedJumpThisFrame = true;

		static nw4r::snd::StrmSoundHandle megaJumpHandle;
		PlaySoundWithFunctionB4(SoundRelatedClass, &megaJumpHandle, SFX_MEGA_JUMP, 1);
	}

	if (!onGround) {
		bool holdingJump = (con->heldButtons & WPAD_TWO);
		bool lightGravity = holdingJump && (megaJumpHoldTimer > 0) && (megaMario->speed.y > 0.0f);
		float gravity = lightGravity ? SMB1_MEGA_GRAVITY_HELD : SMB1_MEGA_GRAVITY_RELEASED;

		// SMB-style jump cut: releasing jump while rising quickly trims jump height.
		if (!holdingJump && megaMario->speed.y > SMB1_MEGA_TAP_JUMP_CUT_SPEED) {
			megaMario->speed.y = SMB1_MEGA_TAP_JUMP_CUT_SPEED;
		}

		megaMario->speed.y -= gravity;
		if (megaMario->speed.y < SMB1_MEGA_MAX_FALL_SPEED)
			megaMario->speed.y = SMB1_MEGA_MAX_FALL_SPEED;

		megaMario->max_speed.y = megaMario->speed.y;

		if (megaJumpHoldTimer > 0)
			megaJumpHoldTimer--;
	}
	else if (startedJumpThisFrame) {
		// Preserve jump impulse on the exact takeoff frame.
		megaMario->max_speed.y = megaMario->speed.y;
	}
	else {
		megaJumpHoldTimer = 0;
		megaMario->max_speed.y = 0.0f;
	}

	if(megaMario->collMgr.outputMaybe & (0x15 << direction))
	{
		megaMario->speed.x = 0.0f;
		megaMario->max_speed.x = 0.0f;
	}

	// Movement is handled by the Mega Mario actor's state to avoid double-updating physics.

	// TEXTURE ANIMS FOR MEGA MARIO SPRITE
	if(!onGround || megaMario->weJumped || megaMario->speed.y > 0.0f)
		megaMario->texState = 0;
	else if(turning)
		megaMario->texState = 3;
	else if(megaAbs(megaMario->speed.x) > 0.03f)
		megaMario->texState = 1;
	else
		megaMario->texState = 2;
}
void daPlBase_c::endState_MegaMario() {
}
