#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>

#include "path.h"

const char* BiddyBudFileList[] = {"biddy", NULL};

class dBiddyBud_c : public dEnPath_c {
public:
	int onCreate();
	int onExecute();
	int onDelete();
	int onDraw();

	mHeapAllocator_c allocator;
	nw4r::g3d::ResFile resFile;
	m3d::mdl_c bodyModel;
	m3d::anmChr_c chrAnimation;
    nw4r::g3d::ResFile anmFile;
	nw4r::g3d::ResAnmTexPat anmPat;
	m3d::anmTexPat_c eyeAnimation;

	bool dying;
	int deathType; // 0 = death, 1 = squish
	bool reverseMode;
	int timer;
	int previousEyes;
	int targetEyes;
	int eyeChangeDelay;

	static dActor_c* build();

	void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	void bindEyesAnimation();
	void setEyesFrame(int frame);
	void startDeath(bool squish);
	void updateEyesByMovement();

	void beginReverseSegment(int fromNodeNum, int toNodeNum);
    void executeReverseMovement();

	void playerCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther);

	bool collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther);

	USING_STATES(dBiddyBud_c);
	DECLARE_STATE(Die);
};

CREATE_STATE(dBiddyBud_c, Die);

const SpriteData BiddyBudSpriteData = {ProfileId::dBiddyBud, 8, -8, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile BiddyBudProfile(&dBiddyBud_c::build, SpriteId::dBiddyBud, &BiddyBudSpriteData, ProfileId::dBiddyBud, ProfileId::dBiddyBud, "dBiddyBud", BiddyBudFileList);

dActor_c* dBiddyBud_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dBiddyBud_c));
	return new(buf) dBiddyBud_c;
}

void dBiddyBud_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->resFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

void dBiddyBud_c::bindEyesAnimation() {
	this->anmPat = this->resFile.GetResAnmTexPat("eyes");
	this->eyeAnimation.setup(this->resFile.GetResMdl("biddybud"), anmPat, &this->allocator, 0, 1);
	this->eyeAnimation.bindEntry(&this->bodyModel, &anmPat, 0, 1);
	this->eyeAnimation.setFrameForEntry(0.0f, 0);
	this->eyeAnimation.setUpdateRateForEntry(0.0f, 0);
	this->bodyModel.bindAnim(&this->eyeAnimation);
}

void dBiddyBud_c::setEyesFrame(int frame) {
	this->previousEyes = frame;

	if (frame < 0)
		frame = 0;
	if (frame > 5)
		frame = 5;
	this->eyeAnimation.setFrameForEntry((float)frame, 0);
}

void dBiddyBud_c::startDeath(bool squish) {
	if (this->dying)
		return;

	this->dying = true;
	this->deathType = squish ? 1 : 0;
	this->removeMyActivePhysics();
	this->setEyesFrame(5);

	if (squish)
	{
		this->bindAnimChr_and_setUpdateRate("squish", 1, 0.0f, 1.0f);
		doStateChange(&StateID_Die);
	}
	else
	{
		this->bindAnimChr_and_setUpdateRate("death", 1, 0.0f, 1.0f);
	}

	
}

void dBiddyBud_c::updateEyesByMovement() {
	if (this->dying)
		return;

	float absX = abs(this->stepVector.x);
	float absY = abs(this->stepVector.y);

	int desiredEyes = 0;

	if (!(absX < 0.05f && absY < 0.05f)) {
		if (absY >= absX)
			desiredEyes = ((this->stepVector.y >= 0.0f) ? 1 : 2);
		else
			desiredEyes = ((this->stepVector.x < 0.0f) ? 3 : 4);
	}

	// If the desired direction changed, play frame 0 for 3 frames before switching.
	if (desiredEyes != this->targetEyes) {
		this->targetEyes = desiredEyes;
		this->eyeChangeDelay = 0;
		setEyesFrame(0);
		return;
	}

	// If we're mid-transition, keep showing frame 0 until the delay expires.
	if (this->eyeChangeDelay > 0) {
		this->eyeChangeDelay--;
		if (this->eyeChangeDelay == 0)
			setEyesFrame(this->targetEyes);
		else
			setEyesFrame(0);
		return;
	}

	setEyesFrame(desiredEyes);
}

void dBiddyBud_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther) {
	if (this->dying)
		return;

	char hitType = usedForDeterminingStatePress_or_playerCollision(this, apThis, apOther, 0);

	if (hitType == 1 || hitType == 3) {
		apOther->someFlagByte |= 2;
		
		startDeath(true);

	} else if (hitType == 0) {
		dEn_c::playerCollision(apThis, apOther);
		this->_vf220(apOther->owner);
	}

	deathInfo.isDead = 0;
	this->flags_4FC |= (1 << (31 - 7));
	this->counter_504[apOther->owner->which_player] = 0;
}

void dBiddyBud_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther) {
	this->playerCollision(apThis, apOther);
}

