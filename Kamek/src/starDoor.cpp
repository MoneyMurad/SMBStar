#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

/*
	64 Stardoor
	Door whose key is the amount of Stars collected by the Player.
*/

const char* StarDoorFileList[] = {"obj_stardoor", NULL};

class daStarDoor_c : public dEn_c {
    int onCreate();
    int onExecute();
    int onDelete();
    int onDraw();

	public: static dActor_c *build();

    mHeapAllocator_c allocator;

	nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile anmFile;
	m3d::mdl_c bodyModel;
	m3d::anmChr_c chrAnimation;
	
	nw4r::g3d::ResAnmTexPat anmPat;
	m3d::anmTexPat_c patAnimation;

	int world_num, level_num;
    int timer;
	int texState;
	int texTimer;
    int starsNeeded;

	int lastCheck;
    int starResult;

	bool doIt;

    int checkPlayerStarSituation();

    void playerCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther);

	bool collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther);
	// bool collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat11_PipeCannon(ActivePhysics *apThis, ActivePhysics *apOther);

	bool checkPlayerInFrontAndStatus();
	
	void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	void texPat_bindAnimChr_and_setUpdateRate(const char* name);

    USING_STATES(daStarDoor_c);
    DECLARE_STATE(Wait);
    DECLARE_STATE(CheckForStarsAndPotentiallyUnlock);
    DECLARE_STATE(Open);
    DECLARE_STATE(Close);
};


CREATE_STATE(daStarDoor_c, Wait);
CREATE_STATE(daStarDoor_c, CheckForStarsAndPotentiallyUnlock);
CREATE_STATE(daStarDoor_c, Open);
CREATE_STATE(daStarDoor_c, Close);

const SpriteData StarDoorData = {ProfileId::StarDoor, 8, 0, 0, 0, 0x100, 0x100, 0x40, 0x40, 0, 0, 0};
Profile StarDoorProfile(&daStarDoor_c::build, SpriteId::StarDoor, &StarDoorData, ProfileId::StarDoor, ProfileId::StarDoor, "StarDoor", StarDoorFileList);

dActor_c* daStarDoor_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(daStarDoor_c));
	return new(buf) daStarDoor_c;
}

void daStarDoor_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	return;
}
void daStarDoor_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther) {
	this->playerCollision(apThis, apOther);
}

bool daStarDoor_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) {
	return false;
}
bool daStarDoor_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
	return this->collisionCatD_Drill(apThis, apOther);
}
bool daStarDoor_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
	return this->collisionCatD_Drill(apThis, apOther);
}
bool daStarDoor_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
	return this->collisionCatD_Drill(apThis, apOther);
}
bool daStarDoor_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther){
	return this->collisionCatD_Drill(apThis, apOther);
}
bool daStarDoor_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther){
	return this->collisionCatD_Drill(apThis, apOther);
}
bool daStarDoor_c::collisionCat11_PipeCannon(ActivePhysics *apThis, ActivePhysics *apOther){
	return this->collisionCatD_Drill(apThis, apOther);
}
bool daStarDoor_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
	return this->collisionCatD_Drill(apThis, apOther);
}

bool daStarDoor_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther){
	return false;
}

bool daStarDoor_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther){
	return false;
}

bool daStarDoor_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
	/*SpawnEffect("Wm_en_explosion", 0, &this->pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});
	SpawnEffect("Wm_en_explosion_smk", 0, &this->pos, &(S16Vec){0,0,0}, &(Vec){3.0, 3.0, 3.0});

	PlaySound(this, SE_OBJ_EMY_FIRE_DISAPP);*/
	
	return true;
}

bool daStarDoor_c::collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther) { 
	/*SpawnEffect("Wm_ob_cmnicekira", 0, &this->pos, &(S16Vec){0,0,0}, &(Vec){1.5, 1.5, 1.5});
	SpawnEffect("Wm_ob_icebreakwt", 0, &this->pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});
	SpawnEffect("Wm_ob_iceattack", 0, &this->pos, &(S16Vec){0,0,0}, &(Vec){1.5, 1.5, 1.5});

	PlaySound(this, SE_OBJ_PNGN_ICE_BREAK);*/

	return true; 
}

