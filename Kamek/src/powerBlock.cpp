#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>

const char *PowerBlockArc[] = {"obj_power_block", 0};

class daPowerBlock_c : public daEnBlockMain_c {
	Physics::Info physicsInfo;

	int onCreate();
	int onDelete();
	int onExecute();
	int onDraw();
		
	bool isHit;
    int dir;
	
	void calledWhenUpMoveExecutes();
	void calledWhenDownMoveExecutes();

	void blockWasHit(bool isDown);

    void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);

	mHeapAllocator_c allocator;

	nw4r::g3d::ResFile resFile;
    nw4r::g3d::ResFile anmFile;
    m3d::anmChr_c chrAnimation;
	m3d::mdl_c model;

	USING_STATES(daPowerBlock_c);
	DECLARE_STATE(Wait);
	DECLARE_STATE(Hit);

	public: static dActor_c *build();
};


CREATE_STATE(daPowerBlock_c, Wait);
CREATE_STATE(daPowerBlock_c, Hit);

const SpriteData PowerBlockData = {ProfileId::powerblock, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile PowerBlockProfile(&daPowerBlock_c::build, SpriteId::powerblock, &PowerBlockData, ProfileId::powerblock, ProfileId::powerblock, "powerblock", PowerBlockArc);

extern "C" int CheckExistingPowerup(void * Player);

dActor_c  *daPowerBlock_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(daPowerBlock_c));
	return new(buf) daPowerBlock_c;
}

void daPowerBlock_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->model, anmChr, unk);
	this->model.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

int daPowerBlock_c::onCreate() {
	allocator.link(-1, GameHeaps[0], 0, 0x20);

	this->resFile.data = getResource("obj_power_block", "g3d/obj_power_block.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("obj_power_block");
	model.setup(mdl, &allocator, 0x108, 1, 0);

    SetupTextures_MapObj(&model, 0);
	
    this->anmFile.data = getResource("obj_power_block", "g3d/obj_power_block.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("spawn");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);

	allocator.unlink();

	blockInit(pos.y);

	physicsInfo.x1 = -20;
	physicsInfo.y1 = 20;
	physicsInfo.x2 = 20;
	physicsInfo.y2 = -20;

	physicsInfo.otherCallback1 = &daEnBlockMain_c::OPhysicsCallback1;
	physicsInfo.otherCallback2 = &daEnBlockMain_c::OPhysicsCallback2;
	physicsInfo.otherCallback3 = &daEnBlockMain_c::OPhysicsCallback3;

	physics.setup(this, &physicsInfo, 3, currentLayerID);
	physics.flagsMaybe = 0x260;
	physics.callback1 = &daEnBlockMain_c::PhysicsCallback1;
	physics.callback2 = &daEnBlockMain_c::PhysicsCallback2;
	physics.callback3 = &daEnBlockMain_c::PhysicsCallback3;
	physics.addToList();

	this->isHit = false;
	
	doStateChange(&daPowerBlock_c::StateID_Wait);
	this->onExecute();

	return true;
}


int daPowerBlock_c::onDelete() {
	physics.removeFromList();

	return true;
}


int daPowerBlock_c::onExecute() {
	model._vf1C();
	acState.execute();
	physics.update();
	
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);
	
    this->model.setDrawMatrix(matrix);
    this->model.setScale(&scale);
    this->model.calcWorld(false);
	
    if(this->isHit)
    {
        if(this->chrAnimation.isAnimationDone()) {
            Vec spawnPos = this->pos;
            spawnPos.y -= 32.0f;
            dStageActor_c* daShroom = (dStageActor_c*)CreateActor(779, 0, spawnPos, 0, 0);

            if(!this->dir) daShroom->pos.y -= 32.0f;
			daShroom->speed.y = (this->dir) ? 7.0f : -7.0f;
			daShroom->speed.x = 2.5f;
            this->Delete(0);
        }
    }

	this->blockUpdate();

	//if (acState.getCurrentState()->isEqual(&StateID_Wait)) {
	//	checkZoneBoundaries(0);
	//}

	return true;
}


int daPowerBlock_c::onDraw() 
{
	this->model.scheduleForDrawing();

	return true;
}

void daPowerBlock_c::blockWasHit(bool isDown) {
	pos.y = initialY;
}

void daPowerBlock_c::calledWhenUpMoveExecutes() {
	if (initialY >= pos.y)
		blockWasHit(false);
}

void daPowerBlock_c::calledWhenDownMoveExecutes() {
	if (initialY <= pos.y)
		blockWasHit(true);
}

void daPowerBlock_c::beginState_Wait() {
}

void daPowerBlock_c::endState_Wait() {
}

void daPowerBlock_c::executeState_Wait() {
	int result = blockResult();

	if (result == 0)
		return;

    bindAnimChr_and_setUpdateRate("break", 1, 0.0, 1.0);
    this->isHit = true;
    
	if (result == 1) {
        dir = 1;

		doStateChange(&daEnBlockMain_c::StateID_UpMove);
		anotherFlag = 2;
		isGroundPound = false;
	} else {
        dir = 0;

		doStateChange(&daEnBlockMain_c::StateID_DownMove);
		anotherFlag = 1;
		isGroundPound = true;
	}
}


void daPowerBlock_c::beginState_Hit() {
}
void daPowerBlock_c::executeState_Hit() {
	
}
void daPowerBlock_c::endState_Hit() {
}
