#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

const char* RATFileList[] = {"kakibo", NULL};

class dGoombrat_c : public dEn_c {
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
	
	int timer;
	int walkPatTimer;
	int lookdownPatTimer;
	int texState;
	int Baseline;
	u32 cmgr_returnValue;
	
	void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	void texPat_bindAnimChr_and_setUpdateRate(const char* name);
	
	void playerCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	
	bool collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther);
	
	bool calculateTileCollisions();
	bool willWalkOntoSuitableGround(float delta = 2.5);
	void _vf148();
	void _vf14C();
	bool CreateIceActors();
	void _vf5C();

	float nearestPlayerDistance();

	bool loaded;
	
	public: static dActor_c *build();
	
	USING_STATES(dGoombrat_c);
	DECLARE_STATE(Wait);
	DECLARE_STATE(Walk);
	DECLARE_STATE(Turn);
	DECLARE_STATE(Die);
	DECLARE_STATE(DieFalling);
};

CREATE_STATE_E(dGoombrat_c, Wait);
CREATE_STATE(dGoombrat_c, Walk);
CREATE_STATE(dGoombrat_c, Turn);
CREATE_STATE(dGoombrat_c, Die);
CREATE_STATE(dGoombrat_c, DieFalling);

const SpriteData GoombratData = {ProfileId::goombrat, 8, 0, 0, 0, 0x100, 0x100, 0x40, 0x40, 0, 0, 0};
Profile GoombratProfile(&dGoombrat_c::build, SpriteId::goombrat, &GoombratData, ProfileId::goombrat, ProfileId::goombrat, "Goombrat", RATFileList);

void dGoombrat_c::executeState_Wait() {}

dActor_c* dGoombrat_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dGoombrat_c));
	return new(buf) dGoombrat_c;
}

extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);
extern "C" void changePosAngle(VEC3 *, S16Vec *, int);

void dGoombrat_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	char hitType = usedForDeterminingStatePress_or_playerCollision(this, apThis, apOther, 0);
	
	if(hitType == 1 | hitType == 3)
	{
		apOther->someFlagByte |= 2;
		doStateChange(&StateID_Die);
	}
	else if(hitType == 0) // Take damage
	{
		dEn_c::playerCollision(apThis, apOther);
		this->_vf220(apOther->owner);
	}
	
	deathInfo.isDead = 0;
	this->flags_4FC |= (1 << (31 - 7));
	this->counter_504[apOther->owner->which_player] = 0;
}
void dGoombrat_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	this->playerCollision(apThis, apOther);
}
void dGoombrat_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) 
{
	u16 name = ((dEn_c*)apOther->owner)->name;

	// Ignore all these
	if (name == EN_COIN || name == EN_EATCOIN || name == AC_BLOCK_COIN || name == EN_COIN_JUGEM || name == EN_COIN_ANGLE
		|| name == EN_COIN_JUMP || name == EN_COIN_FLOOR || name == EN_COIN_VOLT || name == EN_COIN_WIND 
		|| name == EN_BLUE_COIN || name == EN_COIN_WATER || name == EN_REDCOIN || name == EN_GREENCOIN
		|| name == EN_JUMPDAI || name == EN_ITEM) 
		{ return; }
	
	if (acState.getCurrentState() == &StateID_Walk) 
	{
		pos.x = ((pos.x - ((dEn_c*)apOther->owner)->pos.x) > 0) ? pos.x + 1.5 : pos.x - 1.5;
		doStateChange(&StateID_Turn); 
	}
}

bool dGoombrat_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	doStateChange(&StateID_DieSmoke);
			
	return true;
}
bool dGoombrat_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	bool hit = dEn_c::collisionCat3_StarPower(apThis, apOther);
	//doStateChange(&StateID_Die);
	
	return hit;
}
bool dGoombrat_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	this->collisionCat9_RollingObject(apThis, apOther);
	//return true;
}

/*	Ice Physics	*/
void dGoombrat_c::_vf148()
{
	dEn_c::_vf148();
	doStateChange(&StateID_Die);
}
void dGoombrat_c::_vf14C()
{
	dEn_c::_vf14C();
	doStateChange(&StateID_Die);
}

extern "C" void sub_80024C20(void);
extern "C" void __destroy_arr(void *, void(*)(void), int, int);
extern "C" int SmoothRotation(short* rot, u16 amt, int unk2);
extern "C" char usedForDeterminingStatePress_or_playerCollision(dEn_c* t, ActivePhysics *apThis, ActivePhysics *apOther, int unk1);
extern "C" int SomeStrangeModification(dStageActor_c* actor);
extern "C" void DoStuffAndMarkDead(dStageActor_c *actor, Vec vector, float unk);
extern "C" bool HandlesEdgeTurns(dEn_c* actor);
extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);
	
