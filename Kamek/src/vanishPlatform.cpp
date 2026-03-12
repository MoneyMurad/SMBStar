#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

#include "path.h"

const char* VParcList[] = {
	"lift_vanish",
	NULL
};

class daVanishPlatform_c : public dEnPath_c {
	public:
		static dActor_c* build();
		
		int onCreate();
		int onExecute();
		int onDelete();
		int onDraw();
		
		StandOnTopCollider collider;
		
		nw4r::g3d::ResFile resFile;
		mHeapAllocator_c allocator;
		m3d::mdl_c model;
		
		nw4r::g3d::ResAnmTexPat anmPat;
		m3d::anmTexPat_c patAnimation;
		
		nw4r::g3d::ResAnmClr anmClr;
		
		m3d::anmClr_c platClr;
		
		void texClr_bindAnimChr_and_setUpdateRate(const char* name);
		void updateHitbox(float offset);
		
		int timer;
		int type;
		bool spawnedByEvent;
		int respawnType;
		int respawnTimer;
		int dispTimer;
		u64 triggerID;
		bool spawningIn;
		bool invisible;
		bool weAreMoving;
		float scaleAppearOffset;
		int flashAnimTimer;
		float originalScale;
		float scaleOffset;
		bool waitForPlayer;
		
		float initialPosx;
		float initialPosy;
		float initialPosz;
		
		USING_STATES(daVanishPlatform_c);
		DECLARE_STATE(Wait);
		DECLARE_STATE(Appear);
		DECLARE_STATE(Move);
		DECLARE_STATE(WaitDisp);
		DECLARE_STATE(Vanish);
};

CREATE_STATE(daVanishPlatform_c, Wait);
CREATE_STATE(daVanishPlatform_c, Appear);
CREATE_STATE(daVanishPlatform_c, Move);
CREATE_STATE(daVanishPlatform_c, WaitDisp);
CREATE_STATE(daVanishPlatform_c, Vanish);

const SpriteData VanishPlatformData = {ProfileId::vanishplatform, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile VanishPlatformProfile(&daVanishPlatform_c::build, SpriteId::vanishplatform, &VanishPlatformData, ProfileId::vanishplatform, ProfileId::vanishplatform, "VanishPlatform", VParcList);

dActor_c  *daVanishPlatform_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(daVanishPlatform_c));
	return new(buf) daVanishPlatform_c;
}

int daVanishPlatform_c::onCreate()
{
	//Size stuff
	//8.4 per block
	this->type = this->settings >> 8 & 0b11111111; 
	this->respawnType = this->settings >> 28 & 0b11;
	if(this->respawnType == 0)
		respawnTimer = 0;
	else if(this->respawnType == 1)
		respawnTimer = 60;
	else if(this->respawnType == 2)
		respawnTimer = 120;
	else if(this->respawnType == 3)
		respawnTimer = 240;

	// small, small*, medium, medium*, large, large*
	if(type == 0 || type == 1)
		this->originalScale = 25.2f;
	else if(type == 2 || type == 3)
		this->originalScale = 42.0f;
	else
		this->originalScale = 67.2f;
		
	spawnedByEvent = ((settings & 0xF000000) >> 24);

	allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	if(originalScale == 25.2f)
		this->resFile.data = getResource("lift_vanish", "g3d/lift_vanish_s.brres");
	else if(originalScale == 42.0f)
		this->resFile.data = getResource("lift_vanish", "g3d/lift_vanish.brres");
	else
		this->resFile.data = getResource("lift_vanish", "g3d/lift_vanish_l.brres");
	
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("lift_vanish");
	model.setup(mdl, &allocator, 0x108, 1, 0);

	SetupTextures_MapObj(&model, 0);
	
	nw4r::g3d::ResAnmClr anmRes = resFile.GetResAnmClr("vanish_warning");
    platClr.setup(mdl, anmRes, &allocator, 0, 1);
    platClr.bind(&model, anmRes, 0, 0);
	platClr.setUpdateRateForEntry(1.0f, 0);
    model.bindAnim(&platClr, 0.0f);
	
	allocator.unlink();
	
	this->scale = (Vec){0.0, 0.0, 0.0};	// We're starting invisible folks!
	this->initialPosx = this->pos.x;
	this->initialPosy = this->pos.y;
	this->initialPosz = this->pos.z;
	currentNodeNum = 0;
	this->invisible = true;
	this->weAreMoving = false;
	this->dispTimer = 61;
	this->flashAnimTimer = 0;
	
	char eventNum = (this->settings >> 8) & 0x000F;
	triggerID = (eventNum == 0) ? 0 : ((u64)1 << (eventNum - 1));

	OSReport("Event Num: %d\n", eventNum);
	OSReport("Trigger ID: %d\n", triggerID);
	
	//if(this->triggerID == 0)
	//	this->spawningIn = false;
	//else
	//	this->spawningIn = true;
	
	//this->triggerID = (this->triggerID == 0) ? 0 : ((u64)1 << (this->triggerID - 1));
	
	// Collider stuffs
	collider.init(this,
			/*xOffset=*/0.0f, /*yOffset=*/6.0f,
			/*topYOffset=*/0,
			/*rightSize=*/0, /*leftSize=*/0,
			/*rotation=*/0, /*_45=*/1
			);

	collider._47 = 0xA;
	collider.flags = 0x80180 | 0xC00;

	collider.addToList();
	
	doStateChange(&StateID_Wait);
	
	this->onExecute();
	return true;
}

