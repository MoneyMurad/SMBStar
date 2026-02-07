/*
    Perching bird that flies away when close
*/

#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

const char* BDarc[] = {"obj_bird", NULL};

class dBird_c : public daBoss {
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
    bool fliesLeft;

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
	
	USING_STATES(dBird_c);
	DECLARE_STATE(Wait);
	DECLARE_STATE(PrepareFly);
    DECLARE_STATE(Fly);
};

CREATE_STATE(dBird_c, Wait);
CREATE_STATE(dBird_c, PrepareFly);
CREATE_STATE(dBird_c, Fly);

const SpriteData BirdData = {ProfileId::bird, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile BirdProfile(&dBird_c::build, SpriteId::bird, &BirdData, ProfileId::bird, ProfileId::bird, "bird", BDarc);

dActor_c* dBird_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dBird_c));
	return new(buf) dBird_c;
}

void dBird_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	return;
}
void dBird_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	playerCollision(apThis, apOther);
}
void dBird_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) 
{
	return;
}
bool dBird_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dBird_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dBird_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dBird_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}
bool dBird_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dBird_c::collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dBird_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
	return true;
}
bool dBird_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dBird_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dBird_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool dBird_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	playerCollision(apThis, apOther);
	return true;
}

void dBird_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

void dBird_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}


float dBird_c::nearestPlayerDistance() {
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

int dBird_c::onCreate()
{
    allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("obj_bird", "g3d/obj_bird.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("obj_bird");
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);

	SetupTextures_MapObj(&bodyModel, 0);
	
    this->anmFile.data = getResource("obj_bird", "g3d/obj_bird.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("idle");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);

	allocator.unlink();

    this->fliesLeft = (settings & 0x2000);

    this->rot.y = (this->fliesLeft) ? 0x2800 : 0xD800;
    
    this->scale = (Vec){1.0, 1.0, 1.0};
    bindAnimChr_and_setUpdateRate("idle", 1, 0.0, 1.0); 
    doStateChange(&StateID_Wait);

    this->onExecute();
    return true;
}

int dBird_c::onDraw()
{
    bodyModel.scheduleForDrawing();
    return true;
}

int dBird_c::onExecute()
{
    acState.execute();
    updateModelMatrices();
    bodyModel._vf1C();

    return true;
}

int dBird_c::onDelete()
{
    return true;
}

// for waiting around like a bummy bird
void dBird_c::beginState_Wait() {
	this->timer = 0;
}
void dBird_c::executeState_Wait()
{
    if(this->chrAnimation.isAnimationDone())
	{
        this->chrAnimation.setCurrentFrame(0.0);
	}

	this->timer++;
	if(this->nearestPlayerDistance() <= 200.0f && this->timer >= 200)
	{
		static nw4r::snd::StrmSoundHandle handle;
		int random = GenerateRandomNumber(3);

		if(random == 0)
			PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SFX_BIRD_CHIRP_1, 1);
		else if(random == 1)
			PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SFX_BIRD_CHIRP_2, 1);
		else if(random == 2)
			PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SFX_BIRD_CHIRP_3, 1);

		this->timer = 0;
	}

    if(this->nearestPlayerDistance() <= 64.0f)
        doStateChange(&StateID_PrepareFly);
}
void dBird_c::endState_Wait() {}

// bounce around like a fool
void dBird_c::beginState_PrepareFly() 
{
    this->rot.y = (fliesLeft) ? 0xD800 : 0x2800;
    bindAnimChr_and_setUpdateRate("fly_st", 1, 0.0, 1.0); 
}
void dBird_c::executeState_PrepareFly()
{
    if(this->chrAnimation.isAnimationDone())
        doStateChange(&StateID_Fly);
}
void dBird_c::endState_PrepareFly() {}

// fly away like a beautiful bird
void dBird_c::beginState_Fly()
{
    bindAnimChr_and_setUpdateRate("fly", 1, 0.0, 0.75); 

    this->speed.x = (this->fliesLeft) ? -2.0 : 2.0;
    this->speed.y = 1.0;
    this->max_speed.y = 1.0f;

    //this->y_speed_inc = 0.005;

	static nw4r::snd::StrmSoundHandle handle;
	PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SFX_BIRD_FLY, 1);

    this->timer = 0;
}
void dBird_c::executeState_Fly()
{
    HandleXSpeed();
    HandleYSpeed();
    doSpriteMovement();

	if(this->speed.y <= 2.5)
	{
		this->speed.y += 0.1;
		this->max_speed.y += 0.1;
	}

    if(this->chrAnimation.isAnimationDone())
        this->chrAnimation.setCurrentFrame(0.0);

    if(this->timer >= 500)
        this->Delete(1);

    this->timer++;
}
void dBird_c::endState_Fly() {}