void dGoombrat_c::_vf5C() {
    // this is something EVERYONE gets wrong
    // newer does a crappy version of this in a "updateModelMatricies" function
    // you're supposed to do it like this in finalUpdate/_vf5C
    Mtx someMatrix;
    Mtx thirdMatrix;

    changePosAngle(&pos, &rot, 1);
    PSMTXTrans(matrix, pos.x, pos.y, pos.z);
    matrix.applyRotationY(&rot.y);
    
    PSMTXTrans(someMatrix, 0.0, pos_delta2.y, 0.0);
    PSMTXConcat(matrix, someMatrix, matrix);
    matrix.applyRotationX(&rot.x);

    PSMTXTrans(thirdMatrix, 0.0, -pos_delta2.y, 0.0);
    PSMTXConcat(matrix, thirdMatrix, matrix);
    matrix.applyRotationZ(&rot.z);

    bodyModel.setDrawMatrix(matrix);
    bodyModel.setScale(initialScale.x, initialScale.y, initialScale.z);
    bodyModel.calcWorld(false);
    return;
}

bool dGoombrat_c::CreateIceActors()
{
	struct DoSomethingCool my_struct = { 0, this->pos, {1.5, 1.5, 1.5}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	this->frzMgr.Create_ICEACTORs( (void*)&my_struct, 1 );
	__destroy_arr( (void*)&my_struct, sub_80024C20, 0x3C, 1 );
	chrAnimation.setUpdateRate(0.0f);
	return true;
}

bool dGoombrat_c::calculateTileCollisions()
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


//Stolen from newer shyguy
bool dGoombrat_c::willWalkOntoSuitableGround(float delta) {
	static const float deltas[] = {delta, -delta};
	VEC3 checkWhere = {
			pos.x + deltas[direction],
			4.0f + pos.y,
			pos.z};

	u32 props = collMgr.getTileBehaviour2At(checkWhere.x, checkWhere.y, currentLayerID);

	if (((props >> 16) & 0xFF) == 8)
		return false;

	float someFloat = 0.0f;
	if (collMgr.sub_800757B0(&checkWhere, &someFloat, currentLayerID, 1, -1)) {
		if (someFloat < checkWhere.y && someFloat > (pos.y - 5.0f))
			return true;
	}

	return false;
}

void dGoombrat_c::updateModelMatrices() {
	return;

	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}
 
void dGoombrat_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

void dGoombrat_c::texPat_bindAnimChr_and_setUpdateRate(const char* name) {
	this->anmPat = this->resFile.GetResAnmTexPat(name);
	this->patAnimation.bindEntry(&this->bodyModel, &anmPat, 0, 1);
	this->bodyModel.bindAnim(&this->patAnimation);
}

int dGoombrat_c::onCreate()
{
	allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("kakibo", "g3d/kakibo.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("kakibo");
	bodyModel.setup(mdl, &allocator, 0x227, 1, 0);

	SetupTextures_Enemy(&bodyModel, 0);

	this->anmFile.data = getResource("kakibo", "g3d/kakibo.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("walk");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);
	
	this->anmPat = this->resFile.GetResAnmTexPat("walk");
	this->patAnimation.setup(mdl, anmPat, &this->allocator, 0, 1);
	this->patAnimation.bindEntry(&this->bodyModel, &anmPat, 0, 1);
	this->patAnimation.setFrameForEntry(0, 0);
	this->patAnimation.setUpdateRateForEntry(0.0f, 0);
	this->bodyModel.bindAnim(&this->patAnimation);
	
	allocator.unlink();
	
	ActivePhysics::Info HitMeBaby;
	
	// Dist from center
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 8.0;
	
	// Size
	HitMeBaby.xDistToEdge = 8.0;
	HitMeBaby.yDistToEdge = 8.0;
	
	HitMeBaby.category1 = 0x3;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0xFFBAFFFE;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();
	
	pos_delta2.x = 0.0;
    pos_delta2.y = 8.0;
    pos_delta2.z = 0.0;
	
	this->rot.x = 0;
	this->rot.y = 0xD800;
	this->rot.z = 0; 
	this->direction = 1; // facing left
	
	this->speed.x = 0.0;
	this->speed.y = 0.0;
	this->max_speed.x = 0.6;
	this->x_speed_inc = 0.15;
	
	this->walkPatTimer = 0;
	this->lookdownPatTimer = 0;
	this->texState = 0;
	
	this->Baseline = this->pos.y;
	loaded = true;
	
	// STOLEN
	spriteSomeRectX = 28.0f;
	spriteSomeRectY = 32.0f;
	_320 = 0.0f;
	_324 = 16.0f;

	// "These structs tell stupid collider what to collide with - these are from koopa troopa" - Very well spoken ~
	static const lineSensor_s below(-5<<12, 5<<12, 0<<12);
	static const pointSensor_s above(0<<12, 12<<12);
	static const lineSensor_s adjacent(6<<12, 9<<12, 6<<12);

	collMgr.init(this, &below, &above, &adjacent);
	collMgr.calculateBelowCollisionWithSmokeEffect();

	cmgr_returnValue = collMgr.isOnTopOfTile();
	
	bindAnimChr_and_setUpdateRate("walk", 1, 0.0, 1.7); 
	doStateChange(&StateID_Walk);
	
	this->onExecute();
	return true;
}

int dGoombrat_c::onDraw() {
	bodyModel.scheduleForDrawing();
	return true;
}

int dGoombrat_c::onExecute() {
	if (nearestPlayerDistance() > 600.0f)
		return true;
	
	acState.execute();
	updateModelMatrices();
	bodyModel._vf1C();
	
	// :)...
	
	/*dStateBase_c* currentState = this->acState.getCurrentState();
	OSReport("Current State: %s\n", currentState->getName());*/
	
	if(this->texState == 0)
	{
		texPat_bindAnimChr_and_setUpdateRate("walk");
	
		this->lookdownPatTimer = 0;
		
		if(walkPatTimer < 180) {
			this->patAnimation.setFrameForEntry(walkPatTimer, 0);
			walkPatTimer++;
		} else {
			walkPatTimer = 0;
			this->patAnimation.setFrameForEntry(walkPatTimer, 0);
		}
	}
	else if(this->texState == 1)
	{
		texPat_bindAnimChr_and_setUpdateRate("look_down");
	
		this->walkPatTimer = 0;
		
		if(lookdownPatTimer < 60) {
			this->patAnimation.setFrameForEntry(lookdownPatTimer, 0);
			lookdownPatTimer++;
		} else {
			this->texState = 0;
			lookdownPatTimer = 0;
		}
	}
		
	return true;
}

int dGoombrat_c::onDelete() {
	return true;
}

void dGoombrat_c::beginState_Walk() 
{
	this->timer = 0;
	this->rot.y = (direction) ? 0xD800 : 0x2800;
	
	this->max_speed.x = (direction) ? -0.5f : 0.5f;
	this->speed.x = (direction) ? -0.5f : 0.5f;
	
	this->max_speed.y = -4.0;
	this->speed.y = -4.0;
	this->y_speed_inc = -0.1875;
	
	this->texState = 0;
}

/* 		Walk 		*/
void dGoombrat_c::executeState_Walk() 
{
	bool turn = (calculateTileCollisions() || (!willWalkOntoSuitableGround(2.5f) && collMgr.isOnTopOfTile()));
	if(turn) {
		texState = 1;
		doStateChange(&StateID_Turn);
	}
	
	if(this->chrAnimation.isAnimationDone())
		this->chrAnimation.setCurrentFrame(0.0);
	
	chrAnimation.setUpdateRate(1.7f);
}
void dGoombrat_c::endState_Walk() 
{}

/* 		Turn 		*/
void dGoombrat_c::beginState_Turn()
{
	this->direction ^= 1;
	this->speed.x = 0.0;
}
void dGoombrat_c::executeState_Turn()
{
	collMgr.calculateBelowCollisionWithSmokeEffect();

	u16 rotationAmount = (this->direction) ? 0xD800 : 0x2800;
	int weDoneHere = SmoothRotation(&this->rot.y, rotationAmount, 0x150);
	
	if(weDoneHere)
		doStateChange(&StateID_Walk);
	
	if(this->chrAnimation.isAnimationDone())
		this->chrAnimation.setCurrentFrame(0.0);
	
	chrAnimation.setUpdateRate(1.7f);
}
void dGoombrat_c::endState_Turn()
{}

/* 		Die 		*/
void dGoombrat_c::beginState_Die()
{
	this->removeMyActivePhysics();
	this->rot.y = 0;
	
	bindAnimChr_and_setUpdateRate("damage", 1, 0.0, 1.0); 
}
void dGoombrat_c::executeState_Die()
{
	if(this->chrAnimation.isAnimationDone())
	{
		this->Delete(1);
	}
}
void dGoombrat_c::endState_Die()
{}

float dGoombrat_c::nearestPlayerDistance() {
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