int daVanishPlatform_c::onExecute()
{
	model._vf1C();
	acState.execute();
	
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	model.setDrawMatrix(matrix);
	model.setScale(&scale);
	model.calcWorld(false);

	//else
	//{
		dStateBase_c* currentState = this->acState.getCurrentState();
		if(weAreMoving && (currentState == &dEnPath_c::StateID_Done)) // Let's vanish!
		{
			this->timer++;

			// Wait the respawn timer before appearing
			if(this->timer >= this->respawnTimer)
			{
				weAreMoving = false;
				this->timer = 0;

				doStateChange(&StateID_WaitDisp);
			}
		}
		
		collider.update();
		
		if(flashAnimTimer)
			platClr.process();
	//}
	
	return true;
}

int daVanishPlatform_c::onDraw()
{
	model.scheduleForDrawing();
	return true;
}

int daVanishPlatform_c::onDelete()
{
	return true;
}

/*	Wait		*/
void daVanishPlatform_c::beginState_Wait()
{
	this->timer = 0;
	this->weAreMoving = false;
	this->scale = (Vec){0.0, 0.0, 0.0};
}
void daVanishPlatform_c::executeState_Wait()
{
	// Spawned through an event
	//u64 effectiveFlag = triggerID | spriteFlagMask;
	//if(this->triggerID > 0 && !dFlagMgr_c::instance->flags & effectiveFlag)
	//	return;

	if (this->spawnedByEvent && (dFlagMgr_c::instance->flags & spriteFlagMask) == 0)
		return;
	else
	{
		if(this->timer >= ((this->respawnTimer == 0) ? 0 : 60)) // lazy to remove
		{
			this->invisible = false;
			doStateChange(&StateID_Appear);
		}
	}
		
	this->timer++;
}
void daVanishPlatform_c::endState_Wait()
{}

/*	Appear	*/
void daVanishPlatform_c::beginState_Appear()
{
	this->timer = 0;
	
	this->pos.x = this->initialPosx;
	this->pos.y = this->initialPosy;
	this->pos.z = this->initialPosz;
	
	this->scaleOffset = (this->originalScale / 50.0f);
}
void daVanishPlatform_c::executeState_Appear()
{
	//doStateChange(&StateID_Move);
	this->timer++;
	this->scaleAppearOffset = timer * 0.05f;	// Takes a little over a second to get to full scale
	
	if(this->scaleAppearOffset >= 1)
	{
		this->scaleAppearOffset = 1.0f;
		
		collider.init(this,
			/*xOffset=*/0.0f, /*yOffset=*/6.0f,
			/*topYOffset=*/0,
			/*rightSize=*/originalScale, /*leftSize=*/(originalScale * -1),
			/*rotation=*/0, /*_45=*/1
			);
		
		collider._47 = 0xA;
		collider.flags = 0x80180 | 0xC00;

		collider.addToList();
		
		if(this->timer >= 50)
			doStateChange(&StateID_Move);
	}
	
	this->scale = (Vec){this->scaleAppearOffset, this->scaleAppearOffset, this->scaleAppearOffset};
}
void daVanishPlatform_c::endState_Appear()
{}

/*	Move		*/
void daVanishPlatform_c::beginState_Move()
{
	this->timer = 0;
}
void daVanishPlatform_c::executeState_Move()
{
	dStateBase_c* currentState = this->acState.getCurrentState();

	// If we're not moving and the path hasn't started yet, start it
	if(!weAreMoving && (currentState != &dEnPath_c::StateID_Init)) 
	{
		doStateChange(&StateID_Init);
		weAreMoving = true;
	}
}
void daVanishPlatform_c::endState_Move()
{}

/*	WaitDisp		*/
void daVanishPlatform_c::beginState_WaitDisp()
{
	this->timer = 0;
}
void daVanishPlatform_c::executeState_WaitDisp()
{
	if(this->timer <= 60)
		this->flashAnimTimer = 1;
	else
		this->flashAnimTimer = 0;
	
	if(this->timer >= this->dispTimer)
	{
		doStateChange(&StateID_Vanish);
	}
	
	this->timer += 1;
}
void daVanishPlatform_c::endState_WaitDisp()
{
	platClr.setFrameForEntry(0, 0);
}


/*	Vanish		*/
void daVanishPlatform_c::beginState_Vanish()
{
	this->timer = 0;
	this->weAreMoving = false;
	this->scaleOffset = (this->originalScale / 50.0f);
	
	collider.removeFromList();
}
void daVanishPlatform_c::executeState_Vanish()
{
	//doStateChange(&StateID_Wait);
	this->timer++;
	this->scaleAppearOffset = timer * 0.05f;
	
	if(this->scaleAppearOffset >= 1)	// 50 iterations when offset is 0.02
	{
		this->scale = (Vec){0.0, 0.0, 0.0};
		
		doStateChange(&StateID_Wait);
	}
	else
	{
		this->scale = (Vec){1.0f - this->scaleAppearOffset, 1.0f - this->scaleAppearOffset, 1.0f - this->scaleAppearOffset};
	}
}
void daVanishPlatform_c::endState_Vanish()
{}

void daVanishPlatform_c::updateHitbox(float offset)
{
	collider.init(this,
	
		/*xOffset=*/0.0f, /*yOffset=*/6.0f,
		/*topYOffset=*/0,
		/*rightSize=*/(originalScale - offset), /*leftSize=*/-(originalScale - offset),
		/*rotation=*/0, /*_45=*/1
			
	);
}