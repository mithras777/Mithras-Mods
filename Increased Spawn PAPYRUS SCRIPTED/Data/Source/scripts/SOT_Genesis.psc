ScriptName SOT_Genesis Extends ObjectReference  



Float StaminaResult
Float StaminaResultHundred
Float HealthResult
Float HealthResultHundred
Float MagickaResult
Float MagickaResultHundred
Int Property RanCounter Auto
float Property GameDayRan Auto
FormList DudeList
FormList ObjectsList
FormList workinglist
ActorBase HostileNPC
Actor dude 
Int GenDist
Int iindex3 
; FTravelAmbush Property Checker  Auto  
GlobalVariable Property GiveMePotion  Auto  

FormList Property PotionsWell  Auto 
GlobalVariable Property SOT_Stamina  Auto  

GlobalVariable Property SOT_Health  Auto  

GlobalVariable Property SOT_Magicka  Auto  

GlobalVariable Property SOT_StaminaHigh  Auto  

GlobalVariable Property SOT_HealthHigh  Auto  

GlobalVariable Property SOT_MagickaHigh  Auto 
GlobalVariable Property SOT_AllowUnlevelledNPC  Auto  
potion elixir
Actor dudeactor


Event OnLoad()
	If testing.GetValueInt() == 5
		Debug.Notification("Fired On Load!")
	EndIf
EndEvent


Event OnCellAttach()

Actor TempActor = None 
	If testing.GetValueInt() == 5
				Debug.Notification("On Cell Attached Fired!")
			EndIf
utility.wait(1.0)			
Actor PlayerRef = Game.GetPlayer()
positionholder.MoveTo(Game.GetPlayer())	
utility.wait(1.0)		
if Game.GetPlayer().GetDistance(positionholder) < 200000 	
utility.wait(5.0)
If testing.GetValueInt() == 5
				Debug.Notification("Wait Over!")
			EndIf
	If kill35.GetValueInt() == 1
	
if testing.getvalueint() == 5

debug.notification("gamedaysran: " + GameDayRan)
debug.notification("game days passed: " + GameDaysPassed.GetValue() )
debug.notification("genesis days to reset: " + GenesisDaysToReset.GetValue() )

endif	
	
		If GameDayRan == 0 || (GameDaysPassed.GetValue() - GamedayRan) > GenesisDaysToReset.GetValue() ;  || testing.GetValueInt() == 5
		; if toggled check for present hostiles
		TempActor = None
		int checkhostiles = GenesisClear.getvalueint()
	if checkhostiles == 0	
		int count = 0
		actor hero = game.GetPlayer()
								While TempActor == None && (Count < 10) 
											TempActor = Game.FindRandomActorFromRef(hero, 8000.0) 
												If Tempactor != None
												If (TempActor == hero)  || (TempActor.IsDead()) || !(TempActor.IsHostileToActor(Game.GetPlayer())) 
													TempActor = None 
													Else
													If testing.GetValueInt() == 5
		Debug.Notification("Hostile Found!")
		checkhostiles = 100
			EndIf
												EndIf 
												EndIf
											
											Count += 1 
										EndWhile 
		EndIf
		if Tempactor == None && checkhostiles != 100

If testing.GetValueInt() == 5
		
		Debug.Notification("No Hostiles Found!")
		
