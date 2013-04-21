#include "StdInc.h"
#include "../lib/NetPacks.h"

#include "../lib/filesystem/CResourceLoader.h"
#include "../lib/filesystem/CFileInfo.h"
#include "../CCallback.h"
#include "Client.h"
#include "CPlayerInterface.h"
#include "CGameInfo.h"
#include "../lib/Connection.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CDefObjInfoHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/mapping/CMap.h"
#include "../lib/VCMIDirs.h"
#include "../lib/CSpellHandler.h"
#include "CSoundBase.h"
#include "mapHandler.h"
#include "GUIClasses.h"
#include "../lib/CConfigHandler.h"
#include "gui/SDL_Extensions.h"
#include "battle/CBattleInterface.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../lib/CGameState.h"
#include "../lib/BattleState.h"
#include "../lib/GameConstants.h"
#include "gui/CGuiHandler.h"

//macros to avoid code duplication - calls given method with given arguments if interface for specific player is present
//awaiting variadic templates...
#define CALL_IN_PRIVILAGED_INTS(function, ...)										\
	do																				\
	{																				\
		BOOST_FOREACH(IGameEventsReceiver *ger, cl->privilagedGameEventReceivers)	\
			ger->function(__VA_ARGS__);												\
	} while(0)

#define CALL_ONLY_THAT_INTERFACE(player, function, ...)		\
		do													\
		{													\
		if(vstd::contains(cl->playerint,player))			\
			cl->playerint[player]->function(__VA_ARGS__);	\
		}while(0)

#define INTERFACE_CALL_IF_PRESENT(player,function,...) 				\
		do															\
		{															\
			CALL_ONLY_THAT_INTERFACE(player, function, __VA_ARGS__);\
			CALL_IN_PRIVILAGED_INTS(function, __VA_ARGS__);			\
		} while(0)

#define CALL_ONLY_THT_BATTLE_INTERFACE(player,function, ...) 	\
	do															\
	{															\
		if(vstd::contains(cl->battleints,player))				\
			cl->battleints[player]->function(__VA_ARGS__);		\
	} while (0);

#define BATTLE_INTERFACE_CALL_RECEIVERS(function,...) 	\
	do															\
	{															\
		BOOST_FOREACH(IBattleEventsReceiver *ber, cl->privilagedBattleEventReceivers)\
			ber->function(__VA_ARGS__);							\
	} while(0)

#define BATTLE_INTERFACE_CALL_IF_PRESENT(player,function,...) 	\
	do															\
	{															\
		CALL_ONLY_THAT_INTERFACE(player, function, __VA_ARGS__);\
		BATTLE_INTERFACE_CALL_RECEIVERS(function, __VA_ARGS__);	\
	} while(0)

//calls all normal interfaces and privilaged ones, playerints may be updated when iterating over it, so we need a copy
#define CALL_IN_ALL_INTERFACES(function, ...)							\
	do																	\
	{																	\
		std::map<PlayerColor, CGameInterface*> ints = cl->playerint;			\
		for(auto i = ints.begin(); i != ints.end(); i++)\
			CALL_ONLY_THAT_INTERFACE(i->first, function, __VA_ARGS__);	\
	} while(0)


#define BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(function,...) 				\
	CALL_ONLY_THT_BATTLE_INTERFACE(GS(cl)->curB->sides[0], function, __VA_ARGS__)	\
	CALL_ONLY_THT_BATTLE_INTERFACE(GS(cl)->curB->sides[1], function, __VA_ARGS__)	\
	BATTLE_INTERFACE_CALL_RECEIVERS(function, __VA_ARGS__)
