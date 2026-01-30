/* Coin that gives more than just 1 coin upon collection
Made by Murad for Project Horizon*/

#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

const char* BCarc[] = {"obj_coin_big", NULL};

class dBigCoin_c : public daBoss {
	int onCreate();
	int onExecute();
	int onDelete();
	int onDraw();
	
	mHeapAllocator_c allocator;
	
	nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile anmFile;
	m3d::mdl_c bodyModel;
	m3d::anmChr_c chrAnimation;
	
	bool collected;
	bool stopRendering;
	int timer;
	int coinAmount;
	int soundTimer;
	bool gravity;
	u32 cmgr_returnValue;
	const char *brresName;
	const char *mdlName;
	int bounceCounter;
	
	void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	
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
	
	USING_STATES(dBigCoin_c);
	DECLARE_STATE(Wait);
	DECLARE_STATE(Collected);
};

CREATE_STATE(dBigCoin_c, Wait);
CREATE_STATE(dBigCoin_c, Collected);

const SpriteData BigCoinData = {ProfileId::bigcoin, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile BigCoinProfile(&dBigCoin_c::build, SpriteId::bigcoin, &BigCoinData, ProfileId::bigcoin, ProfileId::bigcoin, "bigcoin", BCarc);

extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);

dActor_c* dBigCoin_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dBigCoin_c));
	return new(buf) dBigCoin_c;
}

extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);
extern int MaxCoins;

void dBigCoin_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	if(this->collected)
		return;
	
	this->collected = true;
	
	//Increment our stuffs
	int playerIndex = Player_ID[apOther->owner->which_player];
	Player_Coins[playerIndex] += coinAmount;
	
	nw4r::snd::SoundHandle handle;
	if(Player_Coins[playerIndex] > MaxCoins)
	{
		//1-up stuffs
		PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_SYS_100COIN_ONE_UP, 1);
		
		if(Player_Lives[playerIndex] >= 99)
			Player_Lives[playerIndex] = 99;
		else
			Player_Lives[playerIndex] += 1;
		
		//Resetting the coin counter stuffs
		int difference = Player_Coins[playerIndex] - MaxCoins;
		Player_Coins[playerIndex] = (difference - 1); // For some reason we have to subtract 1 here
	}
	
	//Don't know why exactly THIS is here but... I LIKE IT!
	PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_GET_COIN, 1);
	
	doStateChange(&StateID_Collected);
}
void dBigCoin_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	playerCollision(apThis, apOther);
}
void dBigCoin_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) 
{
	if(this->collected)
		return;
}
bool dBigCoin_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dBigCoin_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dBigCoin_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dBigCoin_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dBigCoin_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dBigCoin_c::collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dBigCoin_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
	this->collected = true;
	
	//Increment our stuffs
	int playerIndex = Player_ID[apOther->owner->which_player];
	Player_Coins[playerIndex] += coinAmount;
	
	nw4r::snd::SoundHandle handle;
	if(Player_Coins[playerIndex] > MaxCoins)
	{
		//1-up stuffs
		PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_SYS_100COIN_ONE_UP, 1);
		
		if(Player_Lives[playerIndex] >= 99)
			Player_Lives[playerIndex] = 99;
		else
			Player_Lives[playerIndex] += 1;
		
		//Resetting the coin counter stuffs
		int difference = Player_Coins[playerIndex] - MaxCoins;
		Player_Coins[playerIndex] = (difference - 1); // For some reason we have to subtract 1 here
	}
	
	//Don't know why exactly THIS is here but... I LIKE IT!
	PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_GET_COIN, 1);
	
	doStateChange(&StateID_Collected);
	return true;
}
bool dBigCoin_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dBigCoin_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dBigCoin_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dBigCoin_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}

void dBigCoin_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

/*void dBigCoin_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->resFile.GetResAnmChr(name);
	this->animationChr.bind(&this->mdl, anmChr, unk);
	this->mdl.bindAnim(&this->animationChr, unk2);
	this->animationChr.setUpdateRate(rate);
}*/

