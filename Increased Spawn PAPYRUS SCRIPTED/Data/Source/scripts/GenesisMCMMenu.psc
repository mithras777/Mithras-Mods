Scriptname GenesisMCMMenu extends SKI_ConfigBase  

GlobalVariable Property kill35  Auto 
bool ModActivationStatus
int modactivation
int daystoresetslider
int daystoresetsliderA
int potentialspawns
int potentialspawnsA
int SOTStamina
int SOTStaminaA
Int AllowUnlevelledNPC
Bool AllowUnlevelledNPCStatus = False
Int SOTHealth
Int SOTHealthA
Int SOTMagicka
Int SOTMagickaA
Int SOTSTaminaHigh
Int SOTSTaminaHighA
Int SOTHealthHigh
Int SOTHealthHighA
Int SOTMagickaHigh
Int SOTMagickaHighA
Int StaminaResult
Int HealthResult
Int MagickaResult
bool AllowBeefierLootStatus = True
int allowbeefierloot
int allowbeefierlootA
int genesisloot
int genesislootA
int potiongive
bool potiongiveStatus = True
int potionodds
int potionoddsA
int spawnwithothers  ; 0 if no
bool spawnwithothersstatus = False
int outsidespawns
int outsidespawnsA
bool allowtestingstatus = False
int allowtesting
int allowtestingA



GlobalVariable Property SOT_Stamina  Auto 
GlobalVariable Property SOT_StaminaHigh  Auto   
GlobalVariable Property SOT_Health  Auto  
GlobalVariable Property SOT_HealthHigh  Auto 
GlobalVariable Property SOT_Magicka  Auto  
GlobalVariable Property SOT_MagickaHigh  Auto
GlobalVariable Property SOT_AllowUnlevelledNPC  Auto 
GlobalVariable Property SOT_BeefierLoot  Auto 
int GenDistance
int GenDistanceA
int Atplayerallow
bool AtplayerallowStatus = True
int togglesurface
bool togglesurfacestatus = False

Event OnConfigInit()
Pages = New String[1]   ; for one menu page, 2 for 2 pages, etc
Pages[0] = "Genesis Options" ; whatever you want to name it
EndEvent


Event OnPageReset(String a_page)

If (a_page == "")
		LoadCustomContent("skyui/MCM-Genesis.dds", 36, 0)
		Return
	Else
		UnloadCustomContent()
	EndIf

If (a_page == "Genesis Options")
SetCursorFillMode(TOP_TO_BOTTOM)  ; we start menu on top
AddHeaderOption("Activation")
AddEmptyOption()
if kill35.getvalueint() == 1
ModActivationStatus = True 
Else
ModActivationStatus = False
EndIf
ModActivation = AddToggleOption("Mod is Active", ModActivationStatus)
AddEmptyOption()
AddHeaderOption("Creation Frequency")
AddEmptyOption()
daystoresetslider = AddSliderOption("Days Before Genesis Resets ", GenesisDaysToReset.GetValueInt())
potentialspawns = AddSliderOption("Max Number of Dungeon Spawns ", GenesisPotentialSpawns.GetValueInt())
outsidespawns = AddSliderOption("Max Number of Surface Spawns ", GenesisOutsideSpawns.GetValueInt())
GenDistance = AddSliderOption("...Distance To Scan", GenesisDistance.GetValue())
spawnwithothers = AddToggleOption("Spawn If Other Hostiles Near", spawnwithothersStatus)
Atplayerallow = AddToggleOption("Allow Spawning At Player", AtplayerallowStatus)
togglesurface = AddToggleOption("Disable Surface Spawns", togglesurfacestatus)
AddEmptyOption()
AddHeaderOption("UnLevel Spawned NPCs") ; default false
AddEmptyOption()
AllowUnlevelledNPC = AddToggleOption("Allow Unlevelled (Dynamic) NPCs", AllowUnlevelledNPCStatus)
        
		if AllowUnlevelledNPCStatus == True
		SOTHealth = AddSliderOption("Health Lowest Value", SOT_Health.GetValueInt())
		SOTHealthHigh = AddSliderOption("Health Highest Value", SOT_HealthHigh.GetValueInt())
		AddEmptyOption()
		SOTSTamina = AddSliderOption("Stamina Lowest Value", SOT_Stamina.GetValueInt())
		SOTSTaminaHigh = AddSliderOption("Stamina Highest Value", SOT_StaminaHigh.GetValueInt())
		AddEmptyOption()
		SOTMagicka = AddSliderOption("Magicka Lowest Value", SOT_Magicka.GetValueInt())
		SOTMagickaHigh = AddSliderOption("Magicka Highest Value", SOT_MagickaHigh.GetValueInt())
		EndIf
		
		
		AddEmptyOption()
	
