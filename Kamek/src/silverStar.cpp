#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

const char* SilverStarFileList[] = {"silver_star", NULL};

int silverStarsCollected = 0;

class daSilverStar_c : public dEn_c {
	int onCreate();
	int onExecute();
	int onDelete();
	int onDraw();
	
	mHeapAllocator_c allocator;
	
	nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile anmFile;
	m3d::mdl_c bodyModel;
	m3d::anmChr_c chrAnimation;
	
	int timer;
	int maxSpinSpeed;
	bool collected;
	bool stopRendering;
	int trackingPos;
	int prev;
	u32 cmgr_returnValue;
	
	dStageActor_c *playerToFollow;
	
	void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	float nearestPlayerDistance(int id);
	
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
	
	USING_STATES(daSilverStar_c);
	DECLARE_STATE(Wait);
	DECLARE_STATE(Follow);
};

CREATE_STATE(daSilverStar_c, Wait);
CREATE_STATE(daSilverStar_c, Follow);

const SpriteData SilverStarData = {ProfileId::silverstar, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile SilverStarProfile(&daSilverStar_c::build, SpriteId::silverstar, &SilverStarData, ProfileId::silverstar, ProfileId::silverstar, "silverstar", SilverStarFileList);

dActor_c* daSilverStar_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(daSilverStar_c));
	return new(buf) daSilverStar_c;
}

extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);

void daSilverStar_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	if(this->collected)
		return;
	
	silverStarsCollected++;
	
	this->collected = true;
	this->maxSpinSpeed = 0x800;
	this->timer = 0;
	
	OSReport("Silver star Follow! Current count: %p\n", silverStarsCollected);
	
	/*while(true)
	{
		// Spin faster
		if((this->rot.y + this->timer) >= this->maxSpinSpeed)
			this->rot.y += this->maxSpinSpeed;
		else
			this->rot.y += (0x400 + this->timer);
		
		// Shrink
		if(this->scale.x - (this->timer * 0.10) >= 1)
			this->scale = (Vec){this->scale.x - (this->timer * 0.10), this->scale.y - (this->timer * 0.10), this->scale.z - (this->timer * 0.10)};
		else 
			this->scale = (Vec){0.1f, 0.1f, 0.1f};
		
		// Move torwards mario
		this->direction = dSprite_c__getXDirectionOfFurthestPlayerRelativeToVEC3(this, this->pos); // Get our x direction
		
		// Get our speeds
		if(direction) //left
			this->speed.x = -5.0f;
		else
			this->speed.x = 5.0f;
		
		if(this->pos.y > apOther->owner->pos.y) // We're higher than the player
			this->speed.y = -5.0f;
		else if(this->pos.y < apOther->owner->pos.y) // We're below
			this->speed.y = 5.0f;
		else
			this->speed.y = 0.0f; // We're just right :3
		
		// SEND IT!
		HandleYSpeed();
		HandleXSpeed();
		doSpriteMovement();
		
		this->timer++;
		break;
	}*/
	
	if(silverStarsCollected > 4)
	{
		dFlagMgr_c::instance->set(21, 0, true, false, false);
	}
	
	playerToFollow = GetSpecificPlayerActor(apOther->owner->which_player);
	
	this->Delete(1);
	//doStateChange(&StateID_Follow);
}
void daSilverStar_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	playerCollision(apThis, apOther);
}
void daSilverStar_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) {

}
bool daSilverStar_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool daSilverStar_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool daSilverStar_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool daSilverStar_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool daSilverStar_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool daSilverStar_c::collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool daSilverStar_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
	if(this->collected)
		return true;
	
	silverStarsCollected++;
	
	this->collected = true;
	this->maxSpinSpeed = 0x800;
	this->timer = 0;
	
	OSReport("Silver star Follow! Current count: %p\n", silverStarsCollected);
	
	if(silverStarsCollected > 4)
	{
		dFlagMgr_c::instance->set(3, 0, true, false, false);
	}
	
	playerToFollow = GetSpecificPlayerActor(apOther->owner->which_player);
	this->Delete(1);
	//doStateChange(&StateID_Follow);
	
	return true;
}
bool daSilverStar_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool daSilverStar_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool daSilverStar_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool daSilverStar_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}

void daSilverStar_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

/*void daSilverStar_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->resFile.GetResAnmChr(name);
	this->animationChr.bind(&this->mdl, anmChr, unk);
	this->mdl.bindAnim(&this->animationChr, unk2);
	this->animationChr.setUpdateRate(rate);
}*/

int daSilverStar_c::onCreate()
{
	allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("silver_star", "g3d/silver_star.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("silver_star");
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);

	SetupTextures_MapObj(&bodyModel, 0);
	
	allocator.unlink();
	
	ActivePhysics::Info HitMeBaby;
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 0.0;
	
	HitMeBaby.xDistToEdge = 6.0;
	HitMeBaby.yDistToEdge = 6.0;
	
	HitMeBaby.category1 = 0x5;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0x200;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();
	
	// KEEPING THESE HERE BECAUSE I MIGHT HAVE GRAVITY CONTROLLED ONES. . . 
	// For ones that have gravity
	spriteSomeRectX = 28.0f;
	spriteSomeRectY = 32.0f;
	_320 = 0.0f;
	_324 = 16.0f;

	// These structs tell stupid collider what to collide with - these are from koopa troopa
	static const lineSensor_s below(-15<<12, -15<<12, -15<<12);
	static const pointSensor_s above(0<<12, 12<<12);
	static const lineSensor_s adjacent(6<<12, 9<<12, 6<<12);

	collMgr.init(this, &below, &above, &adjacent);
	collMgr.calculateBelowCollisionWithSmokeEffect();

	cmgr_returnValue = collMgr.isOnTopOfTile();
	
	this->scale = (Vec){1.5f, 1.5f, 1.5f};
	doStateChange(&StateID_Wait);
	
	this->onExecute();
	return true;
}