int daStarDoor_c::onCreate() {
	starsNeeded = this->settings >> 0 & 0b11111111;

    allocator.link(-1, GameHeaps[0], 0, 0x20);

	this->resFile.data = getResource("obj_stardoor", "g3d/obj_stardoor_1.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("obj_stardoor");
	bodyModel.setup(mdl, &allocator, 0x227, 1, 0);

	SetupTextures_MapObj(&bodyModel, 0);

	// Load animation file
	this->anmFile.data = getResource("obj_stardoor", "g3d/obj_stardoor_1.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("close");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);
	
	/* Setup texture pattern animation
	this->anmPat = this->resFile.GetResAnmTexPat("switch");
	this->patAnimation.setup(mdl, anmPat, &this->allocator, 0, 1);
	this->patAnimation.bindEntry(&this->bodyModel, &anmPat, 0, 1);
	this->patAnimation.setFrameForEntry(stardoorID, 0);
	this->patAnimation.setUpdateRateForEntry(0.0f, 0);
	this->bodyModel.bindAnim(&this->patAnimation);*/

    allocator.unlink();

	world_num = ((this->settings >> 28 & 0xF) - 1);
	level_num = ((this->settings >> 8 & 0b11111111) - 1);

    starResult = checkPlayerStarSituation();

	OSReport("World %d Level %d\n", world_num, level_num);
    OSReport("Stars Collected %d\n", starResult);
    OSReport("Stars Needed %d\n", starsNeeded);

	ActivePhysics::Info HitMeBaby;

    // Dist from center
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 8.0;
	
	// Size
	HitMeBaby.xDistToEdge = 35.0;
	HitMeBaby.yDistToEdge = 60.0;
	
	HitMeBaby.category1 = 0x5;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0x200;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();
	
	doIt = false;
	
	lastCheck = 0;

	// Initialize texture animation variables
	this->texState = 0;
	this->texTimer = 0;

    doStateChange(&StateID_Wait);
	return true;
}
int daStarDoor_c::onExecute() {
	acState.execute();
	updateModelMatrices();
	bodyModel._vf1C();

	checkPlayerInFrontAndStatus();

	if(doIt)
		doStateChange(&StateID_CheckForStarsAndPotentiallyUnlock);

    return true;
}

int daStarDoor_c::onDelete() {
	return true;
}

int daStarDoor_c::onDraw() {
	bodyModel.scheduleForDrawing();

    return true;
}

bool daStarDoor_c::checkPlayerInFrontAndStatus() {
    for (int i = 0; i < 4; i++) {
        if (dAcPy_c *player = dAcPy_c::findByID(i)) {
            if(player->pos.x >= pos.x - 35 && player->pos.x <= pos.x + 35) {
				lastCheck = 1;

				Remocon* con = GetRemoconMng()->controllers[player->settings % 4];

				bool correctState = (
					strcmp(player->states2.getCurrentState()->getName(), "dAcPy_c::StateID_Walk") == 0 || 
					strcmp(player->states2.getCurrentState()->getName(), "daPlBase_c::StateID_Walk") == 0 || 
					strcmp(player->states2.getCurrentState()->getName(), "dAcPy_c::StateID_Turn") == 0
				);

				OSReport("OUR CURRENT STATE NYOO HOO HOO %s", player->states2.getCurrentState()->getName());

				if(strcmp(this->acState.getCurrentState()->getName(), "daStarDoor_c::StateID_Wait") == 0 && 
					((con->heldButtons & WPAD_UP) && correctState)) 
				{
					doStateChange(&StateID_CheckForStarsAndPotentiallyUnlock);
				}
            }
			else
				lastCheck = 0;
        }
    }

    return true;
}

/*  WAIT    */
void daStarDoor_c::beginState_Wait() {}
void daStarDoor_c::executeState_Wait() {
}
void daStarDoor_c::endState_Wait() {}


/*  STARCHECK / UNLOCK    */
int daStarDoor_c::checkPlayerStarSituation()
{
    int stars = 0;

    SaveBlock *save = GetSaveFile()->GetBlock(-1);
    
    for(int w = 0; w <= 9; w++)
        for(int l = 0; l <= 41; l++)
            if(save->GetLevelCondition(w-1, l-1) & 0x10) // Nomral exit complete
                stars++;
    
    return stars - 9; // there are automatically 9 flagged "completed" levels in a fresh save
}
void daStarDoor_c::beginState_CheckForStarsAndPotentiallyUnlock() {
}
void daStarDoor_c::executeState_CheckForStarsAndPotentiallyUnlock() {
    if(starResult >= starsNeeded)
        doStateChange(&StateID_Open);
}
void daStarDoor_c::endState_CheckForStarsAndPotentiallyUnlock() {
}


/*  OPEN    */
void daStarDoor_c::beginState_Open() {
    this->timer = 0;
    bindAnimChr_and_setUpdateRate("open", 1, 0.0, 1.0); 
}
void daStarDoor_c::executeState_Open() {
	if(this->chrAnimation.isAnimationDone())
        this->timer++;

    if(this->timer >= 45)
        doStateChange(&StateID_Close);
}
void daStarDoor_c::endState_Open() {}


/*  CLOSE    */

void daStarDoor_c::beginState_Close() {
    this->timer = 0;
    bindAnimChr_and_setUpdateRate("close", 1, 0.0, 1.0); 
}
void daStarDoor_c::executeState_Close() {
	if(this->chrAnimation.isAnimationDone())
        this->timer++;

    if(this->timer >= 45)
        doStateChange(&StateID_Wait);
}
void daStarDoor_c::endState_Close() {}

void daStarDoor_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

void daStarDoor_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

void daStarDoor_c::texPat_bindAnimChr_and_setUpdateRate(const char* name) {
	this->anmPat = this->resFile.GetResAnmTexPat(name);
	this->patAnimation.bindEntry(&this->bodyModel, &anmPat, 0, 1);
	this->bodyModel.bindAnim(&this->patAnimation);
}

/*
extern "C" void someFlightRelatedFunction(void *player);

int playerTime;
int originalX;
int originalY;

void daPlBase_c::beginState_PaintingJump() {
	dPlayerModelHandler_c *pmh = (dPlayerModelHandler_c*)(((u32)this) + 0x2A60);
    pmh->mdlClass->startAnimation(5, 1.0f, 0.0f, 0.0f); // Spin

	this->rot.y = 0x8000;

	playerTime = 0;
	stopChecking = true;

	originalX, originalY = -1;
	originalX = this->pos.x;

	this->speed.y = 6.0f;
}
void daPlBase_c::executeState_PaintingJump() {
	someFlightRelatedFunction(this);
	this->speed.x = 0.0;

	this->pos.x = originalX;

	playerTime += 1;

	if(playerTime >= 25)
	{
		if(originalY == -1)
			originalY = this->pos.y;
		else
			this->pos.y = originalY;

		this->speed.y = 0;

		this->pos.y = -200.0f; // vanish
	}
	else
		this->speed.y -= 0.002f;
}
void daPlBase_c::endState_PaintingJump() {}
*/
