#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

const char* EVFileList[] = {"obj_coin_switch", NULL};

class dEnEventCoin_c : public daBoss {
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
	bool timedEvent;
	int timeToEnd;
	int timer;
	u8 targetEventID;
	u8 event;
	u64 eventFlag;

	int animCheck;
	
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
	
	USING_STATES(dEnEventCoin_c);
	DECLARE_STATE(Wait);
	DECLARE_STATE(Collected);
};

CREATE_STATE(dEnEventCoin_c, Wait);
CREATE_STATE(dEnEventCoin_c, Collected);

const SpriteData EventcoinData = {ProfileId::eventcoin, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile EventcoinProfile(&dEnEventCoin_c::build, SpriteId::eventcoin, &EventcoinData, ProfileId::eventcoin, ProfileId::eventcoin, "eventcoin", EVFileList);

dActor_c* dEnEventCoin_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dEnEventCoin_c));
	return new(buf) dEnEventCoin_c;
}

extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);

void dEnEventCoin_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	if(!collected)
		doStateChange(&StateID_Collected);
}
void dEnEventCoin_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	if(!collected)
		doStateChange(&StateID_Collected);
}
void dEnEventCoin_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) {

}
bool dEnEventCoin_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dEnEventCoin_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dEnEventCoin_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dEnEventCoin_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dEnEventCoin_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dEnEventCoin_c::collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dEnEventCoin_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
	if(!collected)
		doStateChange(&StateID_Collected);
	
	return true;
}
bool dEnEventCoin_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dEnEventCoin_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dEnEventCoin_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dEnEventCoin_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}

void dEnEventCoin_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

void dEnEventCoin_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->resFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

int dEnEventCoin_c::onCreate()
{
	allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("obj_coin_switch", "g3d/obj_coin_switch.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("switch_coin");
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);

	SetupTextures_MapObj(&bodyModel, 0);

	this->anmFile.data = getResource("obj_coin_switch", "g3d/obj_coin_switch.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("coin_get");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);

	allocator.unlink(); 
	
	ActivePhysics::Info HitMeBaby;
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 0.0;
	
	HitMeBaby.xDistToEdge = 12.0;
	HitMeBaby.yDistToEdge = 12.0;
	HitMeBaby.category1 = 0x5;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0xFFFFFFFF;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();
	
	this->collected = false;
	
	this->timeToEnd = (this->settings >> 8 & 0b11111111);	
	this->timeToEnd = this->timeToEnd * 60;
	
	if(this->timeToEnd > 1)
		timedEvent = true;
	else
		timedEvent = false;
	
	targetEventID = (u8)((settings & 0xFF) - 1);
	
	// triggerid
	u8 eventNum = (this->settings >> 28) & 0xF;
	this->eventFlag = (eventNum == 0) ? 0 : ((u64)1 << (eventNum - 1));

	OSReport("eventFlag: %llu\n", this->eventFlag);
	OSReport("eventNum: %d\n", eventNum);
	
	animCheck = 0;

	doStateChange(&StateID_Wait);
	
	this->onExecute();

	u64 initialFlags = dFlagMgr_c::instance->flags & dStageActor_c::creatingFlagMask;

	if(dFlagMgr_c::instance->active(targetEventID))
		return 2;
	else
		return 1;
}

int dEnEventCoin_c::onDraw() {
	if(!stopRendering) bodyModel.scheduleForDrawing();
	return true;
}

int dEnEventCoin_c::onExecute() {
	// triggerid check
	//if (this->eventFlag == 0 || (dFlagMgr_c::instance->flags & this->eventFlag) || animCheck == 0) {
		acState.execute();
		updateModelMatrices();
		bodyModel._vf1C();
	//}

	return true;
}

int dEnEventCoin_c::onDelete() {
	return true;
}

void dEnEventCoin_c::beginState_Wait() {}
void dEnEventCoin_c::executeState_Wait() 
{
	this->rot.y += 0x400;
}
void dEnEventCoin_c::endState_Wait() {}

void dEnEventCoin_c::beginState_Collected() 
{
    this->removeMyActivePhysics();
	this->timer = 0;
	this->rot.y = 0;
	
	bindAnimChr_and_setUpdateRate("coin_get", 1, 0.0, 1.0);
	SpawnEffect("Wm_ob_starcoinget", 0, &this->pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});
	PlaySound(this, SE_OBJ_GET_DRAGON_COIN);

	event = targetEventID;
}
void dEnEventCoin_c::executeState_Collected() 
{
	if(!this->chrAnimation.isAnimationDone())
		return;

	this->stopRendering = true;

	// If our event is already active we'll kill it later
	bool alreadyActivated = dFlagMgr_c::instance->active(event);
	
	// "later"
	if(!collected) dFlagMgr_c::instance->set(event, 0, !alreadyActivated, false, false);
	this->collected = true;
	
	if(!timedEvent)
		//if(this->chrAnimation.isAnimationDone())
			{this->Delete(deleteForever);
			fBase_c::Delete();}
	else
	{
		if(this->timer > timeToEnd)
		{
			dFlagMgr_c::instance->set(event, 0, false, false, false);
			this->Delete(deleteForever);
			fBase_c::Delete();
		}
	}
	
	this->timer++;
}
void dEnEventCoin_c::endState_Collected() {}