SetCursorPosition(1)

AddHeaderOption("Add Loot") ; default yes
AddEmptyOption()
AllowBeefierLoot = AddToggleOption("Enable ''Enhanced Treasure'' Module", AllowBeefierLootStatus)
GenesisLoot = AddSliderOption("...Odds of Added Treasure: ", GenesisLootChance.getvalueint())
	potiongive = AddToggleOption("Enable Chance of Potions", potiongiveStatus)
		potionodds = AddSliderOption("...Odds of Potions Added to NPCs", OddsofPotion.GetValueInt())	
AddEmptyOption()
AllowTesting = Addtoggleoption("Diagnostics: ",AllowTestingStatus)		
		
		
		
EndIf

EndEvent


Event OnOptionDefault(Int a_option)             ; TOGGLE RESET
	
	If (a_option == allowtesting)
		allowtestingStatus = False
		SetToggleOptionValue(a_option, allowtestingStatus)
		testing.setvalueint(0)
		EndIf
		ForcePageReset()
	
	If (a_option == ModActivation)
		ModActivationStatus = False
		SetToggleOptionValue(a_option, ModActivationStatus)
		kill35.setvalueint(0)
		EndIf
		ForcePageReset()
	

	If (a_option == AllowUnlevelledNPC)
		AllowUnlevelledNPCStatus = False
		SetToggleOptionValue(a_option, AllowUnlevelledNPCStatus)
		SOT_AllowUnlevelledNPC.SetValueInt(0)
		ForcePageReset()
	EndIf	

		If (a_option == AllowBeefierLoot)
		AllowBeefierLootstatus = True
		SetToggleOptionValue(a_option,AllowBeefierLoot)
		SOT_BeefierLoot.SetValueInt(0)
	EndIf
	
		If (a_option == potiongive)
		potiongivestatus = True
		SetToggleOptionValue(a_option, potiongiveStatus)
		GiveMePotion.SetValueInt(0)
	EndIf
	
	If (a_option == spawnwithothers)
		spawnwithothersstatus = False
		SetToggleOptionValue(a_option, spawnwithothersStatus)
		GenesisClear.SetValueInt(0)
	EndIf
	
	If (a_option == Atplayerallow)
		Atplayerallowstatus = True
		SetToggleOptionValue(a_option, AtplayerallowStatus)
		GenesisAtPlayerAllow.SetValueInt(0)
	EndIf
	
		If (a_option == togglesurface)
		togglesurfacestatus = True
		SetToggleOptionValue(a_option, togglesurfaceStatus)
		EvilSkull1.enable()
		EvilSkull2.enable()
		EvilSkull3.enable()
		EvilSkull4.enable()
		EvilSkull5.enable()
		EvilSkull6.enable()
		EvilSkull7.enable()
		EvilSkull8.enable()
		EvilSkull9.enable()
		EvilSkull10.enable()
		EvilSkull11.enable()
		EvilSkull12.enable()
		EvilSkull13.enable()
		EvilSkull14.enable()
		EvilSkull15.enable()
		EvilSkull16.enable()
		EvilSkull17.enable()
		EvilSkull18.enable()
		EvilSkull19.enable()
		EvilSkull20.enable()
		EvilSkull21.enable()
	EndIf
	
EndEvent

Event OnOptionSelect(Int a_option)    ; TOGGLE PROCESS SELECTION


If (a_option == allowtesting)
allowtestingStatus = !allowtestingStatus
	SetToggleOptionValue(a_option, allowtestingStatus)
	If allowtestingStatus== True
		testing.SetValueInt(5)
	Else
	     testing.SetValueInt(0)
	EndIf
	EndIf


