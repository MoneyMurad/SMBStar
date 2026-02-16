/*
    A literal mega mushroom
*/

#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include <actors.h>
#include "boss.h"

const char* MMarc[] = {"obj_mega", NULL};

class dMegaMushroom_c : public daBoss {
    int onCreate();
    int onExecute();
    int onDelete();
    int onDraw();

    mHeapAllocator_c allocator;

    nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile anmFile;
	m3d::mdl_c bodyModel;
	m3d::anmChr_c chrAnimation;
    
    mEf::es2 glow;

    float timer;
    int Baseline;
    u32 cmgr_returnValue;
    
    bool calculateTileCollisions();

    void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	float nearestPlayerDistance();

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
	
	USING_STATES(dMegaMushroom_c);
	DECLARE_STATE(Move);
	DECLARE_STATE(Spawn);
	DECLARE_STATE(Collect);
};

CREATE_STATE(dMegaMushroom_c, Move);
CREATE_STATE(dMegaMushroom_c, Spawn);
CREATE_STATE(dMegaMushroom_c, Collect);

const SpriteData MegaData = {ProfileId::mega, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile MegaProfile(&dMegaMushroom_c::build, SpriteId::mega, &MegaData, ProfileId::mega, ProfileId::mega, "mega", MMarc);

namespace {
	const float kMegaMushroomWalkSpeed = 0.75f;
	const float kMegaMushroomAirGravity = 0.045f;
	const float kMegaMushroomLaunchGravity = 0.22f;
	const float kMegaMushroomMaxFallSpeed = -8.0f;
	const float kMegaMushroomLaunchLerp = 0.06f;
	const float kMegaMushroomGroundSnapLerp = 0.16f;
	const float kMegaMushroomLaunchYDrag = 0.16f;
	const float kMegaMushroomLaunchAdjustFrames = 14.0f;

	inline float absf(float value) {
		return (value < 0.0f) ? -value : value;
	}
}

dActor_c* dMegaMushroom_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dMegaMushroom_c));
	return new(buf) dMegaMushroom_c;
}

void dMegaMushroom_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	dStageActor_c* megaMario = (dStageActor_c*)FindActorByType(mega, 0);
	
    if(megaMario) {
		megaMario->which_player = apOther->owner->which_player;
		megaMario->pos = apOther->owner->pos;
		megaMario->scale = (Vec){0.2f, 0.2f, 0.2f};
		this->Delete(0);
    }
	return;
}
void dMegaMushroom_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	playerCollision(apThis, apOther);
}
void dMegaMushroom_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) 
{
    u16 name = ((dEn_c*)apOther->owner)->name;

	// Ignore all these
	if (name == EN_COIN || name == EN_EATCOIN || name == AC_BLOCK_COIN || name == EN_COIN_JUGEM || name == EN_COIN_ANGLE
		|| name == EN_COIN_JUMP || name == EN_COIN_FLOOR || name == EN_COIN_VOLT || name == EN_COIN_WIND 
		|| name == EN_BLUE_COIN || name == EN_COIN_WATER || name == EN_REDCOIN || name == EN_GREENCOIN
		|| name == EN_JUMPDAI || name == EN_ITEM) 
		{ return; }
	
    pos.x = ((pos.x - ((dEn_c*)apOther->owner)->pos.x) > 0) ? pos.x + 1.5 : pos.x - 1.5;
    
    this->direction ^= 1;
    collMgr.calculateBelowCollisionWithSmokeEffect();

    this->speed.x = (direction) ? -kMegaMushroomWalkSpeed : kMegaMushroomWalkSpeed;
    this->max_speed.x = (direction) ? -kMegaMushroomWalkSpeed : kMegaMushroomWalkSpeed;

	return;
}
bool dMegaMushroom_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dMegaMushroom_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dMegaMushroom_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dMegaMushroom_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dMegaMushroom_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dMegaMushroom_c::collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dMegaMushroom_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dMegaMushroom_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dMegaMushroom_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dMegaMushroom_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dMegaMushroom_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}

void dMegaMushroom_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

void dMegaMushroom_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