bool dBiddyBud_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	this->bindAnimChr_and_setUpdateRate("death", 1, 0.0f, 1.0f);
	dEn_c::collisionCat3_StarPower(apThis, apOther);

	doStateChange(&StateID_Die);

	return true;
}
bool dBiddyBud_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) { 
	this->bindAnimChr_and_setUpdateRate("death", 1, 0.0f, 1.0f);

	PlaySound(this, SE_EMY_DOWN); 
	SpawnEffect("Wm_mr_hardhit", 0, &pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});
	//addScoreWhenHit accepts a player parameter.
	//DON'T DO THIS:
	// this->addScoreWhenHit(this);
	doStateChange(&StateID_Die); 

	return true; 
}
bool dBiddyBud_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) { 
	return this->collisionCat9_RollingObject(apThis, apOther);
}
bool dBiddyBud_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) { 
	return this->collisionCat9_RollingObject(apThis, apOther);
}
bool dBiddyBud_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) { 
	this->bindAnimChr_and_setUpdateRate("death", 1, 0.0f, 1.0f);
	dEn_c::collisionCat9_RollingObject(apThis, apOther);

	doStateChange(&StateID_Die);

	return true;
}
bool dBiddyBud_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther) { 
	return this->collisionCat9_RollingObject(apThis, apOther);
}
bool dBiddyBud_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) { 
	return this->collisionCat9_RollingObject(apThis, apOther);
}
bool dBiddyBud_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) { 
	return this->collisionCat9_RollingObject(apThis, apOther);
}
bool dBiddyBud_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) { 
	return this->collisionCat9_RollingObject(apThis, apOther);
}

void dBiddyBud_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

int dBiddyBud_c::onCreate() {
	const char* brresNames[4] = {
		"g3d/biddy_red.brres",
		"g3d/biddy_green.brres",
		"g3d/biddy_blue.brres",
		"g3d/biddy_pink.brres"
	};

	this->timer = 0;
	this->previousEyes = 0;
	this->targetEyes = 0;
	this->eyeChangeDelay = 0;

	int color = ((this->settings >> 28) & 0xF);
	this->dying = false;
	this->deathType = 0;
	this->currentNodeNum = 0;
	this->playerCollides = 0;
    this->currentNodeNum = (this->settings >> 8 & 0b11111111);
	this->reverseMode = false;

	allocator.link(-1, GameHeaps[0], 0, 0x20);

	this->resFile.data = getResource("biddy", brresNames[color]);
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("biddybud");
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);
	SetupTextures_Enemy(&bodyModel, 0);

	this->anmFile.data = getResource("biddy", brresNames[color]);
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("fly");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);

	bindEyesAnimation();
	allocator.unlink();

	ActivePhysics::Info HitMeBaby;
	HitMeBaby.xDistToCenter = 0.0f;
	HitMeBaby.yDistToCenter = 6.0f;
	HitMeBaby.xDistToEdge = 6.0f;
	HitMeBaby.yDistToEdge = 6.0f;
	HitMeBaby.category1 = 0x3;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0xFFBAFFFE;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();

	this->scale = (Vec){1.1f, 1.1f, 1.1f};
	this->rot.x = 0;
	this->rot.y = 0;
	this->rot.z = 0;

	bindAnimChr_and_setUpdateRate("fly", 1, 0.0f, 1.5f);
    doStateChange(&StateID_Init);
    
	this->onExecute();
	return true;
}

int dBiddyBud_c::onExecute() {
	updateModelMatrices();
	bodyModel._vf1C();

	acState.execute();

	if (acState.getCurrentState() == &dEnPath_c::StateID_Done) {
        executeReverseMovement();
    }

    if(acState.getCurrentState() == &dEnPath_c::StateID_Done && !this->reverseMode)
    {
		this->currentNodeNum = 0;
		doStateChange(&dEnPath_c::StateID_Init);
    }

	if(this->dying) return;
	
	this->rot.x = 0;
	this->rot.y = 0;
	this->rot.z = 0;
	updateEyesByMovement();

	if(this->chrAnimation.isAnimationDone())
		bindAnimChr_and_setUpdateRate("fly", 1, 0.0f, 1.5f);

	return true;
}

int dBiddyBud_c::onDraw() {
	bodyModel.scheduleForDrawing();
	return true;
}

int dBiddyBud_c::onDelete() {
	return true;
}

/* 		Die 		*/
void dBiddyBud_c::beginState_Die()
{
	this->removeMyActivePhysics();
	this->dying = true;
}
void dBiddyBud_c::executeState_Die()
{
	if(this->chrAnimation.isAnimationDone())
	{
		this->Delete(1);
	}
}
void dBiddyBud_c::endState_Die()
{}

void dBiddyBud_c::beginReverseSegment(int fromNodeNum, int toNodeNum) {
    currentNode = &course->railNode[rail->startNode + fromNodeNum];
    nextNode = &course->railNode[rail->startNode + toNodeNum];

    dx = nextNode->xPos - currentNode->xPos;
    dy = (-nextNode->yPos) - (-currentNode->yPos);

    float distanceToNext = sqrtf((dx * dx) + (dy * dy));
    ux = dx / distanceToNext;
    uy = dy / distanceToNext;

    stepVector.x = ux * speed;
    stepVector.y = uy * speed;

    rest = 1.0f - getDecimals(distanceToNext / speed);
    stepCount = floor(distanceToNext / speed);
    stepsDone = 0;

    pos.x = currentNode->xPos;
    pos.y = -currentNode->yPos;
}

void dBiddyBud_c::executeReverseMovement() {
    if (!rail || rail->nodeCount < 2)
        return;

    if (!reverseMode) {
        reverseMode = true;
        currentNodeNum = rail->nodeCount - 1;
        beginReverseSegment(currentNodeNum, currentNodeNum - 1);
    }

    if (stepsDone < stepCount) {
        stepsDone++;
        pos.x += stepVector.x;
        pos.y += stepVector.y;
        return;
    }

    currentNodeNum--;
    if (currentNodeNum <= 0) {
        reverseMode = false;
        currentNodeNum = 0;
        return;
    }

    beginReverseSegment(currentNodeNum, currentNodeNum - 1);
}