If (a_option == ModActivation)
ModActivationStatus = !ModActivationStatus
	SetToggleOptionValue(a_option, ModActivationStatus)
	If ModActivationStatus== True
		kill35.SetValueInt(1)
	Else
	     kill35.SetValueInt(0)
	EndIf
	EndIf
	
If (a_option == AllowUnlevelledNPC)
		AllowUnlevelledNPCStatus = !AllowUnlevelledNPCstatus
		SetToggleOptionValue(a_option, AllowUnlevelledNPCStatus)
		If AllowUnlevelledNPCStatus == True 
			SOT_AllowUnlevelledNPC.SetValueInt(1) 
			ForcePageReset()
		Else
			SOT_AllowUnlevelledNPC.SetValueInt(0)
			ForcePageReset()
		EndIf
	EndIf	
	
If (a_option == AllowBeefierLoot)
		AllowBeefierLootStatus = !AllowBeefierLootStatus
		SetToggleOptionValue(a_option, AllowBeefierLootStatus)
		If AllowBeefierLootStatus == True 
			SOT_BeefierLoot.SetValueInt(0)
			Else
			SOT_BeefierLoot.SetValueInt(5)
			EndIf
	EndIf


	If (a_option == potiongive)
		potiongiveStatus = !potiongiveStatus
		SetToggleOptionValue(a_option, potiongiveStatus)
		If potiongiveStatus == True 
		GiveMePotion.SetValueInt(0)	
		Else
		GiveMePotion.SetValueInt(5)	
		EndIf
	EndIf
	
If (a_option == spawnwithothers)
		spawnwithothersStatus = !spawnwithothersStatus
		SetToggleOptionValue(a_option, spawnwithothersStatus)
		If spawnwithothersStatus == True 
		GenesisClear.SetValueInt(5)	
		Else
		GenesisClear.SetValueInt(0)	
		EndIf
	EndIf	

If (a_option == Atplayerallow)
		AtplayerallowStatus = !AtplayerallowStatus
		SetToggleOptionValue(a_option, AtplayerallowStatus)
		If AtplayerallowStatus == True 
		GenesisAtPlayerAllow.SetValueInt(0)	
		Else
		GenesisAtPlayerAllow.SetValueInt(5)	
		EndIf
	EndIf

If (a_option == togglesurface)
		togglesurfaceStatus = !togglesurfaceStatus
		SetToggleOptionValue(a_option, togglesurfaceStatus)
		If togglesurfaceStatus == True 
		EvilSkull1.Disable()
        EvilSkull2.Disable()
		EvilSkull3.Disable()
		EvilSkull4.Disable()
		EvilSkull5.Disable()
		EvilSkull6.Disable()
		EvilSkull7.Disable()
		EvilSkull8.Disable()
		EvilSkull9.Disable()
		EvilSkull10.Disable()
		EvilSkull11.Disable()
		EvilSkull12.Disable()
		EvilSkull13.Disable()
		EvilSkull14.Disable()
		EvilSkull15.Disable()
		EvilSkull16.Disable()
		EvilSkull17.Disable()
		EvilSkull18.Disable()
		EvilSkull19.Disable()
		EvilSkull20.Disable()
		EvilSkull21.Disable()
		Else
		EvilSkull1.enable()
		EvilSkull2.enable()
		EvilSkull3.enable()
		EvilSkull4.enable()
		EvilSkull5.enable()
		EvilSkull6.enable()
		EvilSkull7.enable()
		EvilSkull8.enable()
		EvilSkull9.enable()
		EvilSkull10.enable()
		EvilSkull11.enable()
		EvilSkull12.enable()
		EvilSkull13.enable()
		EvilSkull14.enable()
		EvilSkull15.enable()
		EvilSkull16.enable()
		EvilSkull17.enable()
		EvilSkull18.enable()
		EvilSkull19.enable()
		EvilSkull20.enable()
		EvilSkull21.enable()
		EndIf
	EndIf
	
	
	EndEvent