int dMegaMushroom_c::onCreate()
{
    allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("obj_mega", "g3d/obj_shroom.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("obj_shroom");
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);

	SetupTextures_MapObj(&bodyModel, 0);

	allocator.unlink();
    
    ActivePhysics::Info HitMeBaby;
	
	// Dist from center
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 16.0;
	
	// Size
	HitMeBaby.xDistToEdge = 16.0;
	HitMeBaby.yDistToEdge = 16.0;
	
	HitMeBaby.category1 = 0x3;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0xFFBAFFFE;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();

    this->direction = 0; // facing left
    this->Baseline = this->pos.y;

    this->scale = (Vec){1.0, 1.0, 1.0};

	spriteSomeRectX = 28.0f;
	spriteSomeRectY = 32.0f;
	_320 = 0.0f;
	_324 = 16.0f;

	static const lineSensor_s below(-5<<12, 5<<12, 0<<12);
	static const pointSensor_s above(0<<12, 12<<12);
	static const lineSensor_s adjacent(6<<12, 9<<12, 6<<12);

	collMgr.init(this, &below, &above, &adjacent);
	collMgr.calculateBelowCollisionWithSmokeEffect();

	cmgr_returnValue = collMgr.isOnTopOfTile();

	this->y_speed_inc = -0.1875;
	doStateChange(&StateID_Move);

    this->onExecute();
    return true;
}

int dMegaMushroom_c::onDraw()
{
    bodyModel.scheduleForDrawing();
    return true;
}

int dMegaMushroom_c::onExecute()
{
    acState.execute();
    updateModelMatrices();
    bodyModel._vf1C();

    //glow.spawn("Wm_ob_keywait", 0, &(Vec){this->pos.x, this->pos.y + 15.0f, this->pos.z - 5.0f}, 0, &(Vec){1.25, 1.25, 1.25});
    
    return true;
}

int dMegaMushroom_c::onDelete()
{
    return true;
}

bool dMegaMushroom_c::calculateTileCollisions()
{
	HandleXSpeed();
	HandleYSpeed();
	doSpriteMovement();
	
	cmgr_returnValue = collMgr.isOnTopOfTile();
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
	float xDelta = pos.x - last_pos.x;
	if (xDelta >= 0.0f)
		direction = 0;
	else
		direction = 1;
	
	if(collMgr.isOnTopOfTile())
	{
		const float targetX = (direction == 1) ? -kMegaMushroomWalkSpeed : kMegaMushroomWalkSpeed;
		speed.y = 0.0f;
		speed.x = speed.x + ((targetX - speed.x) * kMegaMushroomGroundSnapLerp);
		max_speed.x = targetX;
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

// we're on the move
void dMegaMushroom_c::beginState_Move()
{
    this->speed.x = (direction) ? -kMegaMushroomWalkSpeed : kMegaMushroomWalkSpeed;
    this->max_speed.x = (direction) ? -kMegaMushroomWalkSpeed : kMegaMushroomWalkSpeed;

    this->timer = 0;
}
void dMegaMushroom_c::executeState_Move()
{
	this->timer += 1.0f;

	const float targetX = (direction) ? -kMegaMushroomWalkSpeed : kMegaMushroomWalkSpeed;
	if (absf(this->speed.x) > (kMegaMushroomWalkSpeed + 0.01f))
		this->speed.x = this->speed.x + ((targetX - this->speed.x) * kMegaMushroomLaunchLerp);
	this->max_speed.x = targetX;

	if(!collMgr.isOnTopOfTile())
	{
		float launchT = this->timer / kMegaMushroomLaunchAdjustFrames;
		if (launchT > 1.0f)
			launchT = 1.0f;

		const float launchBlend = 1.0f - launchT;
		const float gravity = kMegaMushroomAirGravity + ((kMegaMushroomLaunchGravity - kMegaMushroomAirGravity) * launchBlend);

		// Extra upward drag fades out smoothly so the launch arc doesn't kink into a "V".
		if (this->speed.y > 0.0f && launchBlend > 0.0f)
			this->speed.y -= (kMegaMushroomLaunchYDrag * launchBlend);

		this->speed.y -= gravity;

		if(this->speed.y <= kMegaMushroomMaxFallSpeed)
			this->speed.y = kMegaMushroomMaxFallSpeed;
	}

    bool turn = calculateTileCollisions();
	if(turn) {
		this->direction ^= 1;
        collMgr.calculateBelowCollisionWithSmokeEffect();

        this->speed.x = (direction) ? -kMegaMushroomWalkSpeed : kMegaMushroomWalkSpeed;
        this->max_speed.x = (direction) ? -kMegaMushroomWalkSpeed : kMegaMushroomWalkSpeed;
	}
}
void dMegaMushroom_c::endState_Move() {}

void dMegaMushroom_c::beginState_Collect(){}
void dMegaMushroom_c::executeState_Collect()
{
}
void dMegaMushroom_c::endState_Collect(){}
