#include <game.h>
#include <common.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>

const char* SpinDriftList[] = {"fuwa", NULL};

class daSpinDrift_c : public dEn_c {
    public:
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
        int flightType; // 0 = idle, 1 = left / right, 2: up / down
        bool startsRight;

        bool isDone;
        bool weAreMoving;
        int yDirection;

        static dActor_c *build();

        void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	    void updateModelMatrices();
        void bouncePlayerWhenJumpedOn(void *player);
        float getNearestPlayerY();
        
        void playerCollision(ActivePhysics *apThis, ActivePhysics *apOther);

        bool collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther);

        bool collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther);
        bool collisionCat11_PipeCannon(ActivePhysics *apThis, ActivePhysics *apOther);

        USING_STATES(daSpinDrift_c);
        DECLARE_STATE(Wait);

        DECLARE_STATE(Fly);

        DECLARE_STATE(Turn);
        DECLARE_STATE(Die);
};

CREATE_STATE(daSpinDrift_c, Wait);
CREATE_STATE(daSpinDrift_c, Fly);
CREATE_STATE(daSpinDrift_c, Turn);
CREATE_STATE(daSpinDrift_c, Die);

// Add state to the player
CREATE_STATE(daPlBase_c, SpinFly);

// Add spin drift to the actor profile
const SpriteData spinDriftSpriteData = { ProfileId::SpinDrift, 0, 0 , 0 , 0, 4, 4, 0, 0, 0, 0, 0 };
Profile spinDriftProfile(&daSpinDrift_c::build, SpriteId::SpinDrift, &spinDriftSpriteData, ProfileId::SpinDrift, ProfileId::SpinDrift, "SpinDrift", SpinDriftList);

void bouncePlayerWhenJumpedOn(void *player);
extern "C" void someFlightRelatedFunction(void *player);
extern "C" void playSomeAnimBlahBlah(daPlBase_c *player);
extern "C" void hipAttackStuffizoids(daPlBase_c *player);

dActor_c* daSpinDrift_c::build() {
    void *buffer = AllocFromGameHeap1(sizeof(daSpinDrift_c));
    return new(buffer) daSpinDrift_c;
}

void daSpinDrift_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	char hitType = usedForDeterminingStatePress_or_playerCollision(this, apThis, apOther, 0);
	
	if(hitType == 1 | hitType == 3)
	{
		apOther->someFlagByte |= 2;

        // Set player to our spinfly state
        daPlBase_c *player = (daPlBase_c*)apOther->owner;
        player->states2.setState(&daPlBase_c::StateID_SpinFly);

        StageE4::instance->spawnCoinJump(pos, 0, 1, 0);
		doStateChange(&StateID_DieSmoke);
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

bool daSpinDrift_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) { 
		PlaySound(this, SE_EMY_DOWN); 
		SpawnEffect("Wm_mr_hardhit", 0, &pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});

        daPlBase_c *player = (daPlBase_c*)apOther->owner;
        player->states2.setState(&daPlBase_c::StateID_SpinFly);

        StageE4::instance->spawnCoinJump(pos, 0, 1, 0);
		doStateChange(&StateID_DieSmoke);
		return true;
}
bool daSpinDrift_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
    PlaySound(this, SE_EMY_DOWN); 
	SpawnEffect("Wm_mr_hardhit", 0, &pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});

    doStateChange(&StateID_DieSmoke);
    return true;
}
bool daSpinDrift_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
    return this->collisionCat7_GroundPound(apThis, apOther);
}
bool daSpinDrift_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
    return this->collisionCat7_GroundPound(apThis, apOther);
}
bool daSpinDrift_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther){
    return this->collisionCat7_GroundPound(apThis, apOther);
}
bool daSpinDrift_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther){
    return this->collisionCat7_GroundPound(apThis, apOther);
}
bool daSpinDrift_c::collisionCat11_PipeCannon(ActivePhysics *apThis, ActivePhysics *apOther){
    return this->collisionCat7_GroundPound(apThis, apOther);
}
bool daSpinDrift_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
    return this->collisionCat7_GroundPound(apThis, apOther);
}
bool daSpinDrift_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther){
    bool rtrn = dEn_c::collisionCat3_StarPower(apThis, apOther);
    doStateChange(&StateID_Die);
    return rtrn;
}
bool daSpinDrift_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther){
    doStateChange(&StateID_DieSmoke);
    return true;
}
bool daSpinDrift_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
    StageE4::instance->spawnCoinJump(pos, 0, 1, 0);
    doStateChange(&StateID_DieSmoke);
    return true;
}