int dBigCoin_c::onCreate()
{
	this->gravity = (settings & 0x2000);
	
	//coinAmount = this->settings >> 28 & 0xF;
	brresName = "g3d/coin_10.brres";
	mdlName = "coin_10";
			
	switch(this->settings >> 28 & 0xF)
	{
		case 0:
			coinAmount = 10;
			
			brresName = "g3d/coin_10.brres";
			mdlName = "coin_10";
			break;
		case 1:
			coinAmount = 50;
			
			brresName = "g3d/coin_50.brres";
			mdlName = "coin_50";
			break;
		case 2:
			coinAmount = 100; //Will be 100 later
			
			brresName = "g3d/coin_100.brres";
			mdlName = "coin_100";
			break;
		
	}
	
	int backDoorSetting = (this->settings) & 15;
	
	if(backDoorSetting == 1) {
		coinAmount = 50;
		brresName = "g3d/coin_50.brres";
		mdlName = "coin_50";
	} else if(backDoorSetting == 2) {
		coinAmount = 100;
		brresName = "g3d/coin_100.brres";
		mdlName = "coin_100";
	}
	
	allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("obj_coin_big", brresName);
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl(mdlName);
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);

	SetupTextures_MapObj(&bodyModel, 0);
	
	allocator.unlink();
	
	ActivePhysics::Info HitMeBaby;
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 0.0;
	
	HitMeBaby.xDistToEdge = 12.0;
	HitMeBaby.yDistToEdge = 12.0;
	
	HitMeBaby.category1 = 0x5;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0x200;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();
	
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
	this->bounceCounter = 2;
	
	doStateChange(&StateID_Wait);
	
	this->onExecute();
	return true;
}

int dBigCoin_c::onDraw() {
	if(!stopRendering) bodyModel.scheduleForDrawing();
	return true;
}

int dBigCoin_c::onExecute() {
	acState.execute();
	updateModelMatrices();
	bodyModel._vf1C();
	
	return true;
}

int dBigCoin_c::onDelete() {
	return true;
}

void dBigCoin_c::beginState_Wait() 
{
	this->max_speed.y = -4.0;
	this->speed.y = -4.0;
	this->y_speed_inc = -0.1875;
}
void dBigCoin_c::executeState_Wait() 
{
	if(this->gravity)
	{
		cmgr_returnValue = collMgr.isOnTopOfTile();
		collMgr.calculateBelowCollisionWithSmokeEffect();
	
		if(collMgr.isOnTopOfTile())
		{
			if(this->bounceCounter <= 0)
			{
				this->speed.y = 0.0f;
				this->speed.x = 0.0f;
			}
			else if(bounceCounter == 2)
			{
				this->speed.y = 6.0f / 2;
				this->speed.x = 2.5f / 2;
				this->bounceCounter--;
			}
			else if(bounceCounter == 1)
			{
				this->speed.y = 6.0f / 3;
				this->speed.x = 2.5f / 3;
				this->bounceCounter--;
			}
		}
		else
		{
			//Did we get spawned out of a block and need adjusting?
			if(this->speed.y > -1.0)
			{
				if(this->speed.x > 0.1f) this->speed.x -= 0.1f;
				this->speed.y -= 0.1f;
			}
		}
		
		HandleXSpeed();
		HandleYSpeed();
		doSpriteMovement();
	}
	
	this->rot.y += 0x400;
}
void dBigCoin_c::endState_Wait() {}

void dBigCoin_c::beginState_Collected() 
{
	this->stopRendering = true;
	
    this->removeMyActivePhysics();
	this->timer = 0;
	
	//For the "nsmb-2" coin spam sound effect
	if(this->coinAmount == 10) this->soundTimer = 10;
	else if(this->coinAmount == 50) this->soundTimer = 16;
	else this->soundTimer = 22;

	Vec effectScale = (Vec){1.0, 1.0, 1.0};
	float posOffset = 10.0f;

	if(this->coinAmount == 50)
	{	
		effectScale = (Vec){1.5, 1.5, 1.5};
		posOffset = 25.0f;
	}
	else if(this->coinAmount == 100)
	{
		effectScale = (Vec){2, 2, 2};
		posOffset = 20.0f;
	}

	SpawnEffect("Wm_ob_coinkira", 0, &(Vec){this->pos.x, this->pos.y - posOffset, this->pos.z}, &(S16Vec){0,0,0}, &effectScale);
}
void dBigCoin_c::executeState_Collected() 
{
	nw4r::snd::SoundHandle handle;
	
	if(this->timer % 2 == 0)
		PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_GET_COIN, 1);
	
	if(this->timer >= this->soundTimer)
		this->Delete(1);
	
	this->timer++;
}
void dBigCoin_c::endState_Collected() {}