/*
 * NetPacksClient.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void SetResources::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,receivedResource,-1,-1);
}

void SetResource::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,receivedResource,resid,val);
}

void SetPrimSkill::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
	{
        logNetwork->errorStream() << "Cannot find hero with ID " << id.getNum();
		return;
	}
	INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroPrimarySkillChanged,h,which,val);
}

void SetSecSkill::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
	{
        logNetwork->errorStream() << "Cannot find hero with ID " << id;
		return;
	}
	INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroSecondarySkillChanged,h,which,val);
}

void HeroVisitCastle::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(hid);

	if(start())
	{
		INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroVisitsTown, h, GS(cl)->getTown(tid));
	}
}

void ChangeSpells::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void SetMana::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(hid);
	INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroManaPointsChanged, h);
}

void SetMovePoints::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(hid);
	cl->invalidatePaths(h);
	INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroMovePointsChanged, h);
}

void FoWChange::applyCl( CClient *cl )
{

	BOOST_FOREACH(auto &i, cl->playerint)
		if(cl->getPlayerRelations(i.first, player) != PlayerRelations::ENEMIES)
		{
			if(mode)
				i.second->tileRevealed(tiles);
			else
				i.second->tileHidden(tiles);
		}

	cl->invalidatePaths();
}

void SetAvailableHeroes::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void ChangeStackCount::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner, stackChagedCount, sl, count, absoluteValue);
}

void SetStackType::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner, stackChangedType, sl, *type);
}

void EraseStack::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner, stacksErased, sl);
}

void SwapStacks::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl1.army->tempOwner, stacksSwapped, sl1, sl2);
	if(sl1.army->tempOwner != sl2.army->tempOwner)
		INTERFACE_CALL_IF_PRESENT(sl2.army->tempOwner, stacksSwapped, sl1, sl2);
}

void InsertNewStack::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner,newStackInserted,sl, *sl.getStack());
}

void RebalanceStacks::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(src.army->tempOwner, stacksRebalanced, src, dst, count);
	if(src.army->tempOwner != dst.army->tempOwner)
		INTERFACE_CALL_IF_PRESENT(dst.army->tempOwner,stacksRebalanced, src, dst, count);
}

void PutArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(al.owningPlayer(), artifactPut, al);
}

void EraseArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(al.owningPlayer(), artifactRemoved, al);
}

void MoveArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(src.owningPlayer(), artifactMoved, src, dst);
	if(src.owningPlayer() != dst.owningPlayer())
		INTERFACE_CALL_IF_PRESENT(dst.owningPlayer(), artifactMoved, src, dst);
}

void AssembledArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(al.owningPlayer(), artifactAssembled, al);
}

void DisassembledArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(al.owningPlayer(), artifactDisassembled, al);
}

void HeroVisit::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(hero->tempOwner, heroVisit, hero, obj, starting);
}

void NewTurn::applyCl( CClient *cl )
{
	cl->invalidatePaths();
}


void GiveBonus::applyCl( CClient *cl )
{
	cl->invalidatePaths();
	switch(who)
	{
	case HERO:
		{
			const CGHeroInstance *h = GS(cl)->getHero(ObjectInstanceID(id));
			INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroBonusChanged, h, *h->getBonusList().back(),true);
		}
		break;
	case PLAYER:
		{
			const PlayerState *p = GS(cl)->getPlayer(PlayerColor(id));
			INTERFACE_CALL_IF_PRESENT(PlayerColor(id), playerBonusChanged, *p->getBonusList().back(), true);
		}
		break;
	}
}

void ChangeObjPos::applyFirstCl( CClient *cl )
{
	CGObjectInstance *obj = GS(cl)->getObjInstance(objid);
	if(flags & 1)
		CGI->mh->hideObject(obj);
}
void ChangeObjPos::applyCl( CClient *cl )
{
	CGObjectInstance *obj = GS(cl)->getObjInstance(objid);
	if(flags & 1)
		CGI->mh->printObject(obj);

	cl->invalidatePaths();
}

void PlayerEndsGame::applyCl( CClient *cl )
{
	CALL_IN_ALL_INTERFACES(gameOver, player, victory);
}

void RemoveBonus::applyCl( CClient *cl )
{
	cl->invalidatePaths();
	switch(who)
	{
	case HERO:
		{
			const CGHeroInstance *h = GS(cl)->getHero(ObjectInstanceID(id));
			INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroBonusChanged, h, bonus,false);
		}
		break;
	case PLAYER:
		{
			//const PlayerState *p = GS(cl)->getPlayer(id);
			INTERFACE_CALL_IF_PRESENT(PlayerColor(id), playerBonusChanged, bonus, false);
		}
		break;
	}
}

void UpdateCampaignState::applyCl( CClient *cl )
{
	cl->stopConnection();
	cl->campaignMapFinished(camp);
}

void PrepareForAdvancingCampaign::applyCl(CClient *cl)
{
	cl->serv->prepareForSendingHeroes();
}

void RemoveObject::applyFirstCl( CClient *cl )
{
	const CGObjectInstance *o = cl->getObj(id);

	CGI->mh->hideObject(o);

	int3 pos = o->visitablePos();
	//notify interfaces about removal
	for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
	{
		if(GS(cl)->isVisible(o, i->first))
			i->second->objectRemoved(o);
	}
}

void RemoveObject::applyCl( CClient *cl )
{
	cl->invalidatePaths();
}

void TryMoveHero::applyFirstCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(id);

	//check if playerint will have the knowledge about movement - if not, directly update maphandler
	for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
	{
		if(i->first >= PlayerColor::PLAYER_LIMIT)
			continue;
		TeamState *t = GS(cl)->getPlayerTeam(i->first);
		if((t->fogOfWarMap[start.x-1][start.y][start.z] || t->fogOfWarMap[end.x-1][end.y][end.z])
				&& GS(cl)->getPlayer(i->first)->human)
			humanKnows = true;
	}

	if(result == TELEPORTATION  ||  result == EMBARK  ||  result == DISEMBARK  ||  !humanKnows)
		CGI->mh->removeObject(h);


	if(result == DISEMBARK)
		CGI->mh->printObject(h->boat);
}

void TryMoveHero::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(id);
	cl->invalidatePaths();

	if(result == TELEPORTATION  ||  result == EMBARK  ||  result == DISEMBARK)
	{
		CGI->mh->printObject(h);
	}

	if(result == EMBARK)
		CGI->mh->hideObject(h->boat);

	PlayerColor player = h->tempOwner;

	BOOST_FOREACH(auto &i, cl->playerint)
		if(cl->getPlayerRelations(i.first, player) != PlayerRelations::ENEMIES)
			i.second->tileRevealed(fowRevealed);

	//notify interfaces about move
	for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
	{
		if(i->first >= PlayerColor::PLAYER_LIMIT) continue;
		TeamState *t = GS(cl)->getPlayerTeam(i->first);
		if(t->fogOfWarMap[start.x-1][start.y][start.z] || t->fogOfWarMap[end.x-1][end.y][end.z])
		{
			i->second->heroMoved(*this);
		}
	}

	if(!humanKnows) //maphandler didn't get update from playerint, do it now
	{				//TODO: restructure nicely
		CGI->mh->printObject(h);
	}
}

void NewStructures::applyCl( CClient *cl )
{
	CGTownInstance *town = GS(cl)->getTown(tid);
	BOOST_FOREACH(const auto & id, bid)
	{
		if(id == BuildingID::CAPITOL) //fort or capitol
		{
			town->defInfo = const_cast<CGDefInfo*>(CGI->dobjinfo->capitols[town->subID].get());
		}
		if(id == BuildingID::FORT)
		{
			town->defInfo = const_cast<CGDefInfo*>(CGI->dobjinfo->gobjs[Obj::TOWN][town->subID].get());
		}
		if(vstd::contains(cl->playerint,town->tempOwner))
			cl->playerint[town->tempOwner]->buildChanged(town,id,1);
	}
}
void RazeStructures::applyCl (CClient *cl)
{
	CGTownInstance *town = GS(cl)->getTown(tid);
	BOOST_FOREACH(const auto & id, bid)
	{
		if (id == BuildingID::CAPITOL) //fort or capitol
		{
			town->defInfo = const_cast<CGDefInfo*>(CGI->dobjinfo->gobjs[Obj::TOWN][town->subID].get());
		}
		if(vstd::contains (cl->playerint,town->tempOwner))
			cl->playerint[town->tempOwner]->buildChanged (town,id,2);
	}
}

void SetAvailableCreatures::applyCl( CClient *cl )
{
	const CGDwelling *dw = static_cast<const CGDwelling*>(cl->getObj(tid));

	//inform order about the change

	PlayerColor p;
	if(dw->ID == Obj::WAR_MACHINE_FACTORY) //War Machines Factory is not flaggable, it's "owned" by visitor
		p = cl->getTile(dw->visitablePos())->visitableObjects.back()->tempOwner;
	else
		p = dw->tempOwner;

	INTERFACE_CALL_IF_PRESENT(p, availableCreaturesChanged, dw);
}

void SetHeroesInTown::applyCl( CClient *cl )
{
	CGTownInstance *t = GS(cl)->getTown(tid);
	CGHeroInstance *hGarr  = GS(cl)->getHero(this->garrison);
	CGHeroInstance *hVisit = GS(cl)->getHero(this->visiting);

	std::set<PlayerColor> playersToNotify;

	if(vstd::contains(cl->playerint,t->tempOwner)) // our town
		playersToNotify.insert(t->tempOwner);

	if (hGarr && vstd::contains(cl->playerint,  hGarr->tempOwner))
		playersToNotify.insert(hGarr->tempOwner);

	if (hVisit && vstd::contains(cl->playerint, hVisit->tempOwner))
		playersToNotify.insert(hVisit->tempOwner);

	BOOST_FOREACH(auto playerID, playersToNotify)
		cl->playerint[playerID]->heroInGarrisonChange(t);
}

// void SetHeroArtifacts::applyCl( CClient *cl )
// {
// // 	CGHeroInstance *h = GS(cl)->getHero(hid);
// // 	CGameInterface *player = (vstd::contains(cl->playerint,h->tempOwner) ? cl->playerint[h->tempOwner] : NULL);
// // 	if(!player)
// // 		return;
//
// 	//h->recreateArtBonuses();
// 	//player->heroArtifactSetChanged(h);
//
// // 	BOOST_FOREACH(Bonus bonus, gained)
// // 	{
// // 		player->heroBonusChanged(h,bonus,true);
// // 	}
// // 	BOOST_FOREACH(Bonus bonus, lost)
// // 	{
// // 		player->heroBonusChanged(h,bonus,false);
// // 	}
// }

void HeroRecruited::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->map->heroes.back();
	if(h->subID != hid)
	{
        logNetwork->errorStream() << "Something wrong with hero recruited!";
	}

	CGI->mh->initHeroDef(h);
	CGI->mh->printObject(h);

	if(vstd::contains(cl->playerint,h->tempOwner))
	{
		cl->playerint[h->tempOwner]->heroCreated(h);
		if(const CGTownInstance *t = GS(cl)->getTown(tid))
			cl->playerint[h->tempOwner]->heroInGarrisonChange(t);
	}
}

void GiveHero::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(id);
	CGI->mh->initHeroDef(h);
	CGI->mh->printObject(h);
	cl->playerint[h->tempOwner]->heroCreated(h);
}

void GiveHero::applyFirstCl( CClient *cl )
{
	CGI->mh->hideObject(GS(cl)->getHero(id));
}

void InfoWindow::applyCl( CClient *cl )
{
	std::vector<Component*> comps;
	for(size_t i=0;i<components.size();i++)
	{
		comps.push_back(&components[i]);
	}
	std::string str;
	text.toString(str);

	if(vstd::contains(cl->playerint,player))
		cl->playerint[player]->showInfoDialog(str,comps,(soundBase::soundID)soundID);
	else
        logNetwork->warnStream() << "We received InfoWindow for not our player...";
}

void SetObjectProperty::applyCl( CClient *cl )
{
	//inform all players that see this object
	for(auto it = cl->playerint.cbegin(); it != cl->playerint.cend(); ++it)
	{
		if(GS(cl)->isVisible(GS(cl)->getObjInstance(id), it->first))
			INTERFACE_CALL_IF_PRESENT(it->first, objectPropertyChanged, this);
	}
}

void HeroLevelUp::applyCl( CClient *cl )
{
	//INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroGotLevel, h, primskill, skills, id);
	if(vstd::contains(cl->playerint,hero->tempOwner))
	{
		cl->playerint[hero->tempOwner]->heroGotLevel(hero, primskill, skills, queryID);
	}
	//else
	//	cb->selectionMade(0, queryID);
}

void CommanderLevelUp::applyCl( CClient *cl )
{
	const CCommanderInstance * commander = hero->commander;
	assert (commander);
	PlayerColor player = hero->tempOwner;
	if (commander->armyObj && vstd::contains(cl->playerint, player)) //is it possible for Commander to exist beyond armed instance?
	{
		cl->playerint[player]->commanderGotLevel(commander, skills, queryID);
	}
}

void BlockingDialog::applyCl( CClient *cl )
{
	std::string str;
	text.toString(str);

	if(vstd::contains(cl->playerint,player))
		cl->playerint[player]->showBlockingDialog(str,components,queryID,(soundBase::soundID)soundID,selection(),cancel());
	else
        logNetwork->warnStream() << "We received YesNoDialog for not our player...";
}

void GarrisonDialog::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(hid);
	const CArmedInstance *obj = static_cast<const CArmedInstance*>(cl->getObj(objid));

	if(!vstd::contains(cl->playerint,h->getOwner()))
		return;

	cl->playerint[h->getOwner()]->showGarrisonDialog(obj,h,removableUnits,queryID);
}

void BattleStart::applyCl( CClient *cl )
{
	cl->battleStarted(info);
}

void BattleNextRound::applyFirstCl(CClient *cl)
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleNewRoundFirst,round);
}

void BattleNextRound::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleNewRound,round);
}

void BattleSetActiveStack::applyCl( CClient *cl )
{
	if(!askPlayerInterface)
		return;

	const CStack * activated = GS(cl)->curB->battleGetStackByID(stack);
	PlayerColor playerToCall; //player that will move activated stack
	if( activated->hasBonusOfType(Bonus::HYPNOTIZED) )
	{
		playerToCall = ( GS(cl)->curB->sides[0] == activated->owner ? GS(cl)->curB->sides[1] : GS(cl)->curB->sides[0] );
	}
	else
	{
		playerToCall = activated->owner;
	}
	if( vstd::contains(cl->battleints, playerToCall) )
		boost::thread( boost::bind(&CClient::waitForMoveAndSend, cl, playerToCall) );
}

void BattleTriggerEffect::applyCl(CClient * cl)
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleTriggerEffect, *this);
}

void BattleObstaclePlaced::applyCl(CClient * cl)
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleObstaclePlaced, *obstacle);
}

void BattleResult::applyFirstCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleEnd,this);
	cl->battleFinished();
}

void BattleStackMoved::applyFirstCl( CClient *cl )
{
	const CStack * movedStack = GS(cl)->curB->battleGetStackByID(stack);
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStackMoved,movedStack,tilesToMove,distance);
}

//void BattleStackAttacked::( CClient *cl )
void BattleStackAttacked::applyFirstCl( CClient *cl )
{
	std::vector<BattleStackAttacked> bsa;
	bsa.push_back(*this);

	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksAttacked,bsa);
}

void BattleAttack::applyFirstCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleAttack,this);
	for (int g=0; g<bsa.size(); ++g)
	{
		for (int z=0; z<bsa[g].healedStacks.size(); ++z)
		{
			bsa[g].healedStacks[z].applyCl(cl);
		}
	}
}

void BattleAttack::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksAttacked,bsa);
}

void StartAction::applyFirstCl( CClient *cl )
{
	cl->curbaction = new BattleAction(ba);
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(actionStarted, &ba);
}

void BattleSpellCast::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleSpellCast,this);
}

void SetStackEffect::applyCl( CClient *cl )
{
	//informing about effects
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksEffectsSet,*this);
}

void StacksInjured::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksAttacked,stacks);
}

void BattleResultsApplied::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player1, battleResultsApplied);
	INTERFACE_CALL_IF_PRESENT(player2, battleResultsApplied);
	INTERFACE_CALL_IF_PRESENT(PlayerColor::UNFLAGGABLE, battleResultsApplied);
	if(GS(cl)->initialOpts->mode == StartInfo::DUEL)
	{
		cl->terminate = true;
		CloseServer cs;
		*cl->serv << &cs;
	}
}

void StacksHealedOrResurrected::applyCl( CClient *cl )
{
	std::vector<std::pair<ui32, ui32> > shiftedHealed;
	for(int v=0; v<healedStacks.size(); ++v)
	{
		shiftedHealed.push_back(std::make_pair(healedStacks[v].stackID, healedStacks[v].healedHP));
	}
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksHealedRes, shiftedHealed, lifeDrain, tentHealing, drainedFrom);
}

void ObstaclesRemoved::applyCl( CClient *cl )
{
	//inform interfaces about removed obstacles
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleObstaclesRemoved, obstacles);
}

void CatapultAttack::applyCl( CClient *cl )
{
	//inform interfaces about catapult attack
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleCatapultAttacked, *this);
}

void BattleStacksRemoved::applyCl( CClient *cl )
{
	//inform interfaces about removed stacks
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksRemoved, *this);
}

void BattleStackAdded::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleNewStackAppeared, GS(cl)->curB->stacks.back());
}

CGameState* CPackForClient::GS( CClient *cl )
{
	return cl->gs;
}

void EndAction::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(actionFinished, cl->curbaction);

	delete cl->curbaction;
	cl->curbaction = NULL;
}

void PackageApplied::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player, requestRealized, this);
	if(!cl->waitingRequest.tryRemovingElement(requestID))
        logNetwork->warnStream() << "Surprising server message!";
}

void SystemMessage::applyCl( CClient *cl )
{
	std::ostringstream str;
	str << "System message: " << text;

    logNetwork->errorStream() << str.str(); // usually used to receive error messages from server
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void PlayerBlocked::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,playerBlocked,reason);
}

void YourTurn::applyCl( CClient *cl )
{
	CALL_IN_ALL_INTERFACES(playerStartsTurn, player);
	CALL_ONLY_THAT_INTERFACE(player,yourTurn);
}

void SaveGame::applyCl(CClient *cl)
{
	CFileInfo info(fname);
	CResourceHandler::get()->createResource(info.getStem() + ".vcgm1");

	try
	{
		CSaveFile save(CResourceHandler::get()->getResourceName(ResourceID(info.getStem(), EResType::CLIENT_SAVEGAME)));
		cl->saveCommonState(save);
		save << *cl;
	}
	catch(std::exception &e)
	{
        logNetwork->errorStream() << "Failed to save game:" << e.what();
	}

// 	try
// 	{
// 		auto clientPart = CResourceHandler::get()->getResourceName(ResourceID(info.getStem(), EResType::CLIENT_SAVEGAME));
// 		auto libPart = CResourceHandler::get()->getResourceName(ResourceID(info.getStem(), EResType::LIB_SAVEGAME));
// 		CLoadIntegrityValidator checker(clientPart, libPart);
// 		
// 		CMapHeader mh;
// 		StartInfo *si;
// 		LibClasses *lib;
// 		CGameState *game;
// 
// 		checker.checkMagicBytes(SAVEGAME_MAGIC);
// 		checker >> mh >> si >> lib >> game;
// 	}
// 	catch(...)
// 	{
// 	}
}

void PlayerMessage::applyCl(CClient *cl)
{
	std::ostringstream str;
	str << "Player "<< player <<" sends a message: " << text;

    logNetwork->debugStream() << str.str();
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void SetSelection::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
		return;

	//CPackForClient::GS(cl)->calculatePaths(h, *cl->pathInfo);
}

void ShowInInfobox::applyCl(CClient *cl)
{
	INTERFACE_CALL_IF_PRESENT(player,showComp, c, text.toString());
}

void AdvmapSpellCast::applyCl(CClient *cl)
{
	cl->invalidatePaths();
	//consider notifying other interfaces that see hero?
	INTERFACE_CALL_IF_PRESENT(caster->getOwner(),advmapSpellCast, caster, spellID);
}

void OpenWindow::applyCl(CClient *cl)
{
	switch(window)
	{
	case EXCHANGE_WINDOW:
		{
			const CGHeroInstance *h = cl->getHero(ObjectInstanceID(id1));
			const CGObjectInstance *h2 = cl->getHero(ObjectInstanceID(id2));
			assert(h && h2);
			INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroExchangeStarted, ObjectInstanceID(id1), ObjectInstanceID(id2));
		}
		break;
	case RECRUITMENT_FIRST:
	case RECRUITMENT_ALL:
		{
			const CGDwelling *dw = dynamic_cast<const CGDwelling*>(cl->getObj(ObjectInstanceID(id1)));
			const CArmedInstance *dst = dynamic_cast<const CArmedInstance*>(cl->getObj(ObjectInstanceID(id2)));
			INTERFACE_CALL_IF_PRESENT(dst->tempOwner,showRecruitmentDialog, dw, dst, window == RECRUITMENT_FIRST ? 0 : -1);
		}
		break;
	case SHIPYARD_WINDOW:
		{
			const IShipyard *sy = IShipyard::castFrom(cl->getObj(ObjectInstanceID(id1)));
			INTERFACE_CALL_IF_PRESENT(sy->o->tempOwner, showShipyardDialog, sy);
		}
		break;
	case THIEVES_GUILD:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl->getObj(ObjectInstanceID(id2));
			INTERFACE_CALL_IF_PRESENT(PlayerColor(id1), showThievesGuildWindow, obj);
		}
		break;
	case UNIVERSITY_WINDOW:
		{
			//displays University window (when hero enters University on adventure map)
			const IMarket *market = IMarket::castFrom(cl->getObj(ObjectInstanceID(id1)));
			const CGHeroInstance *hero = cl->getHero(ObjectInstanceID(id2));
			INTERFACE_CALL_IF_PRESENT(hero->tempOwner,showUniversityWindow, market, hero);
		}
		break;
	case MARKET_WINDOW:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl->getObj(ObjectInstanceID(id1));
			const CGHeroInstance *hero = cl->getHero(ObjectInstanceID(id2));
			const IMarket *market = IMarket::castFrom(obj);
			INTERFACE_CALL_IF_PRESENT(cl->getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, showMarketWindow, market, hero);
		}
		break;
	case HILL_FORT_WINDOW:
		{
			//displays Hill fort window
			const CGObjectInstance *obj = cl->getObj(ObjectInstanceID(id1));
			const CGHeroInstance *hero = cl->getHero(ObjectInstanceID(id2));
			INTERFACE_CALL_IF_PRESENT(cl->getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, showHillFortWindow, obj, hero);
		}
		break;
	case PUZZLE_MAP:
		{
			INTERFACE_CALL_IF_PRESENT(PlayerColor(id1), showPuzzleMap);
		}
		break;
	case TAVERN_WINDOW:
		const CGObjectInstance *obj1 = cl->getObj(ObjectInstanceID(id1)),
								*obj2 = cl->getObj(ObjectInstanceID(id2));
		INTERFACE_CALL_IF_PRESENT(obj1->tempOwner, showTavernWindow, obj2);
		break;
	}

}

void CenterView::applyCl(CClient *cl)
{
	INTERFACE_CALL_IF_PRESENT (player, centerView, pos, focusTime);
}

void NewObject::applyCl(CClient *cl)
{
	cl->updatePaths();

	const CGObjectInstance *obj = cl->getObj(id);
	CGI->mh->printObject(obj);

	for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
	{
		if(GS(cl)->isVisible(obj, i->first))
			i->second->newObject(obj);
	}
}

void SetAvailableArtifacts::applyCl(CClient *cl)
{
	if(id < 0) //artifact merchants globally
	{
		for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
			i->second->availableArtifactsChanged(NULL);
	}
	else
	{
		const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(cl->getObj(ObjectInstanceID(id)));
		assert(bm);
		INTERFACE_CALL_IF_PRESENT(cl->getTile(bm->visitablePos())->visitableObjects.back()->tempOwner, availableArtifactsChanged, bm);
	}
}

void TradeComponents::applyCl(CClient *cl)
{///Shop handler
	switch (CGI->mh->map->objects[objectid]->ID)
	{
	case Obj::BLACK_MARKET:
		break;
	case Obj::TAVERN:
		break;
	case Obj::DEN_OF_THIEVES:
		break;
	case Obj::TRADING_POST_SNOW:
		break;
	default:
        logNetwork->warnStream() << "Shop type not supported!";
	}
}