//void daSpinDrift_c::bouncePlayerWhenJumpedOn(void *player) {
//		bouncePlayer(player, 5.5f);
//}

float daSpinDrift_c::getNearestPlayerY() {
	for (int i = 0; i < 4; i++) {
		if (dAcPy_c *player = dAcPy_c::findByID(i)) {
			if (strcmp(player->states2.getCurrentState()->getName(), "dAcPy_c::StateID_Balloon")) {
				return player->pos.y;
			}
		}
	}
	return pos.y;
}


void daSpinDrift_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

void daSpinDrift_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}

int daSpinDrift_c::onCreate() {
    this->timer = 0;
    this->flightType = this->settings >> 28 & 0xF; // nybble 5
    this->startsRight = ((settings & 0xF000000) >> 24);

    this->weAreMoving = false;

    allocator.link(-1, GameHeaps[0], 0, 0x20);

	this->resFile.data = getResource("fuwa", "g3d/fuwa.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("fuwa");
	bodyModel.setup(mdl, &allocator, 0x224, 1, 0);

	SetupTextures_Enemy(&bodyModel, 0);

	this->anmFile.data = getResource("fuwa", "g3d/fuwa.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("fly");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);

	allocator.unlink();

    ActivePhysics::Info HitMeBaby;
	
	// Dist from center
	HitMeBaby.xDistToCenter = 0.0;
	HitMeBaby.yDistToCenter = 8.0;
	
	// Size
	HitMeBaby.xDistToEdge = 7.0;
	HitMeBaby.yDistToEdge = 7.0;
	
	HitMeBaby.category1 = 0x3;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0xFFBAFFFE;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();

    this->scale = (Vec){1.5f, 1.5f, 1.5f};

    this->rot.x = 0;
    if(this->flightType == 2)
        this->rot.y = 0;
    else
        this->rot.y = 0xD800;
    
    this->rot.z = 0;
    this->direction = (this->startsRight) ? 0 : 1; // R / L
    this->yDirection = (this->startsRight) ? 0 : 1; // D / U

    bindAnimChr_and_setUpdateRate("fly", 1, 0.0, 1.0); 

    if(this->flightType == 0)
        doStateChange(&StateID_Wait);
    else
        doStateChange(&StateID_Fly);

    this->isDone = false;

    this->onExecute();
    return true;
}

int daSpinDrift_c::onDelete() {
    return true;
}

int daSpinDrift_c::onDraw() {
    bodyModel.scheduleForDrawing();
    return true;
}

int daSpinDrift_c::onExecute() {
    acState.execute();
    updateModelMatrices();
    bodyModel._vf1C();

    return true;
}


/* Wait State */
void daSpinDrift_c::beginState_Wait() {
    this->timer = 0;
}
void daSpinDrift_c::executeState_Wait() {
    u8 facing = dSprite_c__getXDirectionOfFurthestPlayerRelativeToVEC3(this, this->pos);

    // Only look at player when idle
    if(this->flightType == 0)
    {
        if(facing != this->direction)
        {
            this->direction = facing;
            doStateChange(&StateID_Turn);
        }
    }

    if(this->chrAnimation.isAnimationDone()) {
        this->chrAnimation.setCurrentFrame(0.0);
    }

    // If we're not idle, then we have somewhere to be!
    if(this->flightType != 0 && this->timer >= 45)
        doStateChange(&StateID_Fly);

    this->timer++;
}
void daSpinDrift_c::endState_Wait() {
}


/* Fly State */
void daSpinDrift_c::beginState_Fly() {
    this->timer = 0;

    weAreMoving = true;

    if(this->flightType == 1)
    {
        this->max_speed.x = (direction) ? -0.5f : 0.5f;
	    this->speed.x = (direction) ? -0.5f : 0.5f;
    }
	else if(this->flightType == 2)
    {
        if(this->startsRight)
        {
            this->max_speed.y = -0.5f;
            this->speed.y = -0.5f;
        }
        else
        {
            this->max_speed.y = 0.5f;
            this->speed.y = 0.5f;
        }
    }
	else
    {
        this->max_speed.y = 0.0f;
	    this->speed.y = 0.0f;
	    this->y_speed_inc = 0.0f;
    }
}
void daSpinDrift_c::executeState_Fly() {
    this->speed.y = 0.0f;

    // Left right
    if(this->flightType == 1)
    {
        if(this->timer < 220)
        {
            HandleXSpeed();
            doSpriteMovement();
        }
        else if(this->timer >= 225)
        {
            this->direction ^= 1;
            doStateChange(&StateID_Turn);
        }
    }
    else if(this->flightType == 2)
    {
        if(this->timer < 220)
        {
            HandleYSpeed();
            doSpriteMovement();
        }
        else
        {
            this->speed.y = this->speed.y - (2 * this->speed.y);

            this->timer = 0;
        }
    }

    if(this->chrAnimation.isAnimationDone()) {
        this->chrAnimation.setCurrentFrame(0.0);
    }

    this->timer++;
}
void daSpinDrift_c::endState_Fly() {} 


/* Turn State */
void daSpinDrift_c::beginState_Turn(){}
void daSpinDrift_c::executeState_Turn()
{
	u16 rotationAmount = (this->direction) ? 0xD800 : 0x2800;
	int weDoneHere = SmoothRotation(&this->rot.y, rotationAmount, 0x400);
	
	if(weDoneHere)
		doStateChange(&StateID_Wait);

    if(this->chrAnimation.isAnimationDone()) {
        this->chrAnimation.setCurrentFrame(0.0);
    }
}
void daSpinDrift_c::endState_Turn()
{}


/*  Die State */
void daSpinDrift_c::beginState_Die() {
    this->removeMyActivePhysics();
    bindAnimChr_and_setUpdateRate("death", 1.0, 0.0, 0.5);
}
void daSpinDrift_c::executeState_Die() {
    if(chrAnimation.isAnimationDone()) {
        StageE4::instance->spawnCoinJump(pos, 0, 1, 0);

        this->Delete(1);
    }
}
void daSpinDrift_c::endState_Die() {}

/* Player States */
bool playerIsFallingState;
int playerFlyTime;

void daPlBase_c::changePlayerAnimation(daPlBase_c *player)
{
    return;
}

void daPlBase_c::beginState_SpinFly() {
    this->speed.y = 6.0f;

    playerFlyTime = 0;
    playerIsFallingState = false;

    // Start our anim
    dPlayerModelHandler_c *pmh = (dPlayerModelHandler_c*)(((u32)this) + 0x2A60);
    pmh->mdlClass->startAnimation(71, 1.0f, 0.0f, 0.0f); // Spin
}

void daPlBase_c::executeState_SpinFly() {
    
    someFlightRelatedFunction(this); // Handles x movement apparently
    //changePlayerAnimation(this);
    //hipAttackStuffizoids(this);

    int spinCounter = 7;
    
    // Already boosted up, now fall slowly
    if(playerIsFallingState)
    {
        if(this->speed.y < -1.0f)
            //this->speed.y -= 0.0025f;
        //else
            this->speed.y = -0.35f;

        // Only allow drill if we're in terminal velocity
        if(this->speed.y == -0.35f)
        {
            dPlayerModelHandler_c *pmh = (dPlayerModelHandler_c*)(((u32)this) + 0x2A60);

            // Get our controller
            Remocon* con = GetRemoconMng()->controllers[this->settings % 4];
            
            // Drill?
            if(con->heldButtons & WPAD_DOWN)
            {
                this->speed.y = -4.35f; // Drill.

                pmh->mdlClass->startAnimation(75, 1.0f, 0.0f, 0.0f); // Drill Spin
            }
            else
            {
                this->speed.y = -0.35f;

                pmh->mdlClass->startAnimation(71, 1.0f, 0.0f, 0.0f); // Spin
            }
        }

        spinCounter = 3;
    }
    else
    {   
        // Gravity
        if(this->speed.y <= 1.0f)
            playerIsFallingState = true;
        else if(playerFlyTime <= 18)
            this->speed.y = 6.0f;
        else if(playerFlyTime > 18 && playerFlyTime <= 28)
            this->speed.y = 3.5f;
        else if(playerFlyTime > 28 && playerFlyTime <= 32)
            this->speed.y = 1.5f;
        //else
        //    this->speed.y -= 0.005f;
    }

    // Is our player grounded?
    if(this->collMgr.isOnTopOfTile())
        this->states2.setState(&daPlBase_c::StateID_Jump);  // End spin fly

    // Spin!
    rot.y += (0x800 * spinCounter);

    // Increment timer
    playerFlyTime++;
}

void daPlBase_c::endState_SpinFly() {
    playerIsFallingState = false;
    playerFlyTime = 0;
}