endif

		GameDayRan = GameDaysPassed.GetValue()
			;;;;loot loot loot
			GenesisLoot.stop()
			GENESISloot.Reset()
			utility.wait(1.0)
			GenesisLoot.Start()
			utility.wait(2.0)
			; type of Place
			If Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeDraugrCrypt)
				Dudelist = GenesisDraugr
				ObjectsList = GenesisDraugrObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Draugr Keyword!")
				EndIf
			ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeBanditCamp) || Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeMine)
				Dudelist = GenesisBandits
				ObjectsList = GenesisBanditObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Bandit Keyword!")
				EndIf
			ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeFalmerHive) 
				Dudelist = GenesisFalmer
				ObjectsList = GenesisFalmerObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Falmer Keyword!")
				EndIf
			ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocSetDwarvenRuin) || Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeDwarvenAutomatons)
				Dudelist = GenesisDwarven
				ObjectsList = GenesisDwarvenObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Dwarven Keyword!")
				EndIf
						ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeForswornCamp)
				Dudelist = GenesisForsworn
				ObjectsList = GenesisForswornObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Forsworn Keyword!")
				EndIf
			ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeVampireLair)
				Dudelist = GenesisVampire
				ObjectsList = GenesisVampireObjects2
				If testing.GetValueInt() == 5
					Debug.Notification("Vampire Keyword!")
				EndIf
			ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeWarlockLair)
				Dudelist = GenesisWarlock
				ObjectsList = GenesisWarlockObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Warlock Keyword!")
				EndIf
			ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocSetNordicRuin)
				Dudelist = GenesisBandits
				ObjectsList = GenesisBanditObjects
				If testing.GetValueInt() == 5
					Debug.Notification("NordicRuin Keyword!")
				EndIf
			ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeDungeon)
				Dudelist = GenesisSkeletons
				ObjectsList = GenesisSkeletonsObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Skeleton/Dungeon Keyword!")
				EndIf
				ElseIf Game.GetPlayer().GetCurrentLocation().HasKeyword(LocTypeDragonPriestLair)
				Dudelist = GenesisDragonPriest
				ObjectsList = GenesisDragonPriestObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Dragon Priest Keyword!")
				EndIf
			Else
			
			;indoor or outdoor
			if  Game.GetPlayer().GetDistance(skulltomeasure) < 900000 && !((game.GetPlayer().GetParentCell()).IsInterior())
			; outdoors
			Dudelist = GenesisBandits
				ObjectsList = GenesisBanditObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Bandit Keyword! if outdoors")
				EndIf
			
			Else
			;indoors
			Dudelist = GenesisSpiders
				ObjectsList = GenesisSpiderObjects
				If testing.GetValueInt() == 5
					Debug.Notification("Spiders by Default if indoors!")
						EndIf
			
			EndIf
			
			EndIf
			;look for spawn Places
			If testing.GetValueInt() == 5
					Debug.Notification("Looking for Objects!")
				EndIf
			;objectslist.Revert()
			if ObjectsList != None && ObjectsList.GetSize() > 0 
						Int iIndex = ObjectsList.GetSize() ; Indices are offset by 1 relative to size
						utility.Wait(0.50)
						If testing.GetValueInt() == 5
					Debug.Notification("size of objects list: "+iIndex)
				EndIf
	
			;
			If testing.GetValueInt() == 5
					Debug.Notification("Try to Spawn At Found Object Location!")
				EndIf
			;
			; surface or dungeon?
			if  Game.GetPlayer().GetDistance(skulltomeasure) < 900000 && !((game.GetPlayer().GetParentCell()).IsInterior())
			         ; outdoors
			     iindex3 = GenesisOutsideSpawns.GetValueInt()
			Else
			           ; dungeon
				 iindex3 = GenesisPotentialSpawns.GetValueInt()
			EndIf
			
			
			
			
			While iIndex3
				Iindex3 -= 1
				if Game.GetPlayer().GetDistance(positionholder) < 200000 	
				GenDist = GenesisDistance.GetValueInt()
				ObjectReference p = game.getPlayer() as ObjectReference
				ObjectReference SpawnAtObject = Game.FindRandomReferenceOfAnyTypeInListFromRef(objectslist, p, GenDist)
				Utility.Wait(Utility.RandomFloat(1.0,2.50))
				if iIndex > 200
				Utility.Wait(Utility.RandomFloat(1.0,2.50))
				EndIf
				If SpawnAtObject != None		
					;objectslist.RemoveAddedForm(SpawnAtObject)
					;spawn npc
					Int iIndex2 = DudeList.GetSize() ; Indices are offset by 1 relative to size
					HostileNPC = DudeList.GetAt(Utility.RandomInt(0, (iIndex2 - 1))) As ActorBase 