int daSilverStar_c::onDraw() {
	if(!stopRendering) bodyModel.scheduleForDrawing();
	return true;
}

int daSilverStar_c::onExecute() {
	acState.execute();
	updateModelMatrices();
	bodyModel._vf1C();
	
	return true;
}

int daSilverStar_c::onDelete() {
	return true;
}

float daSilverStar_c::nearestPlayerDistance(int id) {
	float bestSoFar = 10000.0f;

	if (dAcPy_c *player = dAcPy_c::findByID(id)) 
	{
		if (strcmp(player->states2.getCurrentState()->getName(), "dAcPy_c::StateID_Balloon")) 
		{
			float thisDist = abs(player->pos.x - pos.x);
			if (thisDist < bestSoFar)
				bestSoFar = thisDist;
		}
	}

	return bestSoFar;
}

/* 		Wait 		*/
void daSilverStar_c::beginState_Wait() 
{}
void daSilverStar_c::executeState_Wait() 
{
	this->rot.y += 0x400;
}
void daSilverStar_c::endState_Wait() {}

/* 		Follow  		*/
void daSilverStar_c::beginState_Follow() 
{
	//this->collected = true;
	OSReport("Silver star Follow! Current count: %p\n", silverStarsCollected);
	this->timer = 0;
	
	this->prev = silverStarsCollected;
	
	if(silverStarsCollected == 1) this->trackingPos = -15;
	else if(silverStarsCollected == 2) this->trackingPos = -25;
	else if(silverStarsCollected == 3) this->trackingPos = -35;
	else if(silverStarsCollected == 4) this->trackingPos = -45;
	else if(silverStarsCollected == 4) this->trackingPos = -55;
}
void daSilverStar_c::executeState_Follow() 
{
	this->timer++;
	
	this->rot.y += 0x600;
	if(this->timer <= 20) return;
	
	float speedDelta;
	speedDelta = 0.1;
	
	if(this->playerToFollow->direction == 0) this->trackingPos = this->trackingPos * -1;
	else this->trackingPos = this->trackingPos * 1;
	
	if(this->pos.x < this->playerToFollow->pos.x + trackingPos)
	{
		this->speed.x = this->speed.x + speedDelta;
		
		if (this->speed.x < 0) { this->speed.x = this->speed.x + (speedDelta / 2); }
		if (this->speed.x < 6.0) { this->speed.x = this->speed.x + (speedDelta); }
	}
	else if(this->pos.x > this->playerToFollow->pos.x + trackingPos)
	{
		this->speed.x = this->speed.x - speedDelta;
		
		if (this->speed.x > 0) { this->speed.x = this->speed.x - (speedDelta / 2); }
		if (this->speed.x > 2.0) { this->speed.x = this->speed.x - (speedDelta); }
	}
	else
		this->speed.x = 0.0f;
	
	if(this->pos.y < this->playerToFollow->pos.y)
		this->speed.y = 1.5f;
	else if(this->pos.y > this->playerToFollow->pos.y)
		this->speed.y = -1.5f;
	else
		this->speed.x = 0.0f;
	
	HandleYSpeed();
	HandleXSpeed();
	doSpriteMovement();
}
void daSilverStar_c::endState_Follow() {}

/*
this->timer++;
	
	if(this->prev == 1)
		this->trackingPos = this->playerToFollow->pos.x - 30.0f;
	else if(this->prev == 2)
		this->trackingPos = this->playerToFollow->pos.x + 30.0f;
	
	// We need to be SMOOTH here
	float speedDelta = 0.1f; //mr sun? hardly new em
	float speedDeltaY = 0.5f;
	// Move torwards mario
	//this->direction = (this->pos.x < this->trackingPos) ? 0 : 1;
	
	if(this->pos.y > this->trackingPos)
		this->direction = 1;
	else if(this->pos.y < this->trackingPos)
		this->direction = 0;
	
	// Get our speeds
	else if(!this->direction) //right
	{
		this->speed.x += speedDelta;
		
		if(this->speed.x < 0)
			this->speed.x += (speedDelta / 2);
		if(this->speed.x < 1.0)
			this->speed.x += speedDelta;
	}
	else
	{
		this->speed.x -= speedDelta;
		
		if(this->speed.x > 0)
			this->speed.x -= (speedDelta / 2);
		if(this->speed.x > 1.0)
			this->speed.x -= speedDelta;
	}
	
	int directionY;
	if(this->pos.y > this->playerToFollow->pos.y)
		directionY = 1;
	else if(this->pos.y < this->playerToFollow->pos.y)
		directionY = 0;
	
	else if(directionY == 0)
	{	
		this->speed.y += speedDeltaY;
		
		if(this->speed.y < 0)
			this->speed.y += (speedDeltaY / 2);
		if(this->speed.y < 4.0)
			this->speed.y += speedDeltaY;
		
		//if(this->speed.y < -2.0)
		//	this->speed.y = -2.0;
	}
	else if(directionY == 1)
	{
		this->speed.y -= speedDeltaY;
		
		if(this->speed.y > 0)
			this->speed.y -= (speedDeltaY / 2);
		if(this->speed.y > 4.0)
			this->speed.y -= speedDeltaY;
		
		//if(this->speed.y > 2.0)
		//	this->speed.y = 2.0;
	}
*/
