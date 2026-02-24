#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

const char* CashMoneyArc[] = {"gamaguchi", NULL};

float W1L2[9][2] = {
	{3.1f, 6.5f},
	{3.1f, 6.5f},
	{3.9f, 6.7f},
	{3.6f, 6.7f},
	{3.4f, 6.5f},
	{3.0f, 6.0f},
	{3.0f, 6.0f},
	{3.6f, 6.7f},
	{3.4f, 6.5f}
	//{1.5f, 3.5f},
};

class daMoneyBags_c : public dEn_c {
	int onCreate();
	int onExecute();
	int onDelete();
	int onDraw();
	
	mHeapAllocator_c allocator;
	
	nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile anmFile;
	m3d::mdl_c bodyModel;
	m3d::anmChr_c chrAnimation;
	
	nw4r::g3d::ResAnmClr anmClr;
	m3d::anmClr_c platClr;
	
	int timer;
	int health;
	int damagePatTimer;
	int Baseline;
	bool doTexAnim;
	bool spawnedIn;
	bool finishedJumping;
	bool waiting;
	int cuedToWait;
	int coinCount;
	bool isHit;
	int jumpCount;
	
	float xSpeed;
	float ySpeed;
	u32 cmgr_returnValue;
	dStageActor_c* coin;
	dStageActor_c* deathDrop;
	u32 coinID;
	
	void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	void texPat_bindAnimChr_and_setUpdateRate(const char* name);
	
	void playerCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	
	bool collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther);
	
	bool calculateTileCollisions();
	bool willWalkOntoSuitableGround();
	float nearestPlayerDistance();
	void _vf148();
	void _vf14C();
	bool CreateIceActors();
	void doDeathDrop();
	
	bool loaded;
	
	public: static dActor_c *build();
	
	USING_STATES(daMoneyBags_c);
	DECLARE_STATE(WaitCoin);
	DECLARE_STATE(WaitStand);
	DECLARE_STATE(Jump);
	DECLARE_STATE(Land);
	DECLARE_STATE(Turn);
	DECLARE_STATE(Hit);
};

CREATE_STATE(daMoneyBags_c, WaitCoin);
CREATE_STATE(daMoneyBags_c, WaitStand);
CREATE_STATE(daMoneyBags_c, Jump);
CREATE_STATE(daMoneyBags_c, Land);
CREATE_STATE_E(daMoneyBags_c, Turn);
CREATE_STATE(daMoneyBags_c, Hit);

const SpriteData MoneyBagsData = {ProfileId::moneybags, 8, -8, 0, 0, 0x100, 0x100, 0x40, 0x40, 0, 0, 0};
Profile MoneyBagsProfile(&daMoneyBags_c::build, SpriteId::moneybags, &MoneyBagsData, ProfileId::moneybags, ProfileId::moneybags, "moneybags", CashMoneyArc);

dActor_c* daMoneyBags_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(daMoneyBags_c));
	return new(buf) daMoneyBags_c;
}

void daMoneyBags_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	char hitType = usedForDeterminingStatePress_or_playerCollision(this, apThis, apOther, 0);
	
	//bool whatever = apOther->owner->pos.y > (this->pos.y + 8);
	
	if(hitType == 1 | hitType == 3)
	{
		if(this->health >= 20)
		{
			//doDeathDrop();
			
			apOther->someFlagByte |= 2;
			doStateChange(&StateID_DieSmoke);
		}
		else
		{
			if(!this->isHit)
			{
				this->health += 10;
				this->isHit = true;
				
				bindAnimChr_and_setUpdateRate("damage", 1, 0.0, 1); 
				//doStateChange(&StateID_Hit);
			}
		}
	}
	else if(hitType == 0) // Take damage
	{
		if(this->isHit) return;
		
		dEn_c::playerCollision(apThis, apOther);
		this->_vf220(apOther->owner);
	}
	
	//deathInfo.isDead = 0;
	//this->flags_4FC |= (1 << (31 - 7));
	//this->counter_504[apOther->owner->which_player] = 0;
}
void daMoneyBags_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	this->playerCollision(apThis, apOther);
}
void daMoneyBags_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) 
{
	u16 name = ((dEn_c*)apOther->owner)->name;

	// Ignore all these
	if (name == EN_COIN || name == EN_EATCOIN || name == AC_BLOCK_COIN || name == EN_COIN_JUGEM || name == EN_COIN_ANGLE
		|| name == EN_COIN_JUMP || name == EN_COIN_FLOOR || name == EN_COIN_VOLT || name == EN_COIN_WIND 
		|| name == EN_BLUE_COIN || name == EN_COIN_WATER || name == EN_REDCOIN || name == EN_GREENCOIN
		|| name == EN_JUMPDAI || name == EN_ITEM) 
		{ return; }
	
	/*if (acState.getCurrentState() == &StateID_Walk) 
	{
		pos.x = ((pos.x - ((dEn_c*)apOther->owner)->pos.x) > 0) ? pos.x + 1.5 : pos.x - 1.5;
		doStateChange(&StateID_Turn); 
	}*/
}