;check if close to player if GenesisAtPlayerAllow is = to 5 - if zero default then it ok					
;
                    if GenesisAtPlayerAllow.getvalueint() == 0 ||  Game.GetPlayer().GetDistance(SpawnAtObject) > 800
					dude = SpawnAtObject.PlaceActorAtMe (HostileNPC, 2)
;				
If testing.GetValueInt() == 10			
				Debug.Notification("Health before: "+ dude.GetActorValue("Health"))	
					Debug.Notification("Magicka before: "+ dude.GetActorValue("Magicka"))	
					Debug.Notification("Stamina beforfe: "+ dude.GetActorValue("Stamina"))	
EndIf
					UnlevelNPC(dude)
					
					Utility.wait(0.5)
					
					GivePotions(dude)
If testing.GetValueInt() == 10
								
					Debug.Notification("Health after: "+ dude.GetActorValue("Health"))	
					Debug.Notification("Magicka after: "+ dude.GetActorValue("Magicka"))	
					Debug.Notification("Stamina after: "+ dude.GetActorValue("Stamina"))	
										
					EndIf					
					
					dude.StartCombat(Game.GetPlayer())
					
					
					dude.SetRelationshipRank(Game.GetPlayer(), -4)
					Utility.Wait(Utility.RandomFloat(0.5,1.50))
					If testing.GetValueInt() == 5
						Debug.Notification("Spawned! on index #: "+ Iindex3)
					EndIf
					EndIf
				Else
					If testing.GetValueInt() == 5
						Debug.Notification("Nothing Found!")
					EndIf
				EndIf
				Else
				If testing.GetValueInt() == 5
						Debug.Notification("Left Original Skull Area, Cannot Spawn!")
					EndIf
				EndIf
			EndWhile
			;;;
		EndIf
		objectslist.Revert()
		EndIf
		
		
	EndIf
	EndIf
EndIf
EndEvent


	
		
Function UnLevelNPC (Actor Dude2)

	If SOT_AllowUnlevelledNPC.GetValueInt() == 1
		StaminaResult = Utility.RandomFloat(SOT_Stamina.GetValueInt(), SOT_StaminaHigh.GetValueInt())
		StaminaResultHundred = StaminaResult / 100
		StaminaResultHundred = Game.GetPlayer().GetBaseActorValue("Stamina") * StaminaResultHundred
		HealthResult = Utility.RandomFloat(SOT_Health.GetValue(), SOT_HealthHigh.GetValue())
		HealthResultHundred = HealthResult / 100
		HealthResultHundred = Game.GetPlayer().GetBaseActorValue("health") * HealthResultHundred
		MagickaResult = Utility.RandomFloat(SOT_Magicka.GetValue(), SOT_MagickaHigh.GetValue())
		MagickaResultHundred = MagickaResult / 100
		MagickaResultHundred = Game.GetPlayer().GetBaseActorValue("magicka") * MagickaResultHundred
		dude2.SetActorValue("Health", HealthResultHundred)
		dude2.SetActorValue("Magicka", MagickaResultHundred)
		dude2.SetActorValue("Stamina", StaminaResultHundred)
		;;;;;;;;; add potion if not toggled off
		
		;;;;;;;;;
	EndIf
	;EndIf
	
EndFunction


Function GivePotions(Actor aDrinker)
	;if(givepotion!=None && potionchance!=None && PotionsWell!=None)
		if givemepotion.getvalueint() == 0
			if utility.RandomInt() < oddsofpotion.getvalueint()
			int	iIndex = PotionsWell.getsize()
				int myloop = Utility.randomint(1, 4) 
				While myloop > 0
					myloop = myloop - 1						
				int	irandom = utility.randomint(0, iindex - 1)
				potion	bottle = PotionsWell.GetAt(irandom) as Potion
					aDrinker.AddItem(bottle, 1)
					;Debug.Notification("potion added: "+bottle)
				EndWhile
			EndIf
		EndIf
	;EndIf
	return
EndFunction

GlobalVariable Property GenesisAllow  Auto  