Event OnOptionSliderOpen(Int a_option)    ; SLIDERS

	If (a_option == daystoresetslider)
		SetSliderDialogStartValue(GenesisDaysToReset.GetValueInt())
		SetSliderDialogDefaultValue(1)
		SetSliderDialogRange(1, 30)
		SetSliderDialogInterval(1)
	EndIf

	If (a_option == potentialspawns)
		SetSliderDialogStartValue(GenesisPotentialSpawns.GetValueInt())
		SetSliderDialogDefaultValue(10)
		SetSliderDialogRange(1, 20)
		SetSliderDialogInterval(1)
	EndIf	

		If (a_option == SOTSTamina) ; increase stamina on npcs
		SetSliderDialogStartValue(SOT_Stamina.GetValue())
		SetSliderDialogDefaultValue(100.0)
		SetSliderDialogRange(10, 1000)
		SetSliderDialogInterval(1.0)
	EndIf
	
	If (a_option == SOTSTaminaHigh) ; increase stamina on npcs
		SetSliderDialogStartValue(SOT_StaminaHigh.GetValue())
		SetSliderDialogDefaultValue(100.0)
		SetSliderDialogRange(10, 1000)
		SetSliderDialogInterval(1.0)
	EndIf

If (a_option == SOTHealth) ; increase health in npcs
		SetSliderDialogStartValue(SOT_Health.GetValue())
		SetSliderDialogDefaultValue(100.0)
		SetSliderDialogRange(10, 1000)
		SetSliderDialogInterval(1.0)
	EndIf
	
	If (a_option == SOTHealthHigh) ; increase health in npcs
		SetSliderDialogStartValue(SOT_HealthHigh.GetValue())
		SetSliderDialogDefaultValue(100.0)
		SetSliderDialogRange(10, 1000)
		SetSliderDialogInterval(1.0)
	EndIf
	
	If (a_option == SOTMagicka) ; increase health in npcs
		SetSliderDialogStartValue(SOT_Magicka.GetValue())
		SetSliderDialogDefaultValue(100.0)
		SetSliderDialogRange(10, 1000)
		SetSliderDialogInterval(1.0)
	EndIf
	
	If (a_option == SOTMagickaHigh) ; increase health in npcs
		SetSliderDialogStartValue(SOT_MagickaHigh.GetValue())
		SetSliderDialogDefaultValue(100.0)
		SetSliderDialogRange(10, 1000)
		SetSliderDialogInterval(1.0)
	EndIf	
	
		If (a_option == GenesisLoot)
		SetSliderDialogStartValue(GenesisLootChance.GetValueInt())
		SetSliderDialogDefaultValue(33)
		SetSliderDialogRange(1, 100)
		SetSliderDialogInterval(1.0)
	EndIf
	
	If (a_option == potionodds)
		SetSliderDialogStartValue(OddsOfPotion.GetValueInt())
		SetSliderDialogDefaultValue(25)
		SetSliderDialogRange(0, 100)
		SetSliderDialogInterval(1.0)
	EndIf

	If (a_option == GenDistance)
		SetSliderDialogStartValue(GenesisDistance.GetValueInt())
		SetSliderDialogDefaultValue(11000)
		SetSliderDialogRange(1000, 20000)
		SetSliderDialogInterval(1000.0)
	EndIf

	If (a_option == outsidespawns)
		SetSliderDialogStartValue(GenesisOutsideSpawns.GetValueInt())
		SetSliderDialogDefaultValue(10)
		SetSliderDialogRange(1, 20)
		SetSliderDialogInterval(1.0)
	EndIf
	
EndEvent