bool daMoneyBags_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) 
{
	this->health += 5;
	StageE4::instance->spawnCoinJump(pos, 0, 1, 0);
	this->doTexAnim = true;
	
	return true;
}
bool daMoneyBags_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	doStateChange(&StateID_DieSmoke);
			
	return true;
}
bool daMoneyBags_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	bool hit = dEn_c::collisionCat3_StarPower(apThis, apOther);
	//doStateChange(&StateID_Die);
	
	return hit;
}
bool daMoneyBags_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) { 
	PlaySound(this, SE_EMY_DOWN); 
	SpawnEffect("Wm_mr_hardhit", 0, &pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});
	
	doStateChange(&StateID_DieSmoke); 
	return true;
}
bool daMoneyBags_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	return this->collisionCatD_Drill(apThis, apOther);
}

/*	Ice Physics	*/
void daMoneyBags_c::_vf148()
{
	dEn_c::_vf148();
	doStateChange(&StateID_Hit);
}
void daMoneyBags_c::_vf14C()
{
	dEn_c::_vf14C();
	doStateChange(&StateID_Hit);
}

extern "C" void sub_80024C20(void);
extern "C" void __destroy_arr(void *, void(*)(void), int, int);
extern "C" int SmoothRotation(short* rot, u16 amt, int unk2);
extern "C" char usedForDeterminingStatePress_or_playerCollision(dEn_c* t, ActivePhysics *apThis, ActivePhysics *apOther, int unk1);
extern "C" int SomeStrangeModification(dStageActor_c* actor);
extern "C" void DoStuffAndMarkDead(dStageActor_c *actor, Vec vector, float unk);
extern "C" bool HandlesEdgeTurns(dEn_c* actor);
extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);
	