GlobalVariable Property GenesisDaysToReset  Auto  

GlobalVariable Property GameDaysPassed  Auto  

GlobalVariable Property GenesisOddsToSpawnDraugr  Auto  

GlobalVariable Property GenesisOddsToSpawn  Auto  

GlobalVariable Property GenesisOddsToSpawnBandits  Auto  

GlobalVariable Property GenesisOddsToSpawnSpiders  Auto  

GlobalVariable Property GenesisOddsToSpawnSkels  Auto  

GlobalVariable Property GenesisOddsToSpawnBoss  Auto  

FormList Property GenesisDraugr  Auto  

FormList Property GenesisDraugrObjects  Auto  

GlobalVariable Property GenesisOddsToSpawnFalmer  Auto  

FormList Property GenesisBandits  Auto  

FormList Property GenesisBanditsObjects  Auto  

FormList Property GenesisSpiders  Auto  

FormList Property GenesisSpidersObjects  Auto  


FormList Property GenesisSkeletons  Auto  

FormList Property GenesisSkeletonsObjects  Auto  

FormList Property GenesisFalmer  Auto  

FormList Property GenesisFalmerObjects  Auto  
GlobalVariable Property OddsOfPotion  Auto 
Keyword Property LocSetNordicRuin  Auto  
Keyword Property LocTypeDraugrCrypt Auto
Keyword Property LocTypeDungeon Auto
Keyword Property LocSetCave Auto
Keyword Property LocSetCaveIce Auto
Keyword Property LocSetDwarvenRuin Auto
Keyword Property LocSetMilitaryCamp Auto
Keyword Property LocSetMilitaryFort Auto
Keyword Property LocTypeAnimalDen Auto
Keyword Property LocTypeBanditCamp Auto
Keyword Property LocTypeDragonLair Auto
Keyword Property LocTypeDragonPriestLair Auto
Keyword Property LocTypeDwarvenAutomatons Auto
Keyword Property LocTypeFalmerHive Auto
Keyword Property LocTypeForswornCamp Auto
Keyword Property LocTypeGiantCamp Auto
Keyword Property LocTypeHagravenNest Auto
Keyword Property LocTypeHold Auto
Keyword Property LocTypeHoldMajor Auto
Keyword Property LocTypeHoldMinor Auto
Keyword Property LocTypeMilitaryCamp Auto
Keyword Property LocTypeMilitaryFort Auto
Keyword Property LocTypeMine Auto
Keyword Property LocTypeOrcStronghold Auto
Keyword Property LocTypeVampireLair Auto
Keyword Property LocTypeWarlockLair Auto
;Keyword Property LocTypeWerebearLair Auto
Keyword Property LocTypeWerewolfLair Auto

GlobalVariable Property testing  Auto  
GlobalVariable Property kill35  Auto 
GlobalVariable Property GenesisDistance  Auto  

GlobalVariable Property GenesisPotentialSpawns  Auto  

FormList Property GenesisBlank  Auto  

FormList Property GenesisDragonPriest  Auto  

FormList Property GenesisDragonPriestObjects  Auto  

FormList Property GenesisDwarven  Auto  

FormList Property GenesisDwarvenObjects  Auto  

FormList Property GenesisForsworn  Auto  

FormList Property GenesisForswornObjects  Auto  

FormList Property GenesisVampire  Auto  

FormList Property GenesisVampireObjects2  Auto  

FormList Property GenesisWarlock  Auto  

FormList Property GenesisWarlockObjects  Auto  

FormList Property GenesisBanditObjects  Auto 

FormList Property GenesisSpiderObjects Auto

Quest Property GenesisLoot  Auto  



GlobalVariable Property SOT_BeefierLoot  Auto  

;Quest Property FastTravelAmbushes2012  Auto  

GlobalVariable Property GenesisClear  Auto  

GlobalVariable Property GenesisAtPlayerAllow  Auto  

ObjectReference Property skulltomeasure  Auto 



ObjectReference Property PositionHolder  Auto  

GlobalVariable Property GenesisOutsideSpawns  Auto  
