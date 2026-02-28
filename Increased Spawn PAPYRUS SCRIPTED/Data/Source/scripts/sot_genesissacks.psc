Scriptname sot_genesissacks extends ReferenceAlias  



Int Property inout Auto
int whichone

Event OnOpen(ObjectReference akActionRef)
  if (akActionRef == Game.GetPlayer()) && inout == 0 && SOT_BeefierLoot.GetValueInt() == 0 && Utility.RandomInt(1, 100) <= GenesisLootChance.getvalueint()
  inout = 5
 ;Debug.Notification("ON OPEN SACKS!")
  whichone = Utility.RandomInt(1, 3)
    self.make1(self.GetReference())
  if whichone > 1
  self.make2(self.GetReference())
  EndIf
  if whichone > 2
  self.make3(self.GetReference())
EndIf
EndIf  
  
  
  EndEvent




LeveledItem Function Make1 (ObjectReference Box)
Int JIndex = SackLoot.GetSize()
				Int JRandom = Utility.RandomInt(0, JIndex)
				LeveledItem potiontospawn = SackLoot.GetAt(Jrandom) As LeveledItem
				Box.AddItem(potiontospawn)
EndFunction

LeveledItem Function Make2 (ObjectReference Box)
Int JIndex = SackLoot.GetSize()
				Int JRandom = Utility.RandomInt(0, JIndex)
				LeveledItem scrolltospawn = SackLoot.GetAt(Jrandom) As LeveledItem
				Box.AddItem(scrolltospawn)
EndFunction

LeveledItem Function Make3 (ObjectReference Box)
Int JIndex = SackLoot.GetSize()
				Int JRandom = Utility.RandomInt(0, JIndex)
				LeveledItem MiscObjecttospawn = SackLoot.GetAt(Jrandom) As LeveledItem
				Box.AddItem(MiscObjecttospawn)
EndFunction

FormList Property PotionList  Auto  

FormList Property ScrollList  Auto  

FormList Property MiscLoot  Auto  

GlobalVariable Property SOT_BeefierLoot  Auto  

FormList Property SackLoot  Auto  

GlobalVariable Property GenesisLootChance  Auto  