Event OnOptionSliderAccept(Int a_option, Float a_value)

	If (a_option == daystoresetslider)   
		daystoresetsliderA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		GenesisDaysToReset.SetValueInt(daystoresetsliderA)
	EndIf

	If (a_option == potentialspawns)   
		daystoresetsliderA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		GenesisPotentialSpawns.SetValueInt(daystoresetsliderA)
	EndIf

		If (a_option == SOTSTamina)   
		SOTSTaminaA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		SOT_Stamina.SetValueInt(SOTSTaminaA)
		UpdateCurrentInstanceGlobal(SOT_Stamina)
	EndIf
	
	If (a_option == SOTSTaminaHigh)   
		SOTSTaminaHighA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		SOT_StaminaHigh.SetValueInt(SOTSTaminaHighA)
		UpdateCurrentInstanceGlobal(SOT_StaminaHIgh)
	EndIf

	If (a_option == SOTHealth)  
		SOTHealthA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		SOT_Health.SetValueInt(SOTHealthA)
		UpdateCurrentInstanceGlobal(SOT_Health)
	EndIf
	
	If (a_option == SOTHealthHigh)  
		SOTHealthHighA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		SOT_HealthHigh.SetValueInt(SOTHealthHighA)
		UpdateCurrentInstanceGlobal(SOT_HealthHigh)
	EndIf
	
		If (a_option == SOTMagicka)  
		SOTMagickaA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		SOT_Magicka.SetValueInt(SOTMagickaA)
		UpdateCurrentInstanceGlobal(SOT_Magicka)
	EndIf
	
	If (a_option == SOTMagickaHigh)  
		SOTMagickaHighA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		SOT_MagickaHigh.SetValueInt(SOTMagickaHighA)
		UpdateCurrentInstanceGlobal(SOT_MagickaHigh)
	EndIf
	
	If (a_option == GenesisLoot)  
		GenesisLootA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		GenesisLootChance.SetValueInt(GenesisLootA)
		UpdateCurrentInstanceGlobal(GenesisLootChance)
	EndIf
	
		If (a_option == potionodds)  
		potionoddsA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		OddsOfPotion.SetValueInt(potionoddsA)
		UpdateCurrentInstanceGlobal(OddsOfPotion)
	EndIf
	
		If (a_option == GenDistance)  ; distance for genesis to scan
		GenDistanceA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		GenesisDistance.SetValueInt(GenDistanceA)
	EndIf

If (a_option == outsidespawns)  ; surface spawns allowed
		outsidespawnsA = a_value As Int
		SetSliderOptionValue(a_option, a_value)
		GenesisOutsideSpawns.SetValueInt(outsidespawnsA)
	EndIf
	
EndEvent




Event OnOptionHighlight(Int a_option)   ; HIGHLIGHT
If (a_option == ModActivation)
SetInfoText("Toggles Mod On or Off")
elseif (a_option == daystoresetslider)
SetInfoText("Days before Genesis effect can run again in a location where it just ran.  The setting is per dungeon.  ")
ElseIf (a_option == potentialspawns)
 SetInfoText("Desired Maximum Number of Dungeon Spawns.  Final Actual Number May be Less if Location is Small.")
EndIf

If (a_option == outsidespawns)
		SetInfoText("Desired Maximum Number of Spawns on the Surface.  Final Actual Number May be Less if Location is Small.")
	EndIf

If (a_option == togglesurface)
		SetInfoText("Wether to Disable the 21 Surface Spawn Points.  A 22nd Cannot Be Disabled.")
	EndIf

If (a_option == Atplayerallow)
		SetInfoText("Wether to Allow Spawns To Appear Near The Players Field of View.  Default is Yes.")
	EndIf


If (a_option == spawnwithothers)
		SetInfoText("Wether to Allow Spawns If Hostile NPCs Are Already In The Loaded Area.  Default is No.")
	EndIf