bool daMoneyBags_c::CreateIceActors()
{
	struct DoSomethingCool my_struct = { 0, this->pos, {1.0, 1.0, 1.0}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	this->frzMgr.Create_ICEACTORs( (void*)&my_struct, 1 );
	__destroy_arr( (void*)&my_struct, sub_80024C20, 0x3C, 1 );
	chrAnimation.setUpdateRate(0.0f);
	return true;
}

bool daMoneyBags_c::calculateTileCollisions()
{
	//HandleXSpeed();
	//HandleYSpeed();
	//doSpriteMovement();
	
	cmgr_returnValue = collMgr.isOnTopOfTile();
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
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
bool daMoneyBags_c::willWalkOntoSuitableGround() {
	static const float deltas[] = {2.5f, -2.5f};
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

void daMoneyBags_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

void daMoneyBags_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

int daMoneyBags_c::onCreate()
{
	allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("gamaguchi", "g3d/gamaguchi.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("gamaguchi");
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);

	SetupTextures_Enemy(&bodyModel, 0);

	this->anmFile.data = getResource("gamaguchi", "g3d/gamaguchi.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("jump_st");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);
	
	nw4r::g3d::ResAnmClr anmRes = resFile.GetResAnmClr("damage");
    platClr.setup(mdl, anmRes, &allocator, 0, 1);
    platClr.bind(&bodyModel, anmRes, 0, 0);
	platClr.setUpdateRateForEntry(1.0f, 0);
    bodyModel.bindAnim(&platClr, 0.0f);
	
	allocator.unlink();
	
	ActivePhysics::Info HitMeBaby;
	
	// Dist from center
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 8.0;
	
	// Size
	HitMeBaby.xDistToEdge = 16.0;
	HitMeBaby.yDistToEdge = 8.0;
	
	HitMeBaby.category1 = 0x3;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0xFFBAFFFE;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();
	
	this->rot.x = 0;
	this->rot.y = 0x2800;
	this->rot.z = 0; 
	this->direction = 0; // facing right
	
	this->speed.x = 0.0;
	this->speed.y = 0.0;
	this->max_speed.x = 0.6;
	this->x_speed_inc = 0.15;
	
	this->doTexAnim = false;
	this->damagePatTimer = 0;
	
	this->Baseline = this->pos.y;
	this->coinCount = 0;
	this->isHit = false;
	loaded = true;
	
	this->ySpeed = 6.0f;
	this->xSpeed = 3.5f;
	this->jumpCount = 0;
	
	this->scale = (Vec){0.75f, 0.75f, 0.75f};
	
	// STOLEN
	spriteSomeRectX = 28.0f;
	spriteSomeRectY = 32.0f;
	_320 = 0.0f;
	_324 = 16.0f;

	// "These structs tell stupid collider what to collide with - these are from koopa troopa" - Very well spoken ~
	static const lineSensor_s below(3<<12, 3<<12, 3<<12);
	static const pointSensor_s above(0<<12, 12<<12);
	static const lineSensor_s adjacent(6<<12, 9<<12, 6<<12);

	collMgr.init(this, &below, &above, &adjacent);
	collMgr.calculateBelowCollisionWithSmokeEffect();

	cmgr_returnValue = collMgr.isOnTopOfTile();
	
	//Spawn our coin
	coin = (dStageActor_c*)CreateActor(401, 0, this->pos, 0, 0);
	coinID = coin->id;
	
	this->spawnedIn = false;
	this->waiting = false;
	this->cuedToWait = 0;
	
	doStateChange(&StateID_WaitCoin);
	
	this->onExecute();
	return true;
}

int daMoneyBags_c::onDraw() {
	if(this->spawnedIn) bodyModel.scheduleForDrawing();
	return true;
}

int daMoneyBags_c::onExecute() {
	acState.execute();
	
	if(this->spawnedIn)
	{
		updateModelMatrices();
		bodyModel._vf1C();
	}
	
	if(this->doTexAnim)
	{
		this->damagePatTimer++;
		
		if(this->damagePatTimer <= 21)
			platClr.process();
		else
		{
			platClr.setFrameForEntry(0, 0);
			this->damagePatTimer = 0;
			this->doTexAnim = false;
		}
	}
	// Is our coin ok?
	if(!this->spawnedIn)
	{
		dStageActor_c *ac = (dStageActor_c*)fBase_c::search(coinID);

		if (!ac) {
			coin = (dStageActor_c*)CreateActor(401, 0, this->pos, 0, 0);
			coinID = coin->id;
		}
	}
	else
	{
		dStageActor_c *ac = (dStageActor_c*)fBase_c::search(coinID);
		
		if(ac)
			ac->Delete(1);
	}
	
	cmgr_returnValue = collMgr.isOnTopOfTile();
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
	if(collMgr.isOnTopOfTile())
	{
	this->speed.y = 0.0f; }
	
	return true;
}

int daMoneyBags_c::onDelete() {
	return true;
}

float daMoneyBags_c::nearestPlayerDistance() {
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

/* 		Wait Coin 		*/
void daMoneyBags_c::beginState_WaitCoin()
{}
void daMoneyBags_c::executeState_WaitCoin()
{
	if(this->nearestPlayerDistance() <= 175.0f)
	{
		//dStageActor_c *ac = (dStageActor_c*)fBase_c::search(coinID);
		//ac->Delete(1);
		
		doStateChange(&StateID_Land);
	}
}
void daMoneyBags_c::endState_WaitCoin()
{}

/* 		Jump  		*/
void daMoneyBags_c::beginState_Jump()
{
	this->spawnedIn = true;
	
	// Getting our hardcoded speeds
	this->xSpeed = W1L2[this->jumpCount][0];
	this->ySpeed = W1L2[this->jumpCount][1];
	
	if(!this->waiting)
	{
		this->max_speed.x = (direction) ? -xSpeed : xSpeed;
		this->speed.x = (direction) ? -xSpeed : xSpeed;
	}
	else
	{
		this->max_speed.x = 0;
		this->speed.x = 0;
	}
	
	this->speed.y = (!this->waiting) ? ySpeed : 2.0f;
	this->timer = 0;
	
	if(!this->waiting) 
	{
		if(this->coinCount <= 5)
		{
			StageE4::instance->spawnCoinJump(pos, 0, 1, 0);
			this->coinCount++;
		}
	}
}
void daMoneyBags_c::executeState_Jump()
{
	// For ground stuffs
	cmgr_returnValue = collMgr.isOnTopOfTile();
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
	if(this->calculateTileCollisions())
		doStateChange(&StateID_Hit);
	
	if(this->chrAnimation.isAnimationDone())
	{
		this->isHit = false;
		bindAnimChr_and_setUpdateRate("jump_air", 1, 0.0, 2); 
	}
	
	//if(this->calculateTileCollisions()) this->speed.x = 0.0f;
	
	// Standing
	if(collMgr.isOnTopOfTile())
	{
		this->speed.x = 0.0f;
		this->speed.y = 0.0f;
		
		bindAnimChr_and_setUpdateRate("jump_end", 1, 0.0, 2); 
		
		if(this->jumpCount >= 8 || this->jumpCount <= 0)
		{
			this->direction ^= 1;
			doStateChange(&StateID_Turn);
			
			return;
		}
		
		if(this->nearestPlayerDistance() >= 175.0f)
			this->waiting = true;
		else
			this->waiting = false;
		
		// Face Mario
		u8 facing = dSprite_c__getXDirectionOfFurthestPlayerRelativeToVEC3(this, this->pos);
		
		if(this->waiting && (facing != this->direction))
		{
			this->direction = facing;
			doStateChange(&StateID_Turn);
		}
		else if(!this->waiting && (facing == this->direction))
		{
			this->direction ^= 1;
			doStateChange(&StateID_Turn);
		}
		else
			doStateChange(&StateID_Land);
	}
	
	// Jump
	else
	{
		if(this->speed.y <= -4.0f)
			this->speed.y = -4.0f;	// Max gravity speed
		else
			this->speed.y -= 0.15;	// Reduce
	}
	
	HandleXSpeed();
	HandleYSpeed();
	doSpriteMovement();
}
void daMoneyBags_c::endState_Jump()
{}

/*	Land		*/
void daMoneyBags_c::beginState_Land()
{
	this->timer = 0;
	
	this->speed.x = 0.0f;
	this->speed.y = 0.0f;
	
	if(!this->waiting)
		this->jumpCount += (this->direction == 1) ? -1 : 1;
}
void daMoneyBags_c::executeState_Land()
{
	this->timer += 1;
	
	// For anim stuffs
	if(this->timer == 11)
	{
		bindAnimChr_and_setUpdateRate("jump_st", 1, 0.0, 2); 
		return;
	}
	else if(this->timer == 16)
	{
		bindAnimChr_and_setUpdateRate("jump_air", 1, 0.0, 2); 
	}
	
	if(this->timer <= 16) return;
	
	// Getting our hardcoded speed
	this->ySpeed = W1L2[this->jumpCount][1];
	
	this->speed.y = (!this->waiting) ? ySpeed : 2.0f;
	
	cmgr_returnValue = collMgr.isOnTopOfTile();
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
	if(!collMgr.isOnTopOfTile())
		doStateChange(&StateID_Jump);
	
	HandleXSpeed();
	HandleYSpeed();
	doSpriteMovement();
}
void daMoneyBags_c::endState_Land()
{}

/*	Turn		*/
void daMoneyBags_c::executeState_Turn() 
{
	u16 amt = (this->direction == 0) ? 0x2800 : 0xD800;
	int done = SmoothRotation(&this->rot.y, amt, 0x1000);

	if(done) {
		this->doStateChange(&StateID_Land);
	}
}

/*	Hit		*/
void daMoneyBags_c::beginState_Hit() 
{ 
	this->isHit = true;
	
	bindAnimChr_and_setUpdateRate("damage", 1, 0.0, 1); 
}
void daMoneyBags_c::executeState_Hit() 
{ 
	cmgr_returnValue = collMgr.isOnTopOfTile();
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
	if(collMgr.isOnTopOfTile() && this->chrAnimation.isAnimationDone())
	{
		this->speed.x = 0.0f;
		this->speed.y = 0.0f;
		
		doStateChange(&StateID_Land);
	}
	
	if(this->chrAnimation.isAnimationDone())
		this->chrAnimation.setCurrentFrame(0.0);
	
	this->speed.x = 0.0f;
	
	if(this->speed.y <= -4.0f)
		this->speed.y = -4.0f;	// Max gravity speed
	else
		this->speed.y -= 0.15;	// Reduce
	
	HandleXSpeed();
	HandleYSpeed();
	doSpriteMovement();
}
void daMoneyBags_c::endState_Hit() 
{ 
	this->isHit = false;
}

void daMoneyBags_c::doDeathDrop()
{
	deathDrop = (dStageActor_c*)CreateActor(762, 0, pos, 0, 0);
}
