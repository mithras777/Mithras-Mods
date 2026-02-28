ScriptName GenesisMenu Extends ObjectReference  


Actor PlayerRef
Actor Player
Int Property increase Auto
Int Property decrease Auto



Event OnEquipped(Actor akActor)
	PlayerRef = Game.GetPlayer()
	Player = Game.GetPlayer()
	If (akActor == Game.GetPlayer()) 
		Game.DisablePlayerControls(False, False, False, False, False, True)
		Menu()
		Game.EnablePlayerControls()
		;Game.EnablePlayerControls(False, False, False, False, False, True)
	EndIf
EndEvent


Function Menu (Bool abmenu = True, Int aibutton = 0, Int aimessage = 0)
	While abmenu
		If aibutton == -1
		ElseIf aimessage == 0
			;;;;;;;;;;;;;;;;;;
			;;;;;main menu;;;;
			;;;;;;;;;;;;;;;;;;
			aiButton = GenesisMenuPage1.Show() ; Shows your menu
			If aibutton == 3 ; exit
				abmenu = False
			ElseIf aibutton == 0 ; main options
				aimessage = 1
			ElseIf aibutton == 1   ; NPC Re-Leveler
				aimessage = 2  
			ElseIf aibutton == 2   ; Treasure Module
				aimessage = 3
			EndIf
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		ElseIf aimessage == 1
			;;;;;;;;;;;;;;;;;;;;;
			;;;;MAIN OPTIONS;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			aiButton = MainOptions.Show() 
			If aibutton == 9
				aimessage = 0
			ElseIf aiButton == 0
				kill35.SetValueInt(0)
				aimessage = 1
			ElseIf aiButton == 1
				kill35.SetValueInt(1)
				aimessage = 1
			ElseIf aibutton == 2
				aimessage = 4 ; go to cooldown menu done!
			ElseIf aibutton == 3
				aimessage = 5 ;  spawns menu
			ElseIf aibutton == 4
				aimessage = 6 ; scan distance menu
			ElseIf aibutton == 5
				GenesisClear.SetValueInt(0)
				aimessage = 1
			ElseIf aiButton == 6
				GenesisClear.SetValueInt(5)
				aimessage = 1
			ElseIf aiButton == 7
				GenesisAtPlayerAllow.SetValueInt(5)
				aimessage = 1
			ElseIf aiButton == 8
				GenesisAtPlayerAllow.SetValueInt(0)
				aimessage = 1
			EndIf
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		ElseIf aimessage == 2
			;;;;;;;;;;;;;;;;;;;;;
			;;;;NPC Re-Leveler;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			aiButton = NPCReleveler.Show() 
			If aibutton == 5
				aimessage = 0
			ElseIf aibutton == 6
				abmenu = False
			ElseIf aiButton == 0
				SOT_AllowUnlevelledNPC.SetValueInt(0)
				aimessage = 2
			ElseIf aibutton == 1
				SOT_AllowUnlevelledNPC.SetValueInt(1)
				aimessage = 2
			ElseIf aiButton == 2 ; Health menu needed
				aimessage = 9
			ElseIf aiButton == 3 ; stamina menu needed
				aimessage = 9
			ElseIf aiButton == 4 ; magicka button needed
				aimessage = 9	
				
				
				
			EndIf
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		ElseIf aimessage == 3
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Treasure Module;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			aiButton = TreasureMenu.Show() 
			If aibutton == 6
				aimessage = 0
			ElseIf aibutton == 7
				abmenu = False
			ElseIf aiButton == 0
				aimessage = 3
				SOT_BeefierLoot.setvalueint(5)
			ElseIf aiButton == 1
				aimessage = 3
				SOT_BeefierLoot.setvalueint(0)
			ElseIf aibutton == 3
				aimessage = 3
				GiveMePotion.setvalueint(5)
			ElseIf aibutton == 4
				aimessage = 3
				GiveMePotion.setvalueint(0)
			
			ElseIf aibutton == 2 ; chance of treasure
				aimessage = 18 ; chance of treasure menu
			ElseIf aiButton == 5
				aimessage = 19 ; chance of potions menu
			
			EndIf  
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		ElseIf aimessage == 4
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Cooldown menu;;;;;Variable is GenesisDaysToReset 1 to 30 days
			;;;;;;;;;;;;;;;;;;;;;
			Int coolvalue = GenesisDaysToReset.GetValueInt()
			aiButton = CooldownMenu.Show(afArg1 = coolvalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 1
			ElseIf aibutton == 0  ; plus 1
				If GenesisDaysToReset.GetValue()<100
					increase = GenesisDaysToReset.GetValueInt()
					GenesisDaysToReset.SetValueInt(Increase+1)
				ElseIf GenesisDaysToReset.GetValue()>99
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 4
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If GenesisDaysToReset.GetValueInt()<91
					increase = GenesisDaysToReset.GetValueInt()
					increase = increase + 10
					GenesisDaysToReset.SetValueInt(Increase)
					aimessage = 4
				ElseIf GenesisDaysToReset.GetValue()>90
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 4
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If GenesisDaysToReset.GetValue() > 0
					decrease = GenesisDaysToReset.GetValueInt()
					decrease = decrease - 1
					GenesisDaysToReset.SetValue(Decrease)
					aimessage = 4
				ElseIf  GenesisDaysToReset.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 4
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If GenesisDaysToReset.GetValue()>9
					decrease = GenesisDaysToReset.GetValueInt()
					decrease = decrease - 10
					GenesisDaysToReset.SetValue(Decrease)
					aimessage = 4
				ElseIf  GenesisDaysToReset.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 4
				EndIf
			EndIf
			
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
			
		ElseIf aimessage == 5
			;;;;;;;;;;;;;;;;;;;;;
			;;;;spawns top menu;;;;; will then go to aimessage 7 and 8
			;;;;;;;;;;;;;;;;;;;;;
			aiButton = ChooseSurfaceorDungeon.Show() 
			If aibutton == 2
				aimessage = 1
			ElseIf aibutton == 3
				abmenu = False
			ElseIf aiButton == 0
				aimessage = 7 ; surface spawns
			ElseIf aiButton == 1
				aimessage = 8 ; dungeon spawns
			EndIf
			
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
			
		ElseIf aimessage == 7
			;;;;;;;;;;;;;;;;;;;;;
			;;;;surface spawns;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int surfacevalue = GenesisOutsideSpawns.GetValueInt()
			aiButton = SurfaceSpawns.Show(afArg1 = surfacevalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 5
			ElseIf aibutton == 0  ; plus 1
				If GenesisOutsideSpawns.GetValue()<100
					increase = GenesisOutsideSpawns.GetValueInt()
					GenesisOutsideSpawns.SetValueInt(Increase+1)
				ElseIf GenesisOutsideSpawns.GetValue()>99
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 7
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If GenesisOutsideSpawns.GetValueInt()<91
					increase = GenesisOutsideSpawns.GetValueInt()
					increase = increase + 10
					GenesisOutsideSpawns.SetValueInt(Increase)
					aimessage = 7
				ElseIf GenesisOutsideSpawns.GetValue()>90
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 7
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If GenesisOutsideSpawns.GetValue() > 0
					decrease = GenesisOutsideSpawns.GetValueInt()
					decrease = decrease - 1
					GenesisOutsideSpawns.SetValue(Decrease)
					aimessage = 7
				ElseIf  GenesisOutsideSpawns.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 7
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If GenesisOutsideSpawns.GetValue()>9
					decrease = GenesisOutsideSpawns.GetValueInt()
					decrease = decrease - 10
					GenesisOutsideSpawns.SetValue(Decrease)
					aimessage = 7
				ElseIf  GenesisOutsideSpawns.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 7
				EndIf
			EndIf
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			;;;;dungeon spawns;;;;;
			;;;;;;;;;;;;;;;;;;;;;
		ElseIf aimessage == 8
			Int dungeonvalue = GenesisPotentialSpawns.GetValueInt()
			aiButton = DungeonSpawns.Show(afArg1 = dungeonvalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 5
			ElseIf aibutton == 0  ; plus 1
				If GenesisPotentialSpawns.GetValue()<100
					increase = GenesisPotentialSpawns.GetValueInt()
					GenesisPotentialSpawns.SetValueInt(Increase+1)
				ElseIf GenesisPotentialSpawns.GetValue()>99
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 8
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If GenesisPotentialSpawns.GetValueInt()<91
					increase = GenesisPotentialSpawns.GetValueInt()
					increase = increase + 10
					GenesisPotentialSpawns.SetValueInt(Increase)
					aimessage = 8
				ElseIf GenesisPotentialSpawns.GetValue()>90
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 8
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If GenesisPotentialSpawns.GetValue() > 0
					decrease = GenesisPotentialSpawns.GetValueInt()
					decrease = decrease - 1
					GenesisPotentialSpawns.SetValue(Decrease)
					aimessage = 8
				ElseIf  GenesisPotentialSpawns.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 8
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If GenesisPotentialSpawns.GetValue()>9
					decrease = GenesisPotentialSpawns.GetValueInt()
					decrease = decrease - 10
					GenesisPotentialSpawns.SetValue(Decrease)
					aimessage = 8
				ElseIf  GenesisPotentialSpawns.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 8
				EndIf
			EndIf
			
			
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			
			;;;;;;;;;;;;;;;;;;;;;
			;;;;scan distance;;;;;
			;;;;;;;;;;;;;;;;;;;;;
		ElseIf aimessage == 6
			Int scanvalue = GenesisDistance.GetValueInt()
			aiButton = ScandistanceMenu.Show(afArg1 = scanvalue) ; Shows your menu
			If aibutton == 7
				abmenu = False
			ElseIf aibutton == 6
				aimessage = 1
			ElseIf aibutton == 0  ; plus 1
				If GenesisDistance.GetValue()<20000
					increase = GenesisDistance.GetValueInt()
					GenesisDistance.SetValueInt(Increase+1)
				ElseIf GenesisDistance.GetValue()>19999
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 6
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If GenesisDistance.GetValueInt()<19991
					increase = GenesisDistance.GetValueInt()
					increase = increase + 10
					GenesisDistance.SetValueInt(Increase)
					aimessage = 6
				ElseIf GenesisDistance.GetValue()>90
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 6
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If GenesisDistance.GetValue() > 0
					decrease = GenesisDistance.GetValueInt()
					decrease = decrease - 1
					GenesisDistance.SetValue(Decrease)
					aimessage = 6
				ElseIf  GenesisDistance.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 6
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If GenesisDistance.GetValue()>9
					decrease = GenesisDistance.GetValueInt()
					decrease = decrease - 10
					GenesisDistance.SetValue(Decrease)
					aimessage = 6
				ElseIf  GenesisDistance.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 6
				EndIf
			ElseIf aibutton == 4 ; minus 1000
				If GenesisDistance.GetValue()>900
					decrease = GenesisDistance.GetValueInt()
					decrease = decrease - 1000
					GenesisDistance.SetValue(Decrease)
					aimessage = 6
				ElseIf  GenesisDistance.GetValue()<1000
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 6
				EndIf	
			ElseIf aibutton == 5  ; plus 1000
				;int increase 10
				If GenesisDistance.GetValueInt()<19000
					increase = GenesisDistance.GetValueInt()
					increase = increase + 1000
					GenesisDistance.SetValueInt(Increase)
					aimessage = 6
				ElseIf GenesisDistance.GetValue()>19000
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 6
				EndIf
				
			EndIf
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			
			ElseIf aimessage == 9
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Health Stamina Magicka Relevel;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int healthvalue = SOT_Health.GetValueInt()
			aiButton = RelevelHealth.Show() ; Shows your menu
			If aibutton == 7
				abmenu = False
			ElseIf aibutton == 6
				aimessage = 2
			ElseIf aibutton == 0  ; Health Low
				aimessage = 12
			ElseIf aibutton == 1 ; Health High
			    aimessage = 13
			ElseIf aiButton == 2 ; stamina low
				aimessage = 14
			ElseIf aiButton == 3 ; stamina high
				aimessage = 15
			ElseIf aiButton == 4 ; magicka low
				aimessage = 16
			ElseIf aiButton == 5 ; magicka high
				aimessage = 17
			EndIf
					
			
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ElseIf aimessage == 12
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Health Low;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int healthvalue = SOT_Health.GetValueInt()
			aiButton = RelevelHealthLow.Show(afArg1 = healthvalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 9
			ElseIf aibutton == 0  ; plus 1
				If SOT_Health.GetValue()<1000
					increase = SOT_Health.GetValueInt()
					SOT_Health.SetValueInt(Increase+1)
				ElseIf SOT_Health.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 12
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If SOT_Health.GetValueInt()<910
					increase = SOT_Health.GetValueInt()
					increase = increase + 10
					SOT_Health.SetValueInt(Increase)
					aimessage = 12
				ElseIf SOT_Health.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 12
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If SOT_Health.GetValue() > 0
					decrease = SOT_Health.GetValueInt()
					decrease = decrease - 1
					SOT_Health.SetValue(Decrease)
					aimessage = 12
				ElseIf  SOT_Health.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 12
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If SOT_Health.GetValue()>9
					decrease = SOT_Health.GetValueInt()
					decrease = decrease - 10
					SOT_Health.SetValue(Decrease)
					aimessage = 12
				ElseIf  SOT_Health.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 12
				EndIf
			EndIf
			
			
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ElseIf aimessage == 13
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Health High;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int healthvalue = SOT_HealthHigh.GetValueInt()
			aiButton = RelevelHealthHigh.Show(afArg1 = healthvalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 9
			ElseIf aibutton == 0  ; plus 1
				If SOT_HealthHigh.GetValue()<1000
					increase = SOT_HealthHigh.GetValueInt()
					SOT_HealthHigh.SetValueInt(Increase+1)
				ElseIf SOT_HealthHigh.GetValue()>990
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 13
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If SOT_HealthHigh.GetValueInt()<910
					increase = SOT_HealthHigh.GetValueInt()
					increase = increase + 10
					SOT_HealthHigh.SetValueInt(Increase)
					aimessage = 13
				ElseIf SOT_HealthHigh.GetValue()>990
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 13
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If SOT_HealthHigh.GetValue() > 0
					decrease = SOT_HealthHigh.GetValueInt()
					decrease = decrease - 1
					SOT_HealthHigh.SetValue(Decrease)
					aimessage = 13
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If SOT_HealthHigh.GetValue()>9
					decrease = SOT_HealthHigh.GetValueInt()
					decrease = decrease - 10
					SOT_HealthHigh.SetValue(Decrease)
					aimessage = 13
				ElseIf  SOT_HealthHigh.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 13
				EndIf
			EndIf

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ElseIf aimessage == 14
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Stamina Low;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int staminavalue = SOT_Stamina.GetValueInt()
			aiButton = RelevelStaminaLow.Show(afArg1 = staminavalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 9
			ElseIf aibutton == 0  ; plus 1
				If SOT_Stamina.GetValue()<1000
					increase = SOT_Stamina.GetValueInt()
					SOT_Stamina.SetValueInt(Increase+1)
					aimessage = 14
				ElseIf SOT_Stamina.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 14
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If SOT_Stamina.GetValueInt()<910
					increase = SOT_Stamina.GetValueInt()
					increase = increase + 10
					SOT_Stamina.SetValueInt(Increase)
					aimessage = 14
				ElseIf SOT_Stamina.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 14
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If SOT_Stamina.GetValue() > 0
					decrease = SOT_Stamina.GetValueInt()
					decrease = decrease - 1
					SOT_Stamina.SetValue(Decrease)
					aimessage = 14
				ElseIf  SOT_Stamina.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 14
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If SOT_Stamina.GetValue()>9
					decrease = SOT_Stamina.GetValueInt()
					decrease = decrease - 10
					SOT_Stamina.SetValue(Decrease)
					aimessage = 14
				ElseIf  SOT_Stamina.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 14
				EndIf
			EndIf
			
			
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;		


ElseIf aimessage == 15
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Stamina High;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int staminahvalue = SOT_StaminaHigh.GetValueInt()
			aiButton = RelevelStaminaHigh.Show(afArg1 = staminahvalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 9
			ElseIf aibutton == 0  ; plus 1
				If SOT_StaminaHigh.GetValue()<1000
					increase = SOT_StaminaHigh.GetValueInt()
					SOT_StaminaHigh.SetValueInt(Increase+1)
					aimessage = 15
				ElseIf SOT_StaminaHigh.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 15
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If SOT_StaminaHigh.GetValueInt()<910
					increase = SOT_StaminaHigh.GetValueInt()
					increase = increase + 10
					SOT_StaminaHigh.SetValueInt(Increase)
					aimessage = 15
				ElseIf SOT_StaminaHigh.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 15
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If SOT_StaminaHigh.GetValue() > 0
					decrease = SOT_StaminaHigh.GetValueInt()
					decrease = decrease - 1
					SOT_StaminaHigh.SetValue(Decrease)
					aimessage = 15
				ElseIf  SOT_StaminaHigh.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 15
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If SOT_StaminaHigh.GetValue()>9
					decrease = SOT_StaminaHigh.GetValueInt()
					decrease = decrease - 10
					SOT_StaminaHigh.SetValue(Decrease)
					aimessage = 15
				ElseIf  SOT_StaminaHigh.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 15
				EndIf
			EndIf











;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ElseIf aimessage == 16
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Magicka Low;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int magickavalue = SOTMagicka.GetValueInt()
			aiButton = RelevelMagickaLow.Show(afArg1 = magickavalue)
			If aibutton == 5
				abmenu = False
			elseif aibutton == 4
				aimessage = 9
			ElseIf aibutton == 0  ; plus 1
				If SOTMagicka.GetValue()<1000
					increase = SOTMagicka.GetValueInt()
					SOTMagicka.SetValueInt(Increase+1)
				ElseIf SOTMagicka.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 16
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If SOTMagicka.GetValueInt()<910
					increase = SOTMagicka.GetValueInt()
					increase = increase + 10
					SOTMagicka.SetValueInt(Increase)
					aimessage = 16
				ElseIf SOTMagicka.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 16
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If SOTMagicka.GetValue() > 0
					decrease = SOTMagicka.GetValueInt()
					decrease = decrease - 1
					SOTMagicka.SetValue(Decrease)
					aimessage = 16
				ElseIf  SOTMagicka.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 16
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If SOTMagicka.GetValue()>9
					decrease = SOTMagicka.GetValueInt()
					decrease = decrease - 10
					SOTMagicka.SetValue(Decrease)
					aimessage = 16
				ElseIf  SOTMagicka.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 16
			EndIf
			endif
			
			
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ElseIf aimessage == 17
			;;;;;;;;;;;;;;;;;;;;;
			;;;;Magicka High;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int healthhvalue = SOTMagickaHigh.GetValueInt()
			aiButton = RelevelMagickaHigh.Show(afArg1 = healthhvalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 9
			ElseIf aibutton == 0  ; plus 1
				If SOTMagickaHigh.GetValue()<1000
					increase = SOTMagickaHigh.GetValueInt()
					SOTMagickaHigh.SetValueInt(Increase+1)
				ElseIf SOTMagickaHigh.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 17
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If SOTMagickaHigh.GetValueInt()<910
					increase = SOTMagickaHigh.GetValueInt()
					increase = increase + 10
					SOTMagickaHigh.SetValueInt(Increase)
					aimessage = 17
				ElseIf SOTMagickaHigh.GetValue()>990
					Debug.Notification("Cannot increase beyond 1000%")
					aimessage = 17
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If SOTMagickaHigh.GetValue() > 0
					decrease = SOTMagickaHigh.GetValueInt()
					decrease = decrease - 1
					SOTMagickaHigh.SetValue(Decrease)
					aimessage = 17
				ElseIf  SOTMagickaHigh.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 17
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If SOTMagickaHigh.GetValue()>9
					decrease = SOTMagickaHigh.GetValueInt()
					decrease = decrease - 10
					SOTMagickaHigh.SetValue(Decrease)
					aimessage = 17
				ElseIf  SOTMagickaHigh.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 17
				EndIf
			EndIf
			


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


ElseIf aimessage == 18
			;;;;;;;;;;;;;;;;;;;;;
			;;;;chance of treasure;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int treasurevalue = GenesisLootChance.GetValueInt()
			aiButton = ChanceOfTreasure.Show(afArg1 = treasurevalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 3
			ElseIf aibutton == 0  ; plus 1
				If GenesisLootChance.GetValue()<100
					increase = GenesisLootChance.GetValueInt()
					GenesisLootChance.SetValueInt(Increase+1)
				ElseIf GenesisLootChance.GetValue()>99
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 18
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If GenesisLootChance.GetValueInt()<91
					increase = GenesisLootChance.GetValueInt()
					increase = increase + 10
					GenesisLootChance.SetValueInt(Increase)
					aimessage = 18
				ElseIf GenesisLootChance.GetValue()>90
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 18
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If GenesisLootChance.GetValue() > 0
					decrease = GenesisLootChance.GetValueInt()
					decrease = decrease - 1
					GenesisLootChance.SetValue(Decrease)
					aimessage = 18
				ElseIf  GenesisLootChance.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 18
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If GenesisLootChance.GetValue()>9
					decrease = GenesisLootChance.GetValueInt()
					decrease = decrease - 10
					GenesisLootChance.SetValue(Decrease)
					aimessage = 18
				ElseIf  GenesisLootChance.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 18
				EndIf
			EndIf


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ElseIf aimessage == 19
			;;;;;;;;;;;;;;;;;;;;;
			;;;;chance of potions;;;;;
			;;;;;;;;;;;;;;;;;;;;;
			Int potionvalue = oddsofpotion.GetValueInt()
			aiButton = ChanceofPotions.Show(afArg1 = potionvalue) ; Shows your menu
			If aibutton == 5
				abmenu = False
			ElseIf aibutton == 4
				aimessage = 3
			ElseIf aibutton == 0  ; plus 1
				If oddsofpotion.GetValue()<100
					increase = oddsofpotion.GetValueInt()
					oddsofpotion.SetValueInt(Increase+1)
				ElseIf oddsofpotion.GetValue()>99
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 19
				EndIf
			ElseIf aibutton == 1  ; plus 10
				;int increase 10
				If oddsofpotion.GetValueInt()<91
					increase = oddsofpotion.GetValueInt()
					increase = increase + 10
					oddsofpotion.SetValueInt(Increase)
					aimessage = 19
				ElseIf oddsofpotion.GetValue()>90
					Debug.Notification("Cannot increase beyond 100%")
					aimessage = 19
				EndIf
			ElseIf aibutton == 2 ; minus 1
				If oddsofpotion.GetValue() > 0
					decrease = oddsofpotion.GetValueInt()
					decrease = decrease - 1
					oddsofpotion.SetValue(Decrease)
					aimessage = 19
				ElseIf  oddsofpotion.GetValue()<1
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 19
				EndIf
			ElseIf aibutton == 3 ; minus 10
				If oddsofpotion.GetValue()>9
					decrease = oddsofpotion.GetValueInt()
					decrease = decrease - 10
					oddsofpotion.SetValue(Decrease)
					aimessage = 19
				ElseIf  oddsofpotion.GetValue()<10
					Debug.Notification("Cannot Decrease beyond 0%")
					aimessage = 19
				EndIf
			EndIf

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

















		EndIf
	EndWhile
	;Game.EnablePlayerControls(False, False, False, False, False, True)
	Game.EnablePlayerControls()
EndFunction
Message Property GenesisMenuPage1  Auto  

Message Property MainOptions  Auto  

Message Property NPCReleveler  Auto  

Message Property TreasureMenu  Auto  

GlobalVariable Property kill35  Auto  

GlobalVariable Property GenesisClear  Auto  

GlobalVariable Property GenesisAtPlayerAllow  Auto  

Message Property CooldownMenu  Auto  

GlobalVariable Property GenesisDaysToReset  Auto

Message Property ChooseSurfaceorDungeon  Auto  
Message Property SurfaceSpawns  Auto 
Message Property DungeonSpawns  Auto 
Message Property ScandistanceMenu  Auto 
GlobalVariable Property GenesisPotentialSpawns  Auto
GlobalVariable Property GenesisOutsideSpawns  Auto
GlobalVariable Property GenesisDistance  Auto
GlobalVariable Property SOT_AllowUnlevelledNPC  Auto
Message Property NPCRelevel  Auto  

Message Property RelevelHealth  Auto  
GlobalVariable Property SOT_Health  Auto
GlobalVariable Property SOT_HealthHigh  Auto
GlobalVariable Property SOT_Stamina  Auto
GlobalVariable Property SOT_StaminaHigh  Auto
GlobalVariable Property SOTMagicka  Auto
GlobalVariable Property SOTMagickaHigh  Auto
Message Property RelevelHealthLow  Auto  
Message Property RelevelHealthHigh  Auto  
Message Property RelevelStaminaLow  Auto  
Message Property RelevelStaminaHigh  Auto

Message Property RelevelMagickaLow  Auto
Message Property RelevelMagickaHigh  Auto

GlobalVariable Property SOT_BeefierLoot  Auto
GlobalVariable Property GenesisLootChance  Auto
GlobalVariable Property OddsOfPotion  Auto
GlobalVariable Property GiveMePotion  Auto


Message Property ChanceOfTreasure  Auto
Message Property ChanceofPotions  Auto