If (a_option == SOTStamina)
		SetInfoText("If Unlevelled NPCs Are Activated Then This Value is Used to Adjust Lowest Stamina Value in Spawned NPCS. The sliders use a percentage of the players base health, magika, stamina with 100 being equal to the player.")
	EndIf
	
	If (a_option == SOTStaminaHigh)
		SetInfoText("If Unlevelled NPCs Are Activated Then This Value is Used to Adjust Highest Stamina Value in Spawned NPCS. The sliders use a percentage of the players base health, magika, stamina with 100 being equal to the player.")
	EndIf
	
	
	
	If (a_option == SOTHEalth)
		SetInfoText("If Unlevelled NPCs Are Activated Then This Value is Used to Adjust Health in Spawned NPCS. The sliders use a percentage of the players base health, magika, stamina with 100 being equal to the player.")
	EndIf
	
	If (a_option == SOTHEalthHigh)
		SetInfoText("If Unlevelled NPCs Are Activated Then This Value is Used to Adjust Highest Health Value in Spawned NPCS. The sliders use a percentage of the players base health, magika, stamina with 100 being equal to the player.")
	EndIf
	
	
	If (a_option == SOTMagicka)
		SetInfoText("If Unlevelled NPCs Are Activated Then This Value is Used to Adjust Magicka in Spawned NPCS. The sliders use a percentage of the players base health, magika, stamina with 100 being equal to the player.")
	EndIf
	
	If (a_option == SOTMagickaHigh)
		SetInfoText("If Unlevelled NPCs Are Activated Then This Value is Used to Adjust Highest Magicka Value in Spawned NPCS. The sliders use a percentage of the players base health, magika, stamina with 100 being equal to the player.")
	EndIf
	
	If (a_option == AllowUnlevelledNPC)
		SetInfoText("If Enabled Then The Stamina, Health, and Magicka Will be Altered As Per Settings Below. This Will Affect Spawns from Surface, Dungeon, Sleeping and Fast Travel Encounters.")
	EndIf
 If (a_option == AllowBeefierLoot)
		SetInfoText("Allow Genesis Feature to Add More Interesting Treasure To Containers.")
	EndIf

If (a_option == GenesisLoot)
		SetInfoText("Odds of Treasure Spawning.")
	EndIf

			If (a_option == potionodds)
		SetInfoText("Odds of an NPC Getting A Potion.")
	EndIf
	
		If (a_option == potiongive)
		SetInfoText("Whether to Give Spawned Enemy NPCs Potions.")
	EndIf
If (a_option == GenDistance)
		SetInfoText("How Deep to Scan When Genesis Effect is Looking For Spawn Location.  For example, Bleak Falls Barrow is Roughly 15,000 Units Deep. Default is 11,000 For Scan.")
	EndIf	

EndEvent
	
	
GlobalVariable Property GenesisDaysToReset  Auto
GlobalVariable Property GenesisOddsToSpawnDraugr  Auto  ; all odds variables are for version 2 when and if
GlobalVariable Property GenesisOddsToSpawn  Auto  
GlobalVariable Property GenesisOddsToSpawnBandits  Auto  
GlobalVariable Property GenesisOddsToSpawnSpiders  Auto  
GlobalVariable Property GenesisOddsToSpawnSkels  Auto  
GlobalVariable Property GenesisOddsToSpawnBoss  Auto
 GlobalVariable Property GenesisOddsToSpawnFalmer  Auto  
;
GlobalVariable Property GenesisPotentialSpawns  Auto
GlobalVariable Property GenesisLootChance  Auto
GlobalVariable Property GiveMePotion  Auto  ; wether to give npc a potion, if yes then zero
GlobalVariable Property OddsOfPotion  Auto ; chance of newly created npc getting a potion
GlobalVariable Property GenesisDistance  Auto
GlobalVariable Property GenesisClear  Auto
GlobalVariable Property GenesisAtPlayerAllow  Auto  

ObjectReference Property EvilSkull1  Auto  
ObjectReference Property EvilSkull2  Auto 
ObjectReference Property EvilSkull3  Auto 
ObjectReference Property EvilSkull4  Auto 
ObjectReference Property EvilSkull5  Auto 
ObjectReference Property EvilSkull6  Auto 
ObjectReference Property EvilSkull7  Auto 
ObjectReference Property EvilSkull8  Auto 
ObjectReference Property EvilSkull9  Auto 
ObjectReference Property EvilSkull10  Auto 
ObjectReference Property EvilSkull11  Auto 
ObjectReference Property EvilSkull12  Auto 
ObjectReference Property EvilSkull13  Auto 
ObjectReference Property EvilSkull14  Auto 
ObjectReference Property EvilSkull15  Auto 
ObjectReference Property EvilSkull16  Auto 
ObjectReference Property EvilSkull17  Auto 
ObjectReference Property EvilSkull18  Auto 
ObjectReference Property EvilSkull19  Auto 
ObjectReference Property EvilSkull20  Auto 
ObjectReference Property EvilSkull21  Auto 

GlobalVariable Property GenesisOutsideSpawns  Auto  

GlobalVariable Property testing  Auto  